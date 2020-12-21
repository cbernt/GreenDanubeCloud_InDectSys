/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MediaCodecReader.h"

#include <OMX_IVCommon.h>

#include <gui/Surface.h>
#include <ICrypto.h>

#include <stagefright/foundation/ABuffer.h>
#include <stagefright/foundation/ADebug.h>
#include <stagefright/foundation/ALooper.h>
#include <stagefright/foundation/AMessage.h>
#include <stagefright/MediaBuffer.h>
#include <stagefright/MediaCodec.h>
#include <stagefright/MediaDefs.h>
#include <stagefright/MediaExtractor.h>
#include <stagefright/MediaSource.h>
#include <stagefright/MetaData.h>
#include <stagefright/Utils.h>

#include "mozilla/TimeStamp.h"

#include "gfx2DGlue.h"

#include "MediaStreamSource.h"
#include "MediaTaskQueue.h"
#include "MP3FrameParser.h"
#include "nsThreadUtils.h"
#include "ImageContainer.h"
#include "SharedThreadPool.h"
#include "VideoFrameContainer.h"

using namespace android;

namespace mozilla {

enum {
  kNotifyCodecReserved = 'core',
  kNotifyCodecCanceled = 'coca',
};

static const int64_t sInvalidDurationUs = INT64_C(-1);
static const int64_t sInvalidTimestampUs = INT64_C(-1);

// Try not to spend more than this much time (in seconds) in a single call
// to GetCodecOutputData.
static const double sMaxAudioDecodeDurationS = 0.1;
static const double sMaxVideoDecodeDurationS = 0.1;

static CheckedUint32 sInvalidInputIndex = INT32_C(-1);

inline bool
IsValidDurationUs(int64_t aDuration)
{
  return aDuration >= INT64_C(0);
}

inline bool
IsValidTimestampUs(int64_t aTimestamp)
{
  return aTimestamp >= INT64_C(0);
}

MediaCodecReader::MessageHandler::MessageHandler(MediaCodecReader* aReader)
  : mReader(aReader)
{
}

MediaCodecReader::MessageHandler::~MessageHandler()
{
  mReader = nullptr;
}

void
MediaCodecReader::MessageHandler::onMessageReceived(
  const sp<AMessage>& aMessage)
{
  if (mReader) {
    mReader->onMessageReceived(aMessage);
  }
}

MediaCodecReader::VideoResourceListener::VideoResourceListener(
  MediaCodecReader* aReader)
  : mReader(aReader)
{
}

MediaCodecReader::VideoResourceListener::~VideoResourceListener()
{
  mReader = nullptr;
}

void
MediaCodecReader::VideoResourceListener::codecReserved()
{
  if (mReader) {
    mReader->codecReserved(mReader->mVideoTrack);
  }
}

void
MediaCodecReader::VideoResourceListener::codecCanceled()
{
  if (mReader) {
    mReader->codecCanceled(mReader->mVideoTrack);
  }
}

bool
MediaCodecReader::TrackInputCopier::Copy(MediaBuffer* aSourceBuffer,
                                         sp<ABuffer> aCodecBuffer)
{
  if (aSourceBuffer == nullptr ||
      aCodecBuffer == nullptr ||
      aSourceBuffer->range_length() > aCodecBuffer->capacity()) {
    return false;
  }

  aCodecBuffer->setRange(0, aSourceBuffer->range_length());
  memcpy(aCodecBuffer->data(),
         (uint8_t*)aSourceBuffer->data() + aSourceBuffer->range_offset(),
         aSourceBuffer->range_length());

  return true;
}

MediaCodecReader::Track::Track()
  : mSourceIsStopped(true)
  , mDurationLock("MediaCodecReader::Track::mDurationLock")
  , mDurationUs(INT64_C(0))
  , mInputIndex(sInvalidInputIndex)
  , mInputEndOfStream(false)
  , mOutputEndOfStream(false)
  , mSeekTimeUs(sInvalidTimestampUs)
  , mFlushed(false)
  , mDiscontinuity(false)
  , mTaskQueue(nullptr)
{
}

// Append the value of |kKeyValidSamples| to the end of each vorbis buffer.
// https://github.com/mozilla-b2g/platform_frameworks_av/blob/master/media/libstagefright/OMXCodec.cpp#L3128
// https://github.com/mozilla-b2g/platform_frameworks_av/blob/master/media/libstagefright/NuMediaExtractor.cpp#L472
bool
MediaCodecReader::VorbisInputCopier::Copy(MediaBuffer* aSourceBuffer,
                                          sp<ABuffer> aCodecBuffer)
{
  if (aSourceBuffer == nullptr ||
      aCodecBuffer == nullptr ||
      aSourceBuffer->range_length() + sizeof(int32_t) > aCodecBuffer->capacity()) {
    return false;
  }

  int32_t numPageSamples = 0;
  if (!aSourceBuffer->meta_data()->findInt32(kKeyValidSamples, &numPageSamples)) {
    numPageSamples = -1;
  }

  aCodecBuffer->setRange(0, aSourceBuffer->range_length() + sizeof(int32_t));
  memcpy(aCodecBuffer->data(),
         (uint8_t*)aSourceBuffer->data() + aSourceBuffer->range_offset(),
         aSourceBuffer->range_length());
  memcpy(aCodecBuffer->data() + aSourceBuffer->range_length(),
         &numPageSamples, sizeof(numPageSamples));

  return true;
}

MediaCodecReader::AudioTrack::AudioTrack()
{
}

MediaCodecReader::VideoTrack::VideoTrack()
  : mWidth(0)
  , mHeight(0)
  , mStride(0)
  , mSliceHeight(0)
  , mColorFormat(0)
  , mRotation(0)
{
}

MediaCodecReader::CodecBufferInfo::CodecBufferInfo()
  : mIndex(0)
  , mOffset(0)
  , mSize(0)
  , mTimeUs(0)
  , mFlags(0)
{
}

MediaCodecReader::SignalObject::SignalObject(const char* aName)
  : mMonitor(aName)
  , mSignaled(false)
{
}

MediaCodecReader::SignalObject::~SignalObject()
{
}

void
MediaCodecReader::SignalObject::Wait()
{
  MonitorAutoLock al(mMonitor);
  if (!mSignaled) {
    mMonitor.Wait();
  }
}

void
MediaCodecReader::SignalObject::Signal()
{
  MonitorAutoLock al(mMonitor);
  mSignaled = true;
  mMonitor.Notify();
}

MediaCodecReader::ParseCachedDataRunnable::ParseCachedDataRunnable(nsRefPtr<MediaCodecReader> aReader,
                                                                   const char* aBuffer,
                                                                   uint32_t aLength,
                                                                   int64_t aOffset,
                                                                   nsRefPtr<SignalObject> aSignal)
  : mReader(aReader)
  , mBuffer(aBuffer)
  , mLength(aLength)
  , mOffset(aOffset)
  , mSignal(aSignal)
{
  MOZ_ASSERT(mReader, "Should have a valid MediaCodecReader.");
  MOZ_ASSERT(mBuffer, "Should have a valid buffer.");
  MOZ_ASSERT(mOffset >= INT64_C(0), "Should have a valid offset.");
}

NS_IMETHODIMP
MediaCodecReader::ParseCachedDataRunnable::Run()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  if (mReader->ParseDataSegment(mBuffer, mLength, mOffset)) {
    MonitorAutoLock monLock(mReader->mParserMonitor);
    if (mReader->mNextParserPosition >= mOffset + mLength &&
        mReader->mParsedDataLength < mOffset + mLength) {
      mReader->mParsedDataLength = mOffset + mLength;
    }
  }

  if (mSignal != nullptr) {
    mSignal->Signal();
  }

  return NS_OK;
}

MediaCodecReader::ProcessCachedDataTask::ProcessCachedDataTask(nsRefPtr<MediaCodecReader> aReader,
                                                               int64_t aOffset)
  : mReader(aReader)
  , mOffset(aOffset)
{
  MOZ_ASSERT(mReader, "Should have a valid MediaCodecReader.");
  MOZ_ASSERT(mOffset >= INT64_C(0), "Should have a valid offset.");
}

void
MediaCodecReader::ProcessCachedDataTask::Run()
{
  mReader->ProcessCachedData(mOffset, nullptr);
  nsRefPtr<ReferenceKeeperRunnable<MediaCodecReader>> runnable(
      new ReferenceKeeperRunnable<MediaCodecReader>(mReader));
  mReader = nullptr;
  NS_DispatchToMainThread(runnable.get());
}

MediaCodecReader::MediaCodecReader(AbstractMediaDecoder* aDecoder)
  : MediaOmxCommonReader(aDecoder)
  , mColorConverterBufferSize(0)
  , mExtractor(nullptr)
  , mParserMonitor("MediaCodecReader::mParserMonitor")
  , mParseDataFromCache(true)
  , mNextParserPosition(INT64_C(0))
  , mParsedDataLength(INT64_C(0))
{
  mHandler = new MessageHandler(this);
  mVideoListener = new VideoResourceListener(this);
}

MediaCodecReader::~MediaCodecReader()
{
  MOZ_ASSERT(NS_IsMainThread(), "Should be on main thread.");
}

nsresult
MediaCodecReader::Init(MediaDecoderReader* aCloneDonor)
{
  return NS_OK;
}

bool
MediaCodecReader::IsWaitingMediaResources()
{
  return mVideoTrack.mCodec != nullptr && !mVideoTrack.mCodec->allocated();
}

bool
MediaCodecReader::IsDormantNeeded()
{
  return mVideoTrack.mSource != nullptr;
}

void
MediaCodecReader::ReleaseMediaResources()
{
  // Stop the mSource because we are in the dormant state and the stop function
  // will rewind the mSource to the beginning of the stream.
  if (mVideoTrack.mSource != nullptr) {
    mVideoTrack.mSource->stop();
    mVideoTrack.mSourceIsStopped = true;
  }
  if (mAudioTrack.mSource != nullptr) {
    mAudioTrack.mSource->stop();
    mAudioTrack.mSourceIsStopped = true;
  }
  ReleaseCriticalResources();
}

void
MediaCodecReader::Shutdown()
{
  ReleaseResources();
}

void
MediaCodecReader::DispatchAudioTask()
{
  if (mAudioTrack.mTaskQueue && mAudioTrack.mTaskQueue->IsEmpty()) {
    RefPtr<nsIRunnable> task =
      NS_NewRunnableMethod(this,
                           &MediaCodecReader::DecodeAudioDataTask);
    mAudioTrack.mTaskQueue->Dispatch(task);
  }
}

void
MediaCodecReader::DispatchVideoTask(int64_t aTimeThreshold)
{
  if (mVideoTrack.mTaskQueue && mVideoTrack.mTaskQueue->IsEmpty()) {
    RefPtr<nsIRunnable> task =
      NS_NewRunnableMethodWithArg<int64_t>(this,
                                           &MediaCodecReader::DecodeVideoFrameTask,
                                           aTimeThreshold);
    mVideoTrack.mTaskQueue->Dispatch(task);
  }
}

void
MediaCodecReader::RequestAudioData()
{
  MOZ_ASSERT(GetTaskQueue()->IsCurrentThreadIn());
  MOZ_ASSERT(HasAudio());
  if (CheckAudioResources()) {
    DispatchAudioTask();
  }
}

void
MediaCodecReader::RequestVideoData(bool aSkipToNextKeyframe,
                                   int64_t aTimeThreshold)
{
  MOZ_ASSERT(GetTaskQueue()->IsCurrentThreadIn());
  MOZ_ASSERT(HasVideo());

  int64_t threshold = sInvalidTimestampUs;
  if (aSkipToNextKeyframe && IsValidTimestampUs(aTimeThreshold)) {
    mVideoTrack.mTaskQueue->Flush();
    threshold = aTimeThreshold;
  }
  if (CheckVideoResources()) {
    DispatchVideoTask(threshold);
  }
}

bool
MediaCodecReader::DecodeAudioDataSync()
{
  if (mAudioTrack.mCodec == nullptr || !mAudioTrack.mCodec->allocated() ||
      mAudioTrack.mOutputEndOfStream) {
    return false;
  }

  // Get one audio output data from MediaCodec
  CodecBufferInfo bufferInfo;
  status_t status;
  TimeStamp timeout = TimeStamp::Now() +
                      TimeDuration::FromSeconds(sMaxAudioDecodeDurationS);
  while (true) {
    // Try to fill more input buffers and then get one output buffer.
    // FIXME: use callback from MediaCodec
    FillCodecInputData(mAudioTrack);

    status = GetCodecOutputData(mAudioTrack, bufferInfo, sInvalidTimestampUs,
                                timeout);
    if (status == OK || status == ERROR_END_OF_STREAM) {
      break;
    } else if (status == -EAGAIN) {
      if (TimeStamp::Now() > timeout) {
        // Don't let this loop run for too long. Try it again later.
        if (CheckAudioResources()) {
          DispatchAudioTask();
        }
        return true;
      }
      continue; // Try it again now.
    } else if (status == INFO_FORMAT_CHANGED) {
      if (UpdateAudioInfo()) {
        continue; // Try it again now.
      } else {
        return false;
      }
    } else {
      return false;
    }
  }

  bool result = false;
  if (bufferInfo.mBuffer != nullptr && bufferInfo.mSize > 0 &&
      bufferInfo.mBuffer->data() != nullptr) {
    // This is the approximate byte position in the stream.
    int64_t pos = mDecoder->GetResource()->Tell();

    uint32_t frames = bufferInfo.mSize /
                      (mInfo.mAudio.mChannels * sizeof(AudioDataValue));

    result = mAudioCompactor.Push(
      pos,
      bufferInfo.mTimeUs,
      mInfo.mAudio.mRate,
      frames,
      mInfo.mAudio.mChannels,
      AudioCompactor::NativeCopy(
        bufferInfo.mBuffer->data() + bufferInfo.mOffset,
        bufferInfo.mSize,
        mInfo.mAudio.mChannels));
  }

  if ((bufferInfo.mFlags & MediaCodec::BUFFER_FLAG_EOS) ||
      (status == ERROR_END_OF_STREAM)) {
    AudioQueue().Finish();
  }
  mAudioTrack.mCodec->releaseOutputBuffer(bufferInfo.mIndex);

  return result;
}

bool
MediaCodecReader::DecodeAudioDataTask()
{
  bool result = DecodeAudioDataSync();
  if (AudioQueue().GetSize() > 0) {
    AudioData* a = AudioQueue().PopFront();
    if (a) {
      if (mAudioTrack.mDiscontinuity) {
        a->mDiscontinuity = true;
        mAudioTrack.mDiscontinuity = false;
      }
      GetCallback()->OnAudioDecoded(a);
    }
  }
  if (AudioQueue().AtEndOfStream()) {
    GetCallback()->OnAudioEOS();
  }
  return result;
}

bool
MediaCodecReader::DecodeVideoFrameTask(int64_t aTimeThreshold)
{
  bool result = DecodeVideoFrameSync(aTimeThreshold);
  if (VideoQueue().GetSize() > 0) {
    VideoData* v = VideoQueue().PopFront();
    if (v) {
      if (mVideoTrack.mDiscontinuity) {
        v->mDiscontinuity = true;
        mVideoTrack.mDiscontinuity = false;
      }
      GetCallback()->OnVideoDecoded(v);
    }
  }
  if (VideoQueue().AtEndOfStream()) {
    GetCallback()->OnVideoEOS();
  }
  return result;
}

bool
MediaCodecReader::HasAudio()
{
  return mInfo.mAudio.mHasAudio;
}

bool
MediaCodecReader::HasVideo()
{
  return mInfo.mVideo.mHasVideo;
}

void
MediaCodecReader::NotifyDataArrived(const char* aBuffer,
                                    uint32_t aLength,
                                    int64_t aOffset)
{
  MonitorAutoLock monLock(mParserMonitor);
  if (mNextParserPosition == mParsedDataLength &&
      mNextParserPosition >= aOffset &&
      mNextParserPosition <= aOffset + aLength) {
    // No pending parsing runnable currently. And available data are adjacent to
    // parsed data.
    int64_t shift = mNextParserPosition - aOffset;
    const char* buffer = aBuffer + shift;
    uint32_t length = aLength - shift;
    int64_t offset = mNextParserPosition;
    if (length > 0) {
      MonitorAutoUnlock monUnlock(mParserMonitor);
      ParseDataSegment(buffer, length, offset);
    }
    mParseDataFromCache = false;
    mParsedDataLength = offset + length;
    mNextParserPosition = mParsedDataLength;
  }
}

int64_t
MediaCodecReader::ProcessCachedData(int64_t aOffset,
                                    nsRefPtr<SignalObject> aSignal)
{
  // We read data in chunks of 32 KiB. We can reduce this
  // value if media, such as sdcards, is too slow.
  // Because of SD card's slowness, need to keep sReadSize to small size.
  // See Bug 914870.
  static const int64_t sReadSize = 32 * 1024;

  MOZ_ASSERT(!NS_IsMainThread(), "Should not be on main thread.");

  {
    MonitorAutoLock monLock(mParserMonitor);
    if (!mParseDataFromCache) {
      // Skip cache processing since data can be continuously be parsed by
      // ParseDataSegment() from NotifyDataArrived() directly.
      return INT64_C(0);
    }
  }

  MediaResource *resource = mDecoder->GetResource();
  MOZ_ASSERT(resource);

  int64_t resourceLength = resource->GetCachedDataEnd(0);
  NS_ENSURE_TRUE(resourceLength >= 0, INT64_C(-1));

  if (aOffset >= resourceLength) {
    return INT64_C(0); // Cache is empty, nothing to do
  }

  int64_t bufferLength = std::min<int64_t>(resourceLength - aOffset, sReadSize);

  nsAutoArrayPtr<char> buffer(new char[bufferLength]);

  nsresult rv = resource->ReadFromCache(buffer.get(), aOffset, bufferLength);
  NS_ENSURE_SUCCESS(rv, INT64_C(-1));

  MonitorAutoLock monLock(mParserMonitor);
  if (mParseDataFromCache) {
    nsRefPtr<ParseCachedDataRunnable> runnable(
      new ParseCachedDataRunnable(this,
                                  buffer.forget(),
                                  bufferLength,
                                  aOffset,
                                  aSignal));

    rv = NS_DispatchToMainThread(runnable.get());
    NS_ENSURE_SUCCESS(rv, INT64_C(-1));

    mNextParserPosition = aOffset + bufferLength;
    if (mNextParserPosition < resource->GetCachedDataEnd(0)) {
      // We cannot read data in the main thread because it
      // might block for too long. Instead we post an IO task
      // to the IO thread if there is more data available.
      XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
          new ProcessCachedDataTask(this, mNextParserPosition));
    }
  }

  return bufferLength;
}

bool
MediaCodecReader::ParseDataSegment(const char* aBuffer,
                                   uint32_t aLength,
                                   int64_t aOffset)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  int64_t duration = INT64_C(-1);

  {
    MonitorAutoLock monLock(mParserMonitor);

    // currently only mp3 files are supported for incremental parsing
    if (mMP3FrameParser == nullptr) {
      return false;
    }

    if (!mMP3FrameParser->IsMP3()) {
      return true; // NO-OP
    }

    mMP3FrameParser->Parse(aBuffer, aLength, aOffset);

    duration = mMP3FrameParser->GetDuration();
  }

  bool durationUpdateRequired = false;

  {
    MutexAutoLock al(mAudioTrack.mDurationLock);
    if (duration > mAudioTrack.mDurationUs) {
      mAudioTrack.mDurationUs = duration;
      durationUpdateRequired = true;
    }
  }

  if (durationUpdateRequired && HasVideo()) {
    MutexAutoLock al(mVideoTrack.mDurationLock);
    durationUpdateRequired = duration > mVideoTrack.mDurationUs;
  }

  if (durationUpdateRequired) {
    MOZ_ASSERT(mDecoder);
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mDecoder->UpdateEstimatedMediaDuration(duration);
  }

  return true;
}

nsresult
MediaCodecReader::ReadMetadata(MediaInfo* aInfo,
                               MetadataTags** aTags)
{
  MOZ_ASSERT(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  if (!ReallocateResources()) {
    return NS_ERROR_FAILURE;
  }

  if (!TriggerIncrementalParser()) {
    return NS_ERROR_FAILURE;
  }

  if (IsWaitingMediaResources()) {
    return NS_OK;
  }

  // TODO: start streaming

  if (!UpdateDuration()) {
    return NS_ERROR_FAILURE;
  }

  if (!UpdateAudioInfo()) {
    return NS_ERROR_FAILURE;
  }

  if (!UpdateVideoInfo()) {
    return NS_ERROR_FAILURE;
  }

  // Set the total duration (the max of the audio and video track).
  int64_t audioDuration = INT64_C(-1);
  {
    MutexAutoLock al(mAudioTrack.mDurationLock);
    audioDuration = mAudioTrack.mDurationUs;
  }
  int64_t videoDuration = INT64_C(-1);
  {
    MutexAutoLock al(mVideoTrack.mDurationLock);
    videoDuration = mVideoTrack.mDurationUs;
  }
  int64_t duration = audioDuration > videoDuration ? audioDuration : videoDuration;
  if (duration >= INT64_C(0)) {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mDecoder->SetMediaDuration(duration);
  }

  // Video track's frame sizes will not overflow. Activate the video track.
  VideoFrameContainer* container = mDecoder->GetVideoFrameContainer();
  if (container) {
    container->SetCurrentFrame(
      gfxIntSize(mInfo.mVideo.mDisplay.width, mInfo.mVideo.mDisplay.height),
      nullptr,
      mozilla::TimeStamp::Now());
  }

  *aInfo = mInfo;
  *aTags = nullptr;

#ifdef MOZ_AUDIO_OFFLOAD
  CheckAudioOffload();
#endif

  return NS_OK;
}

nsresult
MediaCodecReader::ResetDecode()
{
  if (CheckAudioResources()) {
    mAudioTrack.mTaskQueue->Flush();
    FlushCodecData(mAudioTrack);
    mAudioTrack.mDiscontinuity = true;
  }
  if (CheckVideoResources()) {
    mVideoTrack.mTaskQueue->Flush();
    FlushCodecData(mVideoTrack);
    mVideoTrack.mDiscontinuity = true;
  }

  return MediaDecoderReader::ResetDecode();
}

bool
MediaCodecReader::DecodeVideoFrameSync(int64_t aTimeThreshold)
{
  if (mVideoTrack.mCodec == nullptr || !mVideoTrack.mCodec->allocated() ||
      mVideoTrack.mOutputEndOfStream) {
    return false;
  }

  // Get one video output data from MediaCodec
  CodecBufferInfo bufferInfo;
  status_t status;
  TimeStamp timeout = TimeStamp::Now() +
                      TimeDuration::FromSeconds(sMaxVideoDecodeDurationS);
  while (true) {
    // Try to fill more input buffers and then get one output buffer.
    // FIXME: use callback from MediaCodec
    FillCodecInputData(mVideoTrack);

    status = GetCodecOutputData(mVideoTrack, bufferInfo, aTimeThreshold,
                                timeout);
    if (status == OK || status == ERROR_END_OF_STREAM) {
      break;
    } else if (status == -EAGAIN) {
      if (TimeStamp::Now() > timeout) {
        // Don't let this loop run for too long. Try it again later.
        if (CheckVideoResources()) {
          DispatchVideoTask(aTimeThreshold);
        }
        return true;
      }
      continue; // Try it again now.
    } else if (status == INFO_FORMAT_CHANGED) {
      if (UpdateVideoInfo()) {
        continue; // Try it again now.
      } else {
        return false;
      }
    } else {
      return false;
    }
  }

  bool result = false;
  if (bufferInfo.mBuffer != nullptr && bufferInfo.mSize > 0 &&
      bufferInfo.mBuffer->data() != nullptr) {
    uint8_t* yuv420p_buffer = bufferInfo.mBuffer->data();
    int32_t stride = mVideoTrack.mStride;
    int32_t slice_height = mVideoTrack.mSliceHeight;

    // Converts to OMX_COLOR_FormatYUV420Planar
    if (mVideoTrack.mColorFormat != OMX_COLOR_FormatYUV420Planar) {
      ARect crop;
      crop.top = 0;
      crop.bottom = mVideoTrack.mHeight;
      crop.left = 0;
      crop.right = mVideoTrack.mWidth;

      yuv420p_buffer = GetColorConverterBuffer(mVideoTrack.mWidth,
                                               mVideoTrack.mHeight);
      if (mColorConverter.convertDecoderOutputToI420(
            bufferInfo.mBuffer->data(), mVideoTrack.mWidth, mVideoTrack.mHeight,
            crop, yuv420p_buffer) != OK) {
        mVideoTrack.mCodec->releaseOutputBuffer(bufferInfo.mIndex);
        NS_WARNING("Unable to convert color format");
        return false;
      }

      stride = mVideoTrack.mWidth;
      slice_height = mVideoTrack.mHeight;
    }

    size_t yuv420p_y_size = stride * slice_height;
    size_t yuv420p_u_size = ((stride + 1) / 2) * ((slice_height + 1) / 2);
    uint8_t* yuv420p_y = yuv420p_buffer;
    uint8_t* yuv420p_u = yuv420p_y + yuv420p_y_size;
    uint8_t* yuv420p_v = yuv420p_u + yuv420p_u_size;

    // This is the approximate byte position in the stream.
    int64_t pos = mDecoder->GetResource()->Tell();

    VideoData::YCbCrBuffer b;
    b.mPlanes[0].mData = yuv420p_y;
    b.mPlanes[0].mWidth = mVideoTrack.mWidth;
    b.mPlanes[0].mHeight = mVideoTrack.mHeight;
    b.mPlanes[0].mStride = stride;
    b.mPlanes[0].mOffset = 0;
    b.mPlanes[0].mSkip = 0;

    b.mPlanes[1].mData = yuv420p_u;
    b.mPlanes[1].mWidth = (mVideoTrack.mWidth + 1) / 2;
    b.mPlanes[1].mHeight = (mVideoTrack.mHeight + 1) / 2;
    b.mPlanes[1].mStride = (stride + 1) / 2;
    b.mPlanes[1].mOffset = 0;
    b.mPlanes[1].mSkip = 0;

    b.mPlanes[2].mData = yuv420p_v;
    b.mPlanes[2].mWidth =(mVideoTrack.mWidth + 1) / 2;
    b.mPlanes[2].mHeight = (mVideoTrack.mHeight + 1) / 2;
    b.mPlanes[2].mStride = (stride + 1) / 2;
    b.mPlanes[2].mOffset = 0;
    b.mPlanes[2].mSkip = 0;

    VideoData *v = VideoData::Create(
      mInfo.mVideo,
      mDecoder->GetImageContainer(),
      pos,
      bufferInfo.mTimeUs,
      1, // We don't know the duration.
      b,
      bufferInfo.mFlags & MediaCodec::BUFFER_FLAG_SYNCFRAME,
      -1,
      mVideoTrack.mRelativePictureRect);

    if (v) {
      result = true;
      VideoQueue().Push(v);
    } else {
      NS_WARNING("Unable to create VideoData");
    }
  }

  if ((bufferInfo.mFlags & MediaCodec::BUFFER_FLAG_EOS) ||
      (status == ERROR_END_OF_STREAM)) {
    VideoQueue().Finish();
  }
  mVideoTrack.mCodec->releaseOutputBuffer(bufferInfo.mIndex);

  return result;
}

nsresult
MediaCodecReader::Seek(int64_t aTime,
                       int64_t aStartTime,
                       int64_t aEndTime,
                       int64_t aCurrentTime)
{
  MOZ_ASSERT(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  mVideoTrack.mSeekTimeUs = aTime;
  mAudioTrack.mSeekTimeUs = aTime;
  mVideoTrack.mInputEndOfStream = false;
  mVideoTrack.mOutputEndOfStream = false;
  mAudioTrack.mInputEndOfStream = false;
  mAudioTrack.mOutputEndOfStream = false;
  mAudioTrack.mFlushed = false;
  mVideoTrack.mFlushed = false;

  if (CheckVideoResources()) {
    VideoFrameContainer* videoframe = mDecoder->GetVideoFrameContainer();
    if (videoframe) {
      layers::ImageContainer* image = videoframe->GetImageContainer();
      if (image) {
        image->ClearAllImagesExceptFront();
      }
    }

    MediaBuffer* source_buffer = nullptr;
    MediaSource::ReadOptions options;
    int64_t timestamp = sInvalidTimestampUs;
    options.setSeekTo(aTime, MediaSource::ReadOptions::SEEK_PREVIOUS_SYNC);
    if (mVideoTrack.mSource->read(&source_buffer, &options) != OK ||
        source_buffer == nullptr) {
      return NS_ERROR_FAILURE;
    }
    sp<MetaData> format = source_buffer->meta_data();
    if (format != nullptr) {
      if (format->findInt64(kKeyTime, &timestamp) &&
          IsValidTimestampUs(timestamp)) {
        mVideoTrack.mSeekTimeUs = timestamp;
        mAudioTrack.mSeekTimeUs = timestamp;
      }
      format = nullptr;
    }
    source_buffer->release();

    MOZ_ASSERT(mVideoTrack.mTaskQueue->IsEmpty());
    DispatchVideoTask(mVideoTrack.mSeekTimeUs);

    if (CheckAudioResources()) {
      MOZ_ASSERT(mAudioTrack.mTaskQueue->IsEmpty());
      DispatchAudioTask();
    }
  } else if (CheckAudioResources()) {// Audio only
    MOZ_ASSERT(mAudioTrack.mTaskQueue->IsEmpty());
    DispatchAudioTask();
  }
  return NS_OK;
}

bool
MediaCodecReader::IsMediaSeekable()
{
  // Check the MediaExtract flag if the source is seekable.
  return (mExtractor != nullptr) &&
         (mExtractor->flags() & MediaExtractor::CAN_SEEK);
}

sp<MediaSource>
MediaCodecReader::GetAudioOffloadTrack()
{
  return mAudioOffloadTrack.mSource;
}

bool
MediaCodecReader::ReallocateResources()
{
  if (CreateLooper() &&
      CreateExtractor() &&
      CreateMediaSources() &&
      CreateMediaCodecs() &&
      CreateTaskQueues()) {
    return true;
  }

  ReleaseResources();
  return false;
}

void
MediaCodecReader::ReleaseCriticalResources()
{
  ResetDecode();
  // Before freeing a video codec, all video buffers needed to be released
  // even from graphics pipeline.
  VideoFrameContainer* videoframe = mDecoder->GetVideoFrameContainer();
  if (videoframe) {
    videoframe->ClearCurrentFrame();
  }

  DestroyMediaCodecs();

  ClearColorConverterBuffer();
}

void
MediaCodecReader::ReleaseResources()
{
  ReleaseCriticalResources();
  DestroyMediaSources();
  DestroyExtractor();
  DestroyLooper();
  ShutdownTaskQueues();
}

bool
MediaCodecReader::CreateLooper()
{
  if (mLooper != nullptr) {
    return true;
  }

  // Create ALooper
  mLooper = new ALooper;
  mLooper->setName("MediaCodecReader");

  // Register AMessage handler to ALooper.
  mLooper->registerHandler(mHandler);

  // Start ALooper thread.
  if (mLooper->start() != OK) {
    return false;
  }

  return true;
}

void
MediaCodecReader::DestroyLooper()
{
  if (mLooper == nullptr) {
    return;
  }

  // Unregister AMessage handler from ALooper.
  if (mHandler != nullptr) {
    mLooper->unregisterHandler(mHandler->id());
  }

  // Stop ALooper thread.
  mLooper->stop();

  // Clear ALooper
  mLooper = nullptr;
}

bool
MediaCodecReader::CreateExtractor()
{
  if (mExtractor != nullptr) {
    return true;
  }

  //register sniffers, if they are not registered in this process.
  DataSource::RegisterDefaultSniffers();

  if (mExtractor == nullptr) {
    sp<DataSource> dataSource = new MediaStreamSource(mDecoder->GetResource());

    if (dataSource->initCheck() != OK) {
      return false;
    }

    mExtractor = MediaExtractor::Create(dataSource);
  }

  return mExtractor != nullptr;
}

void
MediaCodecReader::DestroyExtractor()
{
  mExtractor = nullptr;
}

bool
MediaCodecReader::CreateMediaSources()
{
  if (mExtractor == nullptr) {
    return false;
  }

  mMetaData = mExtractor->getMetaData();

  const ssize_t invalidTrackIndex = -1;
  ssize_t audioTrackIndex = invalidTrackIndex;
  ssize_t videoTrackIndex = invalidTrackIndex;

  for (size_t i = 0; i < mExtractor->countTracks(); ++i) {
    sp<MetaData> trackFormat = mExtractor->getTrackMetaData(i);

    const char* mime;
    if (!trackFormat->findCString(kKeyMIMEType, &mime)) {
      continue;
    }

    if (audioTrackIndex == invalidTrackIndex &&
        !strncasecmp(mime, "audio/", 6)) {
      audioTrackIndex = i;
    } else if (videoTrackIndex == invalidTrackIndex &&
               !strncasecmp(mime, "video/", 6)) {
      videoTrackIndex = i;
    }
  }

  if (audioTrackIndex == invalidTrackIndex &&
      videoTrackIndex == invalidTrackIndex) {
    NS_WARNING("OMX decoder could not find audio or video tracks");
    return false;
  }

  if (audioTrackIndex != invalidTrackIndex && mAudioTrack.mSource == nullptr) {
    sp<MediaSource> audioSource = mExtractor->getTrack(audioTrackIndex);
    if (audioSource != nullptr && audioSource->start() == OK) {
      mAudioTrack.mSource = audioSource;
      mAudioTrack.mSourceIsStopped = false;
    }
    // Get one another track instance for audio offload playback.
    mAudioOffloadTrack.mSource = mExtractor->getTrack(audioTrackIndex);
  }

  if (videoTrackIndex != invalidTrackIndex && mVideoTrack.mSource == nullptr) {
    sp<MediaSource> videoSource = mExtractor->getTrack(videoTrackIndex);
    if (videoSource != nullptr && videoSource->start() == OK) {
      mVideoTrack.mSource = videoSource;
      mVideoTrack.mSourceIsStopped = false;
    }
  }

  return
    (audioTrackIndex == invalidTrackIndex || mAudioTrack.mSource != nullptr) &&
    (videoTrackIndex == invalidTrackIndex || mVideoTrack.mSource != nullptr);
}

void
MediaCodecReader::DestroyMediaSources()
{
  mAudioTrack.mSource = nullptr;
  mVideoTrack.mSource = nullptr;
  mAudioOffloadTrack.mSource = nullptr;
}

void
MediaCodecReader::ShutdownTaskQueues()
{
  if(mAudioTrack.mTaskQueue) {
    mAudioTrack.mTaskQueue->Shutdown();
    mAudioTrack.mTaskQueue = nullptr;
  }
  if(mVideoTrack.mTaskQueue) {
    mVideoTrack.mTaskQueue->Shutdown();
    mVideoTrack.mTaskQueue = nullptr;
  }
}

bool
MediaCodecReader::CreateTaskQueues()
{
  if (mAudioTrack.mSource != nullptr && mAudioTrack.mCodec != nullptr &&
      !mAudioTrack.mTaskQueue) {
    mAudioTrack.mTaskQueue = new MediaTaskQueue(
      SharedThreadPool::Get(NS_LITERAL_CSTRING("MediaCodecReader Audio"), 1));
    NS_ENSURE_TRUE(mAudioTrack.mTaskQueue, false);
  }
 if (mVideoTrack.mSource != nullptr && mVideoTrack.mCodec != nullptr &&
     !mVideoTrack.mTaskQueue) {
    mVideoTrack.mTaskQueue = new MediaTaskQueue(
      SharedThreadPool::Get(NS_LITERAL_CSTRING("MediaCodecReader Video"), 1));
    NS_ENSURE_TRUE(mVideoTrack.mTaskQueue, false);
  }

  return true;
}

bool
MediaCodecReader::CreateMediaCodecs()
{
  if (CreateMediaCodec(mLooper, mAudioTrack, false, nullptr) &&
      CreateMediaCodec(mLooper, mVideoTrack, true, mVideoListener)) {
    return true;
  }

  return false;
}

bool
MediaCodecReader::CreateMediaCodec(sp<ALooper>& aLooper,
                                   Track& aTrack,
                                   bool aAsync,
                                   wp<MediaCodecProxy::CodecResourceListener> aListener)
{
  if (aTrack.mSource != nullptr && aTrack.mCodec == nullptr) {
    sp<MetaData> sourceFormat = aTrack.mSource->getFormat();

    const char* mime;
    if (sourceFormat->findCString(kKeyMIMEType, &mime)) {
      aTrack.mCodec = MediaCodecProxy::CreateByType(aLooper, mime, false, aAsync, aListener);
    }

    if (aTrack.mCodec == nullptr) {
      NS_WARNING("Couldn't create MediaCodecProxy");
      return false;
    }

    if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_VORBIS)) {
      aTrack.mInputCopier = new VorbisInputCopier;
    } else {
      aTrack.mInputCopier = new TrackInputCopier;
    }

    if (!aAsync) {
      // Pending configure() and start() to codecReserved() if the creation
      // should be asynchronous.
      if (!aTrack.mCodec->allocated() || !ConfigureMediaCodec(aTrack)){
        NS_WARNING("Couldn't create and configure MediaCodec synchronously");
        aTrack.mCodec = nullptr;
        return false;
      }
    }
  }

  return true;
}

bool
MediaCodecReader::ConfigureMediaCodec(Track& aTrack)
{
  if (aTrack.mSource != nullptr && aTrack.mCodec != nullptr) {
    if (!aTrack.mCodec->allocated()) {
      return false;
    }

    sp<MetaData> sourceFormat = aTrack.mSource->getFormat();
    sp<AMessage> codecFormat;
    convertMetaDataToMessage(sourceFormat, &codecFormat);

    bool allpass = true;
    if (allpass && aTrack.mCodec->configure(codecFormat, nullptr, nullptr, 0) != OK) {
      NS_WARNING("Couldn't configure MediaCodec");
      allpass = false;
    }
    if (allpass && aTrack.mCodec->start() != OK) {
      NS_WARNING("Couldn't start MediaCodec");
      allpass = false;
    }
    if (allpass && aTrack.mCodec->getInputBuffers(&aTrack.mInputBuffers) != OK) {
      NS_WARNING("Couldn't get input buffers from MediaCodec");
      allpass = false;
    }
    if (allpass && aTrack.mCodec->getOutputBuffers(&aTrack.mOutputBuffers) != OK) {
      NS_WARNING("Couldn't get output buffers from MediaCodec");
      allpass = false;
    }
    if (!allpass) {
      aTrack.mCodec = nullptr;
      return false;
    }
  }

  return true;
}

void
MediaCodecReader::DestroyMediaCodecs()
{
  DestroyMediaCodecs(mAudioTrack);
  DestroyMediaCodecs(mVideoTrack);
}

void
MediaCodecReader::DestroyMediaCodecs(Track& aTrack)
{
  aTrack.mCodec = nullptr;
}

bool
MediaCodecReader::TriggerIncrementalParser()
{
  if (mMetaData == nullptr) {
    return false;
  }

  int64_t duration = INT64_C(-1);

  {
    MonitorAutoLock monLock(mParserMonitor);

    // only support incremental parsing for mp3 currently.
    if (mMP3FrameParser != nullptr) {
      return true;
    }

    mParseDataFromCache = true;
    mNextParserPosition = INT64_C(0);
    mParsedDataLength = INT64_C(0);

    // MP3 file duration
    mMP3FrameParser = new MP3FrameParser(mDecoder->GetResource()->GetLength());
    const char* mime = nullptr;
    if (mMetaData->findCString(kKeyMIMEType, &mime) &&
        !strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_MPEG)) {
      {
        MonitorAutoUnlock monUnlock(mParserMonitor);
        // trigger parsing logic and wait for finishing parsing data in the beginning.
        nsRefPtr<SignalObject> signalObject = new SignalObject("MediaCodecReader::UpdateDuration()");
        if (ProcessCachedData(INT64_C(0), signalObject) > INT64_C(0)) {
          signalObject->Wait();
        }
      }
      duration = mMP3FrameParser->GetDuration();
    }
  }

  {
    MutexAutoLock al(mAudioTrack.mDurationLock);
    if (duration > mAudioTrack.mDurationUs) {
      mAudioTrack.mDurationUs = duration;
    }
  }

  return true;
}

bool
MediaCodecReader::UpdateDuration()
{
  // read audio duration
  if (mAudioTrack.mSource != nullptr) {
    sp<MetaData> audioFormat = mAudioTrack.mSource->getFormat();
    if (audioFormat != nullptr) {
      int64_t duration = INT64_C(0);
      if (audioFormat->findInt64(kKeyDuration, &duration)) {
        MutexAutoLock al(mAudioTrack.mDurationLock);
        if (duration > mAudioTrack.mDurationUs) {
          mAudioTrack.mDurationUs = duration;
        }
      }
    }
  }

  // read video duration
  if (mVideoTrack.mSource != nullptr) {
    sp<MetaData> videoFormat = mVideoTrack.mSource->getFormat();
    if (videoFormat != nullptr) {
      int64_t duration = INT64_C(0);
      if (videoFormat->findInt64(kKeyDuration, &duration)) {
        MutexAutoLock al(mVideoTrack.mDurationLock);
        if (duration > mVideoTrack.mDurationUs) {
          mVideoTrack.mDurationUs = duration;
        }
      }
    }
  }

  return true;
}

bool
MediaCodecReader::UpdateAudioInfo()
{
  if (mAudioTrack.mSource == nullptr && mAudioTrack.mCodec == nullptr) {
    // No needs to update AudioInfo if no audio streams.
    return true;
  }

  if (mAudioTrack.mSource == nullptr || mAudioTrack.mCodec == nullptr ||
      !mAudioTrack.mCodec->allocated()) {
    // Something wrong.
    MOZ_ASSERT(mAudioTrack.mSource != nullptr, "mAudioTrack.mSource should not be nullptr");
    MOZ_ASSERT(mAudioTrack.mCodec != nullptr, "mAudioTrack.mCodec should not be nullptr");
    MOZ_ASSERT(mAudioTrack.mCodec->allocated(), "mAudioTrack.mCodec->allocated() should not be false");
    return false;
  }

  // read audio metadata from MediaSource
  sp<MetaData> audioSourceFormat = mAudioTrack.mSource->getFormat();
  if (audioSourceFormat == nullptr) {
    return false;
  }

  // ensure audio metadata from MediaCodec has been parsed
  if (!EnsureCodecFormatParsed(mAudioTrack)){
    return false;
  }

  // read audio metadata from MediaCodec
  sp<AMessage> audioCodecFormat;
  if (mAudioTrack.mCodec->getOutputFormat(&audioCodecFormat) != OK ||
      audioCodecFormat == nullptr) {
    return false;
  }

  AString codec_mime;
  int32_t codec_channel_count = 0;
  int32_t codec_sample_rate = 0;
  if (!audioCodecFormat->findString("mime", &codec_mime) ||
      !audioCodecFormat->findInt32("channel-count", &codec_channel_count) ||
      !audioCodecFormat->findInt32("sample-rate", &codec_sample_rate)) {
    return false;
  }

  // Update AudioInfo
  mInfo.mAudio.mHasAudio = true;
  mInfo.mAudio.mChannels = codec_channel_count;
  mInfo.mAudio.mRate = codec_sample_rate;

  return true;
}

bool
MediaCodecReader::UpdateVideoInfo()
{
  if (mVideoTrack.mSource == nullptr && mVideoTrack.mCodec == nullptr) {
    // No needs to update VideoInfo if no video streams.
    return true;
  }

  if (mVideoTrack.mSource == nullptr || mVideoTrack.mCodec == nullptr ||
      !mVideoTrack.mCodec->allocated()) {
    // Something wrong.
    MOZ_ASSERT(mVideoTrack.mSource != nullptr, "mVideoTrack.mSource should not be nullptr");
    MOZ_ASSERT(mVideoTrack.mCodec != nullptr, "mVideoTrack.mCodec should not be nullptr");
    MOZ_ASSERT(mVideoTrack.mCodec->allocated(), "mVideoTrack.mCodec->allocated() should not be false");
    return false;
  }

  // read video metadata from MediaSource
  sp<MetaData> videoSourceFormat = mVideoTrack.mSource->getFormat();
  if (videoSourceFormat == nullptr) {
    return false;
  }
  int32_t container_width = 0;
  int32_t container_height = 0;
  int32_t container_rotation = 0;
  if (!videoSourceFormat->findInt32(kKeyWidth, &container_width) ||
      !videoSourceFormat->findInt32(kKeyHeight, &container_height)) {
    return false;
  }
  mVideoTrack.mFrameSize = nsIntSize(container_width, container_height);
  if (videoSourceFormat->findInt32(kKeyRotation, &container_rotation)) {
    mVideoTrack.mRotation = container_rotation;
  }

  // ensure video metadata from MediaCodec has been parsed
  if (!EnsureCodecFormatParsed(mVideoTrack)){
    return false;
  }

  // read video metadata from MediaCodec
  sp<AMessage> videoCodecFormat;
  if (mVideoTrack.mCodec->getOutputFormat(&videoCodecFormat) != OK ||
      videoCodecFormat == nullptr) {
    return false;
  }
  AString codec_mime;
  int32_t codec_width = 0;
  int32_t codec_height = 0;
  int32_t codec_stride = 0;
  int32_t codec_slice_height = 0;
  int32_t codec_color_format = 0;
  int32_t codec_crop_left = 0;
  int32_t codec_crop_top = 0;
  int32_t codec_crop_right = 0;
  int32_t codec_crop_bottom = 0;
  if (!videoCodecFormat->findString("mime", &codec_mime) ||
      !videoCodecFormat->findInt32("width", &codec_width) ||
      !videoCodecFormat->findInt32("height", &codec_height) ||
      !videoCodecFormat->findInt32("stride", &codec_stride) ||
      !videoCodecFormat->findInt32("slice-height", &codec_slice_height) ||
      !videoCodecFormat->findInt32("color-format", &codec_color_format) ||
      !videoCodecFormat->findRect("crop", &codec_crop_left, &codec_crop_top,
                                  &codec_crop_right, &codec_crop_bottom)) {
    return false;
  }

  mVideoTrack.mWidth = codec_width;
  mVideoTrack.mHeight = codec_height;
  mVideoTrack.mStride = codec_stride;
  mVideoTrack.mSliceHeight = codec_slice_height;
  mVideoTrack.mColorFormat = codec_color_format;

  // Validate the container-reported frame and pictureRect sizes. This ensures
  // that our video frame creation code doesn't overflow.
  int32_t display_width = codec_crop_right - codec_crop_left + 1;
  int32_t display_height = codec_crop_bottom - codec_crop_top + 1;
  nsIntRect picture_rect(0, 0, mVideoTrack.mWidth, mVideoTrack.mHeight);
  nsIntSize display_size(display_width, display_height);
  if (!IsValidVideoRegion(mVideoTrack.mFrameSize, picture_rect, display_size)) {
    return false;
  }

  // Relative picture size
  gfx::IntRect relative_picture_rect = gfx::ToIntRect(picture_rect);
  if (mVideoTrack.mWidth != mVideoTrack.mFrameSize.width ||
      mVideoTrack.mHeight != mVideoTrack.mFrameSize.height) {
    // Frame size is different from what the container reports. This is legal,
    // and we will preserve the ratio of the crop rectangle as it
    // was reported relative to the picture size reported by the container.
    relative_picture_rect.x = (picture_rect.x * mVideoTrack.mWidth) /
                              mVideoTrack.mFrameSize.width;
    relative_picture_rect.y = (picture_rect.y * mVideoTrack.mHeight) /
                              mVideoTrack.mFrameSize.height;
    relative_picture_rect.width = (picture_rect.width * mVideoTrack.mWidth) /
                                  mVideoTrack.mFrameSize.width;
    relative_picture_rect.height = (picture_rect.height * mVideoTrack.mHeight) /
                                   mVideoTrack.mFrameSize.height;
  }

  // Update VideoInfo
  mInfo.mVideo.mHasVideo = true;
  mVideoTrack.mPictureRect = picture_rect;
  mInfo.mVideo.mDisplay = display_size;
  mVideoTrack.mRelativePictureRect = relative_picture_rect;

  return true;
}

status_t
MediaCodecReader::FlushCodecData(Track& aTrack)
{
  if (aTrack.mSource == nullptr || aTrack.mCodec == nullptr ||
      !aTrack.mCodec->allocated()) {
    return UNKNOWN_ERROR;
  }

  status_t status = aTrack.mCodec->flush();
  aTrack.mFlushed = (status == OK);
  if (aTrack.mFlushed) {
    aTrack.mInputIndex = sInvalidInputIndex;
  }

  return status;
}

// Keep filling data if there are available buffers.
// FIXME: change to non-blocking read
status_t
MediaCodecReader::FillCodecInputData(Track& aTrack)
{
  if (aTrack.mSource == nullptr || aTrack.mCodec == nullptr ||
      !aTrack.mCodec->allocated()) {
    return UNKNOWN_ERROR;
  }

  if (aTrack.mInputEndOfStream) {
    return ERROR_END_OF_STREAM;
  }

  if (IsValidTimestampUs(aTrack.mSeekTimeUs) && !aTrack.mFlushed) {
    FlushCodecData(aTrack);
  }

  size_t index = 0;
  while (aTrack.mInputIndex.isValid() ||
         aTrack.mCodec->dequeueInputBuffer(&index) == OK) {
    if (!aTrack.mInputIndex.isValid()) {
      aTrack.mInputIndex = index;
    }
    MOZ_ASSERT(aTrack.mInputIndex.isValid(), "aElement.mInputIndex should be valid");

    // Start the mSource before we read it.
    if (aTrack.mSourceIsStopped) {
      if (aTrack.mSource->start() == OK) {
        aTrack.mSourceIsStopped = false;
      } else {
        return UNKNOWN_ERROR;
      }
    }
    MediaBuffer* source_buffer = nullptr;
    status_t status = OK;
    if (IsValidTimestampUs(aTrack.mSeekTimeUs)) {
      MediaSource::ReadOptions options;
      options.setSeekTo(aTrack.mSeekTimeUs);
      status = aTrack.mSource->read(&source_buffer, &options);
    } else {
      status = aTrack.mSource->read(&source_buffer);
    }

    // read() fails
    if (status == INFO_FORMAT_CHANGED) {
      return INFO_FORMAT_CHANGED;
    } else if (status == ERROR_END_OF_STREAM) {
      aTrack.mInputEndOfStream = true;
      aTrack.mCodec->queueInputBuffer(aTrack.mInputIndex.value(),
                                      0, 0, 0,
                                      MediaCodec::BUFFER_FLAG_EOS);
      return ERROR_END_OF_STREAM;
    } else if (status == -ETIMEDOUT) {
      return OK; // try it later
    } else if (status != OK) {
      return status;
    } else if (source_buffer == nullptr) {
      return UNKNOWN_ERROR;
    }

    // read() successes
    aTrack.mInputEndOfStream = false;
    aTrack.mSeekTimeUs = sInvalidTimestampUs;

    sp<ABuffer> input_buffer = nullptr;
    if (aTrack.mInputIndex.value() < aTrack.mInputBuffers.size()) {
      input_buffer = aTrack.mInputBuffers[aTrack.mInputIndex.value()];
    }
    if (input_buffer != nullptr &&
        aTrack.mInputCopier != nullptr &&
        aTrack.mInputCopier->Copy(source_buffer, input_buffer)) {
      int64_t timestamp = sInvalidTimestampUs;
      sp<MetaData> codec_format = source_buffer->meta_data();
      if (codec_format != nullptr) {
        codec_format->findInt64(kKeyTime, &timestamp);
      }

      status = aTrack.mCodec->queueInputBuffer(
        aTrack.mInputIndex.value(), input_buffer->offset(),
        input_buffer->size(), timestamp, 0);
      if (status == OK) {
        aTrack.mInputIndex = sInvalidInputIndex;
      }
    }
    source_buffer->release();

    if (status != OK) {
      return status;
    }
  }

  return OK;
}

status_t
MediaCodecReader::GetCodecOutputData(Track& aTrack,
                                     CodecBufferInfo& aBuffer,
                                     int64_t aThreshold,
                                     const TimeStamp& aTimeout)
{
  // Read next frame.
  CodecBufferInfo info;
  status_t status = OK;
  while (status == OK || status == INFO_OUTPUT_BUFFERS_CHANGED ||
         status == -EAGAIN) {

    int64_t duration = (int64_t)(aTimeout - TimeStamp::Now()).ToMicroseconds();
    if (!IsValidDurationUs(duration)) {
      return -EAGAIN;
    }

    status = aTrack.mCodec->dequeueOutputBuffer(&info.mIndex, &info.mOffset,
      &info.mSize, &info.mTimeUs, &info.mFlags, duration);
    // Check EOS first.
    if (status == ERROR_END_OF_STREAM ||
        (info.mFlags & MediaCodec::BUFFER_FLAG_EOS)) {
      aBuffer = info;
      aBuffer.mBuffer = aTrack.mOutputBuffers[info.mIndex];
      aTrack.mOutputEndOfStream = true;
      return ERROR_END_OF_STREAM;
    }

    if (status == OK) {
      if (!IsValidTimestampUs(aThreshold) || info.mTimeUs >= aThreshold) {
        // Get a valid output buffer.
        break;
      } else {
        aTrack.mCodec->releaseOutputBuffer(info.mIndex);
      }
    } else if (status == INFO_OUTPUT_BUFFERS_CHANGED) {
      // Update output buffers of MediaCodec.
      if (aTrack.mCodec->getOutputBuffers(&aTrack.mOutputBuffers) != OK) {
        NS_WARNING("Couldn't get output buffers from MediaCodec");
        aTrack.mCodec = nullptr;
        return UNKNOWN_ERROR;
      }
    }

    if (TimeStamp::Now() > aTimeout) {
      // Don't let this loop run for too long. Try it again later.
      return -EAGAIN;
    }
  }

  if (status != OK) {
    // Something wrong.
    return status;
  }

  if (info.mIndex >= aTrack.mOutputBuffers.size()) {
    NS_WARNING("Couldn't get proper index of output buffers from MediaCodec");
    aTrack.mCodec->releaseOutputBuffer(info.mIndex);
    return UNKNOWN_ERROR;
  }

  aBuffer = info;
  aBuffer.mBuffer = aTrack.mOutputBuffers[info.mIndex];

  return OK;
}

bool
MediaCodecReader::EnsureCodecFormatParsed(Track& aTrack)
{
  if (aTrack.mSource == nullptr || aTrack.mCodec == nullptr ||
      !aTrack.mCodec->allocated()) {
    return false;
  }

  sp<AMessage> format;
  if (aTrack.mCodec->getOutputFormat(&format) == OK) {
    return true;
  }

  status_t status = OK;
  size_t index = 0;
  size_t offset = 0;
  size_t size = 0;
  int64_t timeUs = INT64_C(0);
  uint32_t flags = 0;
  while ((status = aTrack.mCodec->dequeueOutputBuffer(&index, &offset, &size,
                     &timeUs, &flags)) != INFO_FORMAT_CHANGED) {
    if (status == OK) {
      aTrack.mCodec->releaseOutputBuffer(index);
    }
    status = FillCodecInputData(aTrack);
    if (status == INFO_FORMAT_CHANGED) {
      break;
    } else if (status != OK) {
      return false;
    }
  }
  return aTrack.mCodec->getOutputFormat(&format) == OK;
}

uint8_t*
MediaCodecReader::GetColorConverterBuffer(int32_t aWidth, int32_t aHeight)
{
  // Allocate a temporary YUV420Planer buffer.
  size_t yuv420p_y_size = aWidth * aHeight;
  size_t yuv420p_u_size = ((aWidth + 1) / 2) * ((aHeight + 1) / 2);
  size_t yuv420p_v_size = yuv420p_u_size;
  size_t yuv420p_size = yuv420p_y_size + yuv420p_u_size + yuv420p_v_size;
  if (mColorConverterBufferSize != yuv420p_size) {
    mColorConverterBuffer = nullptr; // release the previous buffer first
    mColorConverterBuffer = new uint8_t[yuv420p_size];
    mColorConverterBufferSize = yuv420p_size;
  }
  return mColorConverterBuffer.get();
}

void
MediaCodecReader::ClearColorConverterBuffer()
{
  mColorConverterBuffer = nullptr;
  mColorConverterBufferSize = 0;
}

// Called on MediaCodecReader::mLooper thread.
void
MediaCodecReader::onMessageReceived(const sp<AMessage>& aMessage)
{
  switch (aMessage->what()) {

    case kNotifyCodecReserved:
    {
      // Our decode may have acquired the hardware resource that it needs
      // to start. Notify the state machine to resume loading metadata.
      mDecoder->NotifyWaitingForResourcesStatusChanged();
      break;
    }

    case kNotifyCodecCanceled:
    {
      ReleaseCriticalResources();
      break;
    }

    default:
      TRESPASS();
      break;
  }
}

// Called on Binder thread.
void
MediaCodecReader::codecReserved(Track& aTrack)
{
  if (!ConfigureMediaCodec(aTrack)) {
    DestroyMediaCodecs(aTrack);
    return;
  }

  if (mHandler != nullptr) {
    // post kNotifyCodecReserved to MediaCodecReader::mLooper thread.
    sp<AMessage> notify = new AMessage(kNotifyCodecReserved, mHandler->id());
    notify->post();
  }
}

// Called on Binder thread.
void
MediaCodecReader::codecCanceled(Track& aTrack)
{
  DestroyMediaCodecs(aTrack);

  if (mHandler != nullptr) {
    // post kNotifyCodecCanceled to MediaCodecReader::mLooper thread.
    sp<AMessage> notify = new AMessage(kNotifyCodecCanceled, mHandler->id());
    notify->post();
  }
}

} // namespace mozilla
