#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <process.h>
#include <unistd.h>

#define SIZE 100
#define PULSE_PRIORITY 10
#define PULSE_CODE 6

typedef struct {
	int type;
	char txt[256];
	int cksum;
} my_msg;

typedef struct {
	int bounce;
} my_pulse;

typedef union {
    my_pulse pulse;
    my_msg msg;
} my_union;   

int main(int argc, char *argv[]) {
	int coid, status, server_pid, server_chid;
	int my_pid, my_chid;
	my_pulse pulse;
	my_msg msg;
	int n = 5;
	pulse.bounce = 0;
	
	if(argc < 3) { perror("Write server's pid, chid"); exit(EXIT_FAILURE); }		
	
	server_pid = atoi(argv[1]);
	server_chid = atoi(argv[2]);
	
	printf("Connecting with server... %d, chid %d\n", server_pid, server_chid);
	
	coid = ConnectAttach(ND_LOCAL_NODE, server_pid, server_chid, _NTO_SIDE_CHANNEL, 0);	
	if(coid < 0) { perror("ConnectAttach"); exit(EXIT_FAILURE); }
	
	my_chid = ChannelCreate(0);
	if(my_chid < 0) { perror("ChannelCreate"); exit(EXIT_FAILURE); }

	my_pid = getpid();

	printf("Client channel pid: %d, my_chid: %d\n", my_pid, my_chid);

	printf("\nUderzam pilke: %d\n", ++pulse.bounce);
	status = MsgSendPulse(coid, PULSE_PRIORITY, PULSE_CODE, pulse.bounce);
	if(status < 0) { perror("MsgSend"); exit(EXIT_FAILURE); }
	sleep(1);
	
	while(n--) {
		int rcvid;
		sleep(1);
		rcvid = MsgReceive(my_chid, &msg, sizeof(msg), NULL);
		if(rcvid < 0) { perror("MsgReceive error"); }
		if(rcvid == 0) {
			printf("\nOdebralem puls, odbijam pilke: %d\n", pulse.bounce+=2);
			status = MsgSendPulse(coid, PULSE_PRIORITY, PULSE_CODE, NULL);
			if(status < 0) { perror("MsgSend"); exit(EXIT_FAILURE); }
		}
		sleep(1);
	}
	
	return EXIT_SUCCESS;
}
