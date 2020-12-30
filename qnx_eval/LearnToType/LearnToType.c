/* Autor: Konrad Bysiek */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <process.h>
#include <errno.h>
#define PULSE_CODE_BAD 5 //timer stop
#define PULSE_CODE_GOOD 6 //good typed
#define PULSE_PRIORITY 10

int N = 1;

typedef struct my_msg {
	int pid;
	int chid;
	int coid;
	char *str;
} my_msg;

typedef union {
    struct _pulse pulse;
} my_message_t;

void * playGame(void *argv){
	int status;
	my_msg* msg_arg = (my_msg*)argv;
	char * input = malloc(sizeof(char) * N + 1);
	
	fprintf(stderr, "\n\n~~~~~~~~~~LEVEL-%d~~~~~~~~~~~\n\n", N);
	
	while(1){
		fprintf(stderr, "Podaj lancuch: %s\n", msg_arg->str);
		
		scanf("%s", input);
		
		if(strcmp(input, msg_arg->str) == 0) {
			status = MsgSendPulse(msg_arg->coid, PULSE_PRIORITY, PULSE_CODE_GOOD, getpid());
    		if(status < 0) { perror("MsgSendPulse error "); exit(EXIT_FAILURE); }
			
			break;
		}
	}

	return EXIT_SUCCESS;
}

char * stringGenerator(int n){
	char c;
	char * str = malloc(sizeof(char) * n + 1);
	srand(time(NULL));
	
	while(n--) {
		c = (char)((rand() % 24) + 97);
		str[n] = c;
	}
	
	return str;
}

timer_t timerId;
struct itimerspec itime;

int main(int argc, char *argv[]) {
	
	int tid, rcvid;
	my_msg msg;
	my_message_t incMsg;
	struct sigevent event;
	
	
	msg.chid = ChannelCreate(0);
	if(msg.chid < 0) { perror("ChannelCreate error"); exit(EXIT_FAILURE); }
	
	msg.pid = getpid();
	msg.str = malloc(sizeof(char) * N + 1);
		
	msg.coid = ConnectAttach(ND_LOCAL_NODE, msg.pid, msg.chid, _NTO_SIDE_CHANNEL, 0);
	if(msg.coid < 0) { perror("ConnectAttach error "); exit(EXIT_FAILURE); }
	
	event.sigev_notify = SIGEV_PULSE; //timer wysyla pulsy
	event.sigev_coid = msg.coid;
	event.sigev_priority = getprio(0);
	event.sigev_code = PULSE_CODE_BAD;
	
	timer_create(CLOCK_REALTIME, &event, &timerId);
	
	itime.it_interval.tv_sec = 5; //co 4 sekundy interwal
	itime.it_interval.tv_nsec = 0;
	itime.it_value.tv_sec = 5; //cztery sekundy startowe
	itime.it_value.tv_nsec = 0;
	
	
	while(1) {
		timer_settime(timerId, 0, &itime, NULL);
		strcpy(msg.str, stringGenerator(N));
		pthread_create(&tid, NULL, playGame, (void *)&msg);
		
		rcvid = MsgReceive(msg.chid, &incMsg, sizeof(incMsg), NULL);
		if(rcvid == 0) {
			if(incMsg.pulse.code == PULSE_CODE_GOOD) {
				fprintf(stderr, "Poprawnie wpisales, gratuluje.");
				N++;
			}
			if(incMsg.pulse.code == PULSE_CODE_BAD) {
				fprintf(stderr, "Braklo czasu, sprobuj jeszcze raz.");
			}
			pthread_cancel(tid);
		}
	}
	
	return EXIT_SUCCESS;
}
