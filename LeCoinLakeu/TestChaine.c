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
	pid_t pid = fork();
	printf("%i",pid);
	if(pid == 0){
		printf("test");
		sem_getvalue(sem,&value);
		printf("%d\n",value);
		printf("Fils entre en attente\n");
		sem_wait(sem);
		printf("Fils libéré\n");
		exit(0);
	}
	else{
	printf("J'attend 1s\n");
	sleep(1);

	printf("J'affiche la valeur de la semaphore\n");

	sem_getvalue(sem,&value);
	printf("%d\n",value);

	printf("Je libère le fils\n");
	sem_post(sem);

	sem_getvalue(sem,&value);
	printf("%d\n",value);

	wait(NULL);
	sem_unlink("/toto");
	sem_destroy(sem);
	}
	return 0;



		

}
