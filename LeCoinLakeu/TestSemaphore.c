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

/*Gros includes de porc */

#define CLE 123

sem_t *sem;

int main(){

	int value;
	sem = sem_open("/toto",O_RDWR|O_CREAT,066,0);

	if(fork() == 0){

		sem_getvalue(sem,&value);
		printf("------------------------------------------\n");
		printf("Valeur du sémaphore du fils %d\n",value);
		printf("------------------------------------------\n");
		printf("Fils entre en attente\n");
		sem_wait(sem);
		printf("Fils libéré\n");
		
	}
	else{
	printf("------------------------------------------\n");
	printf("J'attend 1s\n");
	sleep(2);

	sem_getvalue(sem,&value);
	printf("------------------------------------------\n");
	printf("Valeur de la semaphore du père %d\n",value);
	printf("------------------------------------------\n");
	printf("Je libère le fils\n");
	sem_post(sem);

	sem_getvalue(sem,&value);
	printf("------------------------------------------\n");
	printf("Valeur de la semaphore du père %d\n",value);
	printf("------------------------------------------\n");

	wait(NULL);
	sem_unlink("/toto");
	sem_destroy(sem);
	}
	return 0;



		

}
