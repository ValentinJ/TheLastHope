#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <sys/msg.h>
#include <string.h>

/*Gros includes de porc */

#define CLE 123456

typedef struct{
	char data[256];
	long pid;
}message;

int main(void){

	pid_t pere,pid1,pid2,pid3;
	int msg,i=0;
	//key_t key = ftok(CLE, 'a');


	msg = msgget(CLE,IPC_CREAT|IPC_EXCL | 0600);
	if(msg == -1){
		perror("Probleme avec la file");
	}

	pere = getpid();
	while(pere == getpid() && i<3){
		
		if(fork()==0){
			printf("Processus %i cree\n",i);
			message m;
			strcpy(m.data,"je suis le process ");
			char tmp[2];
			sprintf(tmp,"%d",i);
			strcat(m.data,tmp);
			m.pid = getpid();
			msgsnd(msg,&m,1000,0);
			printf("JE MEURS %i\n",i);
			exit(0);
		}
		i++;
	}
	message m;

	for(i=0;i<3;i++) wait(NULL);
	for(i=0;i<3;i++){
		msgrcv(msg,&m,1000,0,IPC_NOWAIT);
		printf("%s\n",m.data);
	}

	msgctl(msg,IPC_RMID,0);

	return 0;



}