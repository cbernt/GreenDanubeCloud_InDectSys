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
	int my_chid, my_pid, status, coid;
	int client_chid, client_pid;
	my_msg msg;
	my_pulse pulse;
	int n = 5;
	pulse.bounce = 0;

	my_chid = ChannelCreate(0);
	if(my_chid < 0) { perror("ChannelCreate"); exit(EXIT_FAILURE); }

	my_pid = getpid();
	printf("Server channel pid: %d, my_chid: %d\n", my_pid, my_chid);
	
	printf("Run client, then...\nWrite client pid: ");
	scanf("%d", &client_pid);
	printf("\nWrite client chid: ");
	scanf("%d", &client_chid);
	printf("\nConnecting with client... pid: %d, chid: %d\n", client_pid, client_chid);

	coid = ConnectAttach(ND_LOCAL_NODE, client_pid, client_chid, _NTO_SIDE_CHANNEL, 0);
	if(coid < 0) { perror("ConnectAttach"); exit(EXIT_FAILURE); }

	while(n--) {
		int rcvid;
		rcvid = MsgReceive(my_chid, &msg, sizeof(msg), NULL);
		if(rcvid < 0) { perror("MsgReceive error"); }
		if(rcvid == 0) {
			printf("\nOdebralem puls, odbijam pilke: %d\n", pulse.bounce+=2);
			status = MsgSendPulse(coid, PULSE_PRIORITY, PULSE_CODE, pulse.bounce);
			if(status < 0) { perror("MsgSend"); exit(EXIT_FAILURE); }
		}
	}
	
	return EXIT_SUCCESS;
}




