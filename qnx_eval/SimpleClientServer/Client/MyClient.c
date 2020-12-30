#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
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
	int coid, status, server_pid, server_chid;
	int incoming_checksum;
	my_msg msg;
	
	if(argc < 4) { perror("Podaj pid, chid, msg"); exit(EXIT_FAILURE); }		
	
	server_pid = atoi(argv[1]);
	server_chid = atoi(argv[2]);
	strcpy(msg.txt, argv[3]);
	msg.type = 0;
	msg.cksum = calculateChecksum(msg.txt);
	
	printf("Lacze sie z pid: %d, chid %d\n", server_pid, server_chid);
	
	coid = ConnectAttach(ND_LOCAL_NODE, server_pid, server_chid, _NTO_SIDE_CHANNEL, 0);	
	if(coid < 0) { perror("ConnectAttach"); exit(EXIT_FAILURE); }
	
	printf("\nWysylam: %s\n", msg.txt);
	
	status = MsgSend(coid, &msg, sizeof(msg), &incoming_checksum, sizeof(incoming_checksum));
	if(status < 0) { perror("MsgSend"); exit(EXIT_FAILURE); }
	
	if(msg.cksum == incoming_checksum) { printf("\n~~Corretly sent!(cksum:%d)~~\n", incoming_checksum); }
	printf("Status: %d\n", status);
	
	return EXIT_SUCCESS;
}
