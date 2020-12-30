#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/neutrino.h>
#include <process.h>
#include <ctype.h>
#define SIZE 512

typedef struct my_msg {
	int type;
	int from;
	int ile;
	char txt[SIZE];
	int cksum;
}my_msg;


int countSmallLetters(char *text) {
	char *c;
	int numberOfSmall = 0;
	for (c = text; *c != NULL; c++) {
		if(islower(*c)) 
			++numberOfSmall;
	}
	return numberOfSmall;
}

int calculateChecksum(char *text) {
    char *c;
    int cksum = 0;
    for (c = text; *c != NULL; c++) 
        cksum += *c;
    return cksum;
}

void changeToBigLetters(char *txt) {
	int i = 0;
	for(i = 0; i < strlen(txt); i++) {
		txt[i] = toupper(txt[i]);
	}
}

int main(int argc, char *argv[]) {
	my_msg msg, outcoming_msg;
	int chid, pid, rcvid, status;
	
	chid = ChannelCreate(0);
	if(chid < 0) { exit(EXIT_FAILURE); }

	pid = getpid();
	printf("Server's pid: %d, chid: %d\n", pid, chid);
	
	while(1) {
		rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
		if(rcvid < 0) { perror("MsgReceive error"); }

		printf("\nMSG txt from client: %s\n", msg.txt);
		printf("MSG from from client: %d\n", msg.from);	
		printf("MSG ile from client: %d\n", msg.ile);	
		printf("MSG type from client: %d\n", msg.type);	
		printf("MSG cksum from client: %d\n", msg.cksum);	
		
		
		outcoming_msg.cksum = calculateChecksum(msg.txt);
		if(outcoming_msg.cksum == msg.cksum) { printf("\n~~Corretly rcvd!(cksum:%d)~~\n", msg.cksum); }
		
		strcpy(outcoming_msg.txt, msg.txt);
		
		if(countSmallLetters(outcoming_msg.txt)) {
			printf("before change to big: %s", outcoming_msg.txt);
			changeToBigLetters(&outcoming_msg.txt);
			printf("after change to big: %s", outcoming_msg.txt);
		}
		
		status = MsgReply(rcvid, EOK, &outcoming_msg, sizeof(outcoming_msg));
		if(status < 0) { perror("MsgReply error"); }
			
	}
	
	return EXIT_SUCCESS;
}




