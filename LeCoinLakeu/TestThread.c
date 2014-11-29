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
#include <pthread.h>

//pthread_t th1;

typedef struct {
	int x;
	char data[256];
}donnee;

void* maFonction(void* _s){

	 donnee* p = ( donnee*) _s;
	printf("je suis le thread %i\n",p->x);

	printf("Voici ce que j'ai recu : %s\n",p->data);
}

int main(void){
	int x = 1;
	int i;
	donnee tab[10];
	pthread_t threads[10];
	
	char tmp[2];

	for(i=0;i<10;i++){
		tab[i].x = i+1;
		strcpy(tab[i].data,"Je suis le thread ");
		sprintf(tmp,"%i",tab[i].x);
		strcat(tab[i].data,tmp);

	}

	printf("Initialisation terminée.\nLancement des threads\n");
	for(i=0;i<10;i++) 	pthread_create(&threads[i],0,maFonction,&tab[i]);
	for(i=0;i<10;i++)  	pthread_join(threads[i],NULL);



	return 0;
}