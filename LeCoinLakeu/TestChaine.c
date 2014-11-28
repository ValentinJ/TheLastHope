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
	long mtype;
	char data[256];
}message;

typedef struct{
	long id;
	int cle;
	char data[256];
}cleMemoire;


int main(void){

	pid_t pere,pid1,pid2,pid3;
	int msg,i=1;
	//key_t key = ftok(CLE, 'a');


	msg = msgget(CLE,IPC_CREAT|IPC_EXCL | 0600);
	if(msg == -1){
		perror("Probleme avec la file");
	}

	pere = getpid();
	while(pere == getpid() && i<4){
		
		if(fork()==0){


			printf("Processus %i cree\n",i);
			message m;
			strcpy(m.data,"je suis le process ");
			char tmp[2];
			sprintf(tmp,"%d",i);
			strcat(m.data,tmp);
			m.mtype = 5;

			msgsnd(msg,&m,1000,IPC_NOWAIT);
			
			int mymemory;
			mymemory = shmget(IPC_PRIVATE,sizeof(char),0666);

			char* c = (char*) shmat(mymemory,NULL,0);
			
			cleMemoire cm;
			

			if(i>1){
			cm.id = i -1;
			cm.cle = mymemory;
			strcpy(cm.data,m.data);
			printf("Je suis %i et j'envoie à %i\n",i,(int)cm.id);
			msgsnd(msg,&cm,1000,0);
			}
			if(i<3){
			msgrcv(msg,&cm,1000,i,0);
			printf("Je suis %i et j'ai recu : %s\n",i,cm.data);
			}
			shmdt(c);
			shmctl(mymemory,IPC_RMID,NULL);

			printf("JE MEURS %i\n",i);
			exit(0);
		}
		i++;
	}
		message m;

		for(i=1;i<4;i++) wait(NULL);
		for(i=1;i<4;i++){
			//printf("Je lis %i\n",i);
			msgrcv(msg,&m,1000,5,0);
			printf("%s\n",m.data);
			//printf("J'ai lu %i\n",i);
		}

		msgctl(msg,IPC_RMID,0);
		printf("Je m'en vais, bisous\n");
		return 0;

}