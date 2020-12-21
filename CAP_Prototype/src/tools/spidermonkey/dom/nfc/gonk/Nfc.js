/* Copyright 2012 Mozilla Foundation and Mozilla contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Copyright © 2013, Deutsche Telekom, Inc. */

"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "NFC", function () {
  let obj = {};
  Cu.import("resource://gre/modules/nfc_consts.js", obj);
  return obj;
});

Cu.import("resource://gre/modules/systemlibs.js");
const NFC_ENABLED = libcutils.property_get("ro.moz.nfc.enabled", "false") === "true";

// set to true in nfc_consts.js to see debug messages
let DEBUG = NFC.DEBUG_NFC;

let debug;
if (DEBUG) {
  debug = function (s) {
    dump("-*- Nfc: " + s + "\n");
  };
} else {
  debug = function (s) {};
}

const NFC_CONTRACTID = "@mozilla.org/nfc;1";
const NFC_CID =
  Components.ID("{2ff24790-5e74-11e1-b86c-0800200c9a66}");

const NFC_IPC_MSG_NAMES = [
  "NFC:CheckSessionToken"
];

const NFC_IPC_READ_PERM_MSG_NAMES = [
  "NFC:ReadNDEF",
  "NFC:GetDetailsNDEF",
  "NFC:Connect",
  "NFC:Close",
];

const NFC_IPC_WRITE_PERM_MSG_NAMES = [
  "NFC:WriteNDEF",
  "NFC:MakeReadOnlyNDEF",
  "NFC:SendFile",
  "NFC:RegisterPeerReadyTarget",
  "NFC:UnregisterPeerReadyTarget"
];

const NFC_IPC_MANAGER_PERM_MSG_NAMES = [
  "NFC:CheckP2PRegistration",
  "NFC:NotifyUserAcceptedP2P",
  "NFC:NotifySendFileStatus",
  "NFC:StartPoll",
  "NFC:StopPoll",
  "NFC:PowerOff"
];

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageBroadcaster");
XPCOMUtils.defineLazyServiceGetter(this, "gSystemMessenger",
                                   "@mozilla.org/system-message-internal;1",
                                   "nsISystemMessagesInternal");
XPCOMUtils.defineLazyServiceGetter(this, "UUIDGenerator",
                                    "@mozilla.org/uuid-generator;1",
                                    "nsIUUIDGenerator");
XPCOMUtils.defineLazyGetter(this, "gMessageManager", function () {
  return {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIMessageListener,
                                           Ci.nsIObserver]),

    nfc: null,

    // Manage registered Peer Targets
    peerTargets: {},
    currentPeer: null,

    init: function init(nfc) {
      this.nfc = nfc;

      Services.obs.addObserver(this, NFC.TOPIC_XPCOM_SHUTDOWN, false);
      this._registerMessageListeners();
    },

    _shutdown: function _shutdown() {
      this.nfc.shutdown();
      this.nfc = null;

      Services.obs.removeObserver(this, NFC.TOPIC_XPCOM_SHUTDOWN);
      this._unregisterMessageListeners();
    },

    _registerMessageListeners: function _registerMessageListeners() {
      ppmm.addMessageListener("child-process-shutdown", this);

      for (let message of NFC_IPC_MSG_NAMES) {
        ppmm.addMessageListener(message, this);
      }

      for (let message of NFC_IPC_READ_PERM_MSG_NAMES) {
        ppmm.addMessageListener(message, this);
      }

      for (let message of NFC_IPC_WRITE_PERM_MSG_NAMES) {
        ppmm.addMessageListener(message, this);
      }

      for (let message of NFC_IPC_MANAGER_PERM_MSG_NAMES) {
        ppmm.addMessageListener(message, this);
      }
    },

    _unregisterMessageListeners: function _unregisterMessageListeners() {
      ppmm.removeMessageListener("child-process-shutdown", this);

      for (let message of NFC_IPC_MSG_NAMES) {
        ppmm.removeMessageListener(message, this);
      }

      for (let message of NFC_IPC_READ_PERM_MSG_NAMES) {
        ppmm.removeMessageListener(message, this);
      }

      for (let message of NFC_IPC_WRITE_PERM_MSG_NAMES) {
        ppmm.removeMessageListener(message, this);
      }

      for (let message of NFC_IPC_MANAGER_PERM_MSG_NAMES) {
        ppmm.removeMessageListener(message, this);
      }

      ppmm = null;
    },

    registerPeerReadyTarget: function registerPeerReadyTarget(target, appId) {
      if (!this.peerTargets[appId]) {
        this.peerTargets[appId] = target;
      }
    },

    unregisterPeerReadyTarget: function unregisterPeerReadyTarget(appId) {
      if (this.peerTargets[appId]) {
        delete this.peerTargets[appId];
      }
    },

    removePeerTarget: function removePeerTarget(target) {
      Object.keys(this.peerTargets).forEach((appId) => {
        if (this.peerTargets[appId] === target) {
          if (this.currentPeer === target) {
            this.currentPeer = null;
          }
          delete this.peerTargets[appId];
        }
      });
    },

    notifyPeerEvent: function notifyPeerEvent(target, event, sessionToken) {
      if (!target) {
        dump("invalid target");
        return;
      }

      target.sendAsyncMessage("NFC:PeerEvent", {
        event: event,
        sessionToken: sessionToken
      });
    },

    checkP2PRegistration: function checkP2PRegistration(message) {
      // Check if the session and application id yeild a valid registered
      // target.  It should have registered for NFC_PEER_EVENT_READY
      let isValid = (this.nfc._currentSessionId != null) &&
                    (this.peerTargets[message.data.appId] != null);

      let respMsg = { requestId: message.data.requestId };
      if (!isValid) {
        respMsg.errorMsg = this.nfc.getErrorMessage(NFC.NFC_GECKO_ERROR_P2P_REG_INVALID);
      }
      // Notify the content process immediately of the status
      message.target.sendAsyncMessage(message.name + "Response", respMsg);
    },

    notifyUserAcceptedP2P: function notifyUserAcceptedP2P(appId) {
      let target = this.peerTargets[appId];
      let sessionToken = this.nfc.sessionTokenMap[this.nfc._currentSessionId];
      let isValid = (sessionToken != null) && (target != null);
      if (!isValid) {
        debug("Peer already lost or " + appId + " is not a registered PeerReadytarget");
        return;
      }

      // Remember the target that receives onpeerready.
      this.currentPeer = target;
      this.notifyPeerEvent(target, NFC.NFC_PEER_EVENT_READY, sessionToken);
    },

    onPeerLost: function onPeerLost(sessionToken) {
      if (!this.currentPeer) {
        // not a P2P session or the target is already killed.
        return;
      }

      // For peerlost, the message is delievered to the target which
      // onpeerready has been called before.
      this.notifyPeerEvent(this.currentPeer, NFC.NFC_PEER_EVENT_LOST, sessionToken);
      this.currentPeer = null;
    },

    /**
     * nsIMessageListener interface methods.
     */

    receiveMessage: function receiveMessage(message) {
      DEBUG && debug("Received message from content process: " + JSON.stringify(message));

      if (message.name == "child-process-shutdown") {
        this.removePeerTarget(message.target);
        this.nfc.removeTarget(message.target);
        return null;
      }

      if (NFC_IPC_MSG_NAMES.indexOf(message.name) != -1) {
        // Do nothing.
      } else if (NFC_IPC_READ_PERM_MSG_NAMES.indexOf(message.name) != -1) {
        if (!message.target.assertPermission("nfc-read")) {
          debug("Nfc message " + message.name +
                " from a content process with no 'nfc-read' privileges.");
          return null;
        }
      } else if (NFC_IPC_WRITE_PERM_MSG_NAMES.indexOf(message.name) != -1) {
        if (!message.target.assertPermission("nfc-write")) {
          debug("Nfc Peer message  " + message.name +
                " from a content process with no 'nfc-write' privileges.");
          return null;
        }
      } else if (NFC_IPC_MANAGER_PERM_MSG_NAMES.indexOf(message.name) != -1) {
        if (!message.target.assertPermission("nfc-manager")) {
          debug("NFC message " + message.name +
                " from a content process with no 'nfc-manager' privileges.");
          return null;
        }
      } else {
        debug("Ignoring unknown message type: " + message.name);
        return null;
      }

      switch (message.name) {
        case "NFC:CheckSessionToken":
          if (message.data.sessionToken !== this.nfc.sessionTokenMap[this.nfc._currentSessionId]) {
            debug("Received invalid Session Token: " + message.data.sessionToken +
                  ", current SessionToken: " + this.nfc.sessionTokenMap[this.nfc._currentSessionId]);
            return NFC.NFC_ERROR_BAD_SESSION_ID;
          }
          return NFC.NFC_SUCCESS;
        case "NFC:RegisterPeerReadyTarget":
          this.registerPeerReadyTarget(message.target, message.data.appId);
          return null;
        case "NFC:UnregisterPeerReadyTarget":
          this.unregisterPeerReadyTarget(message.data.appId);
          return null;
        case "NFC:CheckP2PRegistration":
          this.checkP2PRegistration(message);
          return null;
        case "NFC:NotifyUserAcceptedP2P":
          this.notifyUserAcceptedP2P(message.data.appId);
          return null;
        case "NFC:NotifySendFileStatus":
          // Upon receiving the status of sendFile operation, send the response
          // to appropriate content process.
          message.data.type = "NotifySendFileStatus";
          if (message.data.status !== NFC.NFC_SUCCESS) {
            message.data.errorMsg =
              this.nfc.getErrorMessage(NFC.NFC_GECKO_ERROR_SEND_FILE_FAILED);
          }
          this.nfc.sendNfcResponse(message.data);
          return null;
        default:
          return this.nfc.receiveMessage(message);
      }
    },

    /**
     * nsIObserver interface methods.
     */

    observe: function observe(subject, topic, data) {
      switch (topic) {
        case NFC.TOPIC_XPCOM_SHUTDOWN:
          this._shutdown();
          break;
      }
    },
  };
});

function Nfc() {
  debug("Starting Nfc Service");

  let nfcService = Cc["@mozilla.org/nfc/service;1"].getService(Ci.nsINfcService);
  if (!nfcService) {
    debug("No nfc service component available!");
    return;
  }

  nfcService.start(this);
  this.nfcService = nfcService;

  gMessageManager.init(this);

  // Maps sessionId (that are generated from nfcd) with a unique guid : 'SessionToken'
  this.sessionTokenMap = {};
  this.targetsByRequestId = {};
}

Nfc.prototype = {

  classID:   NFC_CID,
  classInfo: XPCOMUtils.generateCI({classID: NFC_CID,
                                    classDescription: "Nfc",
                                    interfaces: [Ci.nsINfcService]}),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver, Ci.nsINfcEventListener]),

  _currentSessionId: null,

  powerLevel: NFC.NFC_POWER_LEVEL_UNKNOWN,

  /**
   * Send arbitrary message to Nfc service.
   *
   * @param nfcMessageType
   *        A text message type.
   * @param message [optional]
   *        An optional message object to send.
   */
  sendToNfcService: function sendToNfcService(nfcMessageType, message) {
    message = message || {};
    message.type = nfcMessageType;
    this.nfcService.sendCommand(message);
  },

  sendNfcResponse: function sendNfcResponse(message) {
    let target = this.targetsByRequestId[message.requestId];
    if (!target) {
      debug("No target for requestId: " + message.requestId);
      return;
    }
    delete this.targetsByRequestId[message.requestId];

    target.sendAsyncMessage("NFC:" + message.type, message);
  },

  /**
   * Send Error response to content. This is used only
   * in case of discovering an error in message received from
   * content process.
   *
   * @param message
   *        An nsIMessageListener's message parameter.
   */
  sendNfcErrorResponse: function sendNfcErrorResponse(message, errorCode) {
    if (!message.target) {
      return;
    }

    let nfcMsgType = message.name + "Response";
    message.data.errorMsg = this.getErrorMessage(errorCode);
    message.target.sendAsyncMessage(nfcMsgType, message.data);
  },

  getErrorMessage: function getErrorMessage(errorCode) {
    return NFC.NFC_ERROR_MSG[errorCode] ||
           NFC.NFC_ERROR_MSG[NFC.NFC_GECKO_ERROR_GENERIC_FAILURE];
  },

  /**
   * Process the incoming message from the NFC Service.
   */
  onEvent: function onEvent(event) {
    let message = Cu.cloneInto(event, this);
    DEBUG && debug("Received message from NFC Service: " + JSON.stringify(message));

    // mapping error code to error message
    if (message.status !== undefined && message.status !== NFC.NFC_SUCCESS) {
      message.errorMsg = this.getErrorMessage(message.status);
    }

    switch (message.type) {
      case "InitializedNotification":
        // Do nothing.
        break;
      case "TechDiscoveredNotification":
        message.type = "techDiscovered";
        this._currentSessionId = message.sessionId;

        // Check if the session token already exists. If exists, continue to use the same one.
        // If not, generate a new token.
        if (!this.sessionTokenMap[this._currentSessionId]) {
          this.sessionTokenMap[this._currentSessionId] = UUIDGenerator.generateUUID().toString();
        }
        // Update the upper layers with a session token (alias)
        message.sessionToken = this.sessionTokenMap[this._currentSessionId];
        // Do not expose the actual session to the content
        delete message.sessionId;

        gSystemMessenger.broadcastMessage("nfc-manager-tech-discovered", message);
        break;
      case "TechLostNotification":
        message.type = "techLost";

        // Update the upper layers with a session token (alias)
        message.sessionToken = this.sessionTokenMap[this._currentSessionId];
        // Do not expose the actual session to the content
        delete message.sessionId;

        gSystemMessenger.broadcastMessage("nfc-manager-tech-lost", message);
        gMessageManager.onPeerLost(this.sessionTokenMap[this._currentSessionId]);

        delete this.sessionTokenMap[this._currentSessionId];
        this._currentSessionId = null;

        break;
     case "HCIEventTransactionNotification":
        this.notifyHCIEventTransaction(message);
        break;
     case "ConfigResponse":
        if (message.status === NFC.NFC_SUCCESS) {
          this.powerLevel = message.powerLevel;
        }

        this.sendNfcResponse(message);
        break;
      case "ConnectResponse": // Fall through.
      case "CloseResponse":
      case "GetDetailsNDEFResponse":
      case "ReadNDEFResponse":
      case "MakeReadOnlyNDEFResponse":
      case "WriteNDEFResponse":
        this.sendNfcResponse(message);
        break;
      default:
        throw new Error("Don't know about this message type: " + message.type);
    }
  },

  // HCI Event Transaction
  notifyHCIEventTransaction: function notifyHCIEventTransaction(message) {
    delete message.type;
    /**
     * FIXME:
     * GSMA 6.0 7.4 UI Application triggering requirements
     * This specifies the need for the following parameters to be derived and
     * sent. One unclear spec is what the URI format "secure:0" refers to, given
     * SEName can be something like "SIM1" or "SIM2".
     *
     * 1) Mime-type - Secure Element application dependent
     * 2) URI,  of the format:  nfc://secure:0/<SEName>/<AID>
     *     - SEName reflects the originating SE. It must be compliant with
     *       SIMAlliance Open Mobile APIs
     *     - AID reflects the originating UICC applet identifier
     * 3) Data - Data payload of the transaction notification, if any.
     */
    gSystemMessenger.broadcastMessage("nfc-hci-event-transaction", message);
  },

  nfcService: null,

  sessionTokenMap: null,

  targetsByRequestId: null,

  /**
   * Process a message from the gMessageManager.
   */
  receiveMessage: function receiveMessage(message) {
    let isPowerAPI = message.name == "NFC:StartPoll" ||
                     message.name == "NFC:StopPoll"  ||
                     message.name == "NFC:PowerOff";

    if (!isPowerAPI) {
      if (this.powerLevel != NFC.NFC_POWER_LEVEL_ENABLED) {
        debug("NFC is not enabled. current powerLevel:" + this.powerLevel);
        this.sendNfcErrorResponse(message, NFC.NFC_GECKO_ERROR_NOT_ENABLED);
        return null;
      }

      // Update the current sessionId before sending to the NFC service.
      message.data.sessionId = this._currentSessionId;
    }

    // Sanity check on sessionId
    let sessionToken = this.sessionTokenMap[this._currentSessionId];
    if (message.data.sessionToken && (message.data.sessionToken !== sessionToken)) {
      debug("Invalid Session Token: " + message.data.sessionToken +
            " Expected Session Token: " + sessionToken);
      this.sendNfcErrorResponse(message, NFC.NFC_ERROR_BAD_SESSION_ID);
      return null;
    }

    switch (message.name) {
      case "NFC:StartPoll":
        this.setConfig({powerLevel: NFC.NFC_POWER_LEVEL_ENABLED,
                        requestId: message.data.requestId});
        break;
      case "NFC:StopPoll":
        this.setConfig({powerLevel: NFC.NFC_POWER_LEVEL_LOW,
                        requestId: message.data.requestId});
        break;
      case "NFC:PowerOff":
        this.setConfig({powerLevel: NFC.NFC_POWER_LEVEL_DISABLED,
                        requestId: message.data.requestId});
        break;
      case "NFC:GetDetailsNDEF":
        this.sendToNfcService("getDetailsNDEF", message.data);
        break;
      case "NFC:ReadNDEF":
        this.sendToNfcService("readNDEF", message.data);
        break;
      case "NFC:WriteNDEF":
        this.sendToNfcService("writeNDEF", message.data);
        break;
      case "NFC:MakeReadOnlyNDEF":
        this.sendToNfcService("makeReadOnlyNDEF", message.data);
        break;
      case "NFC:Connect":
        this.sendToNfcService("connect", message.data);
        break;
      case "NFC:Close":
        this.sendToNfcService("close", message.data);
        break;
      case "NFC:SendFile":
        // Chrome process is the arbitrator / mediator between
        // system app (content process) that issued nfc 'sendFile' operation
        // and system app that handles the system message :
        // 'nfc-manager-send-file'. System app subsequently handover's
        // the data to alternate carrier's (BT / WiFi) 'sendFile' interface.

        // Notify system app to initiate BT send file operation
        gSystemMessenger.broadcastMessage("nfc-manager-send-file",
                                           message.data);
        break;
      default:
        debug("UnSupported : Message Name " + message.name);
        return null;
    }
    this.targetsByRequestId[message.data.requestId] = message.target;

    return null;
  },

  removeTarget: function removeTarget(target) {
    Object.keys(this.targetsByRequestId).forEach((requestId) => {
      if (this.targetsByRequestId[requestId] === target) {
        delete this.targetsByRequestId[requestId];
      }
    });
  },

  /**
   * nsIObserver interface methods.
   */
  observe: function(subject, topic, data) {
    if (topic != "profile-after-change") {
      debug("Should receive 'profile-after-change' only, received " + topic);
    }
  },

  setConfig: function setConfig(prop) {
    this.sendToNfcService("config", prop);
  },

  shutdown: function shutdown() {
    this.nfcService.shutdown();
    this.nfcService = null;
  }
};

if (NFC_ENABLED) {
  this.NSGetFactory = XPCOMUtils.generateNSGetFactory([Nfc]);
}
