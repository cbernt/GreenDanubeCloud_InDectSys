#ifndef _MSG_DEF_H_
#define _MSG_DEF_H_

#include <sys/iomsg.h>

#define MAX_STRING_LEN    256
#define CKSUM_MSG_TYPE _IO_MAX + 1

//layout of msg's should always defined by a struct, and ID'd by a msg type 
// number as the first member
typedef struct {
	uint16_t msg_type;
	char string_to_cksum[MAX_STRING_LEN + 1];  //ptr to string we're sending to server, to make it
} cksum_msg_t;					//easy, server assumes a max of 256 chars!!!!


//some of this extra stuff comes into play in the later exercises
typedef union {
	struct _pulse pulse;
	cksum_msg_t cksum;
} msg_buf_t;
                            
#define SERVER_NAME		"cksum_server"                            
                              

#endif //_MSG_DEF_H_
