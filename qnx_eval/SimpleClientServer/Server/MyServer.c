#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/neutrino.h>
#include <process.h>

typedef struct my_msg {
	int type;
	char txt[256];
	int cksum;
}my_msg;

int calculateChecksum(char *text) {
    char *c;
    int cksum = 0;
    for (c = text; *c != NULL; c++) 
        cksum += *c;
    return cksum;
}

int main(int argc, char *argv[]) {
	my_msg msg;
	int chid, pid, rcvid, checksum, status;
	
	chid = ChannelCreate(0);
	if(chid < 0) { exit(EXIT_FAILURE); }

	pid = getpid();
	printf("Server's pid: %d, chid: %d\n", pid, chid);
	
	while(1) {
		rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
		if(rcvid < 0) { perror("MsgReceive error"); }

		printf("MSG txt from client: %s\n", msg.txt);
		printf("MSG type from client: %d\n", msg.type);	
		
		checksum = calculateChecksum(msg.txt);
		if(msg.cksum == checksum) { printf("\n~~Corretly rcvd!(cksum:%d)~~\n", checksum); }
		
		status = MsgReply(rcvid, EOK, &checksum, sizeof(checksum));
		if(status < 0) { perror("MsgReply error"); }
			
	}
	
	return EXIT_SUCCESS;
}




