#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
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

int calculateChecksum(char *text) {
    char *c;
    int cksum = 0;
    for (c = text; *c != NULL; c++) 
        cksum += *c;
    return cksum;
}

int countSmallLetters(char *text) {
	char *c;
	int numberOfSmall = 0;
	for (c = text; *c != NULL; c++) {
		if(islower(*c)) 
			++numberOfSmall;
	}
	return numberOfSmall;
}

int main(int argc, char *argv[]) {
	int coid, status, server_pid, server_chid;
	my_msg msg, incoming_msg;
	
	if(argc < 3) { perror("Podaj pid, chid"); exit(EXIT_FAILURE); }		
	
	server_pid = atoi(argv[1]);
	server_chid = atoi(argv[2]);
	msg.type = 0;
	msg.from = getpid();
	
	printf("Lacze sie z pid: %d, chid %d\n", server_pid, server_chid);
	
	coid = ConnectAttach(ND_LOCAL_NODE, server_pid, server_chid, _NTO_SIDE_CHANNEL, 0);	
	if(coid < 0) { perror("ConnectAttach"); exit(EXIT_FAILURE); }

	while(scanf("%s", msg.txt)) {
		msg.cksum = calculateChecksum(msg.txt);
		msg.ile = countSmallLetters(msg.txt);
		printf("\nWysylam: %s\n", msg.txt);

		status = MsgSend(coid, &msg, sizeof(msg), &incoming_msg, sizeof(incoming_msg));
		if(status < 0) { perror("MsgSend"); exit(EXIT_FAILURE); }
		
		if(msg.cksum == incoming_msg.cksum) { printf("\n~~Corretly sent!(cksum:%d)~~\n", msg.cksum); }
		
		printf("Incoming_message: %s\n", incoming_msg.txt);
	}
	
	
	
	return EXIT_SUCCESS;
}
