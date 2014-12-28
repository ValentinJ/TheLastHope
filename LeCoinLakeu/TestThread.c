#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <sys/msg.h>
#include <pthread.h>
#include <string.h>

//pthread_t th1;
typedef struct{
	long id;
	char data[256];
}donneeMemoire;

typedef struct {
	int x;
	char data[256];
	int fd;
	donneeMemoire dm;
	int mymem1;
	int mymem2;
	int tomem1;
	int tomem2;
}donnee;

typedef struct{
	long id;
	char data[256];
}donneeMessage;


void* maFonction(void* _s){

	 donnee* p = ( donnee*) _s;
	 donneeMessage dm;
	 strcpy(dm.data, p->data);
	 dm.id = p->x+1;
	 msgsnd(p->fd,&dm,1000,IPC_NOWAIT);
	printf("T%i : Voici ce que j'ai recu : %s\n",p->x,p->data);
	donneeMemoire* dmem;
	//on attache la sequence de données
	dmem = (donneeMemoire* ) shmat(p->mymem1,NULL,0);

	strcpy(dmem->data,dm.data);
	//on détache
	shmdt(dmem);
	if(p->x < 4){
		dmem = (donneeMemoire* ) shmat(p->tomem2,NULL,0);
		strcpy(dmem->data,dm.data);
		shmdt(dmem);
	}


}

int main(void){



	int fileMessage;
	fileMessage = msgget(123456,IPC_CREAT|0600);
	if (fileMessage == -1)
		perror("Problème avec la file de message");


	int i;
	int memoires[10];
	donnee tab[5];
	pthread_t threads[5];
	
	char tmp[256];
	//création des memoires partagées
	for(i=0;i<5;i++){
		memoires[i]= shmget(IPC_PRIVATE,sizeof(donneeMemoire),0666);
		if(memoires[i]==-1) perror("Probeme de memoire");
	}
	//Préparation des données pour les threads
	for(i=0;i<5;i++){
		tab[i].x = i;
		strcpy(tab[i].data,"Je suis le thread ");
		sprintf(tmp,"%i",tab[i].x);
		strcat(tab[i].data,tmp);
		tab[i].fd = fileMessage;
		//On attribue ses 2 containeurs
		tab[i].mymem1 = memoires[i*2];
		tab[i].mymem2 = memoires[i*2+1];
		//on attribue les containeurs du process suivant
		/* CECI EST UN CAS D'INSTANCIATION LINEAIRE*/
		if(i<9){
			tab[i].tomem1 = memoires[(i+1)*2];
			tab[i].tomem2 = memoires[(i+1)*2 + 1];
		}

	}

	printf("\n\nInitialisation terminée.\n\nLancement des threads\n");
	for(i=0;i<5;i++) 	pthread_create(&threads[i],0,maFonction,&tab[i]);
	
	donneeMessage dm;

	for(i=0;i<5;i++)  	pthread_join(threads[i],NULL);

	for(i=0;i<5;i++){
		msgrcv(fileMessage,&dm,1000,i+1,0);
		printf("Père : j'ai lu : %s\n",dm.data);
	}


	donneeMemoire* dmem;

	for(i=0;i<10;i++){
		dmem = (donneeMemoire*) shmat(memoires[i],NULL,0);
		printf("Père a lu dans la mémoire %i : %s",i,dmem->data);
		shmdt(dmem);
	}
	for(i=0;i<50;i++){
		shmctl(memoires[i],IPC_RMID,NULL);
	}
	msgctl(fileMessage,IPC_RMID,0);

	return 0;
}