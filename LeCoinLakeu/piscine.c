#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>

/*-------------------*/
#define NBPANIER 6
#define NBCABINE 6
#define NBPLACE 2
#define NBABONNES 10
/*-------------------*/

sem_t *panier;
sem_t *cabine;
sem_t *attente;

void prendrePanier(int num){

	printf("Le baigneur %i attend un panier\n",num);
	sem_wait(panier);
	printf("Le baigneur %i prend un panier\n",num);
	return;
}

void seChanger(int num){
	printf("Le baigneur %i attend une cabine\n",num);
	sem_wait(cabine);
	printf("Le baigneur %i a trouvé une cabine et va se changer\n",num);
	sleep(3);
	printf("Le baigneur %i a fini de se changer et sort\n",num);
	sem_post(cabine);
	return;
}

void seBaigner(int num){
	printf("Le baigneur %i se baigne ...\n",num);
	sleep(10);
	printf("Le baigneur %i a fini de se baigner...\n",num);
	return;
}

void rendrePanier(int num){
	sem_post(panier);
	printf("Le baigneur %i a rendu le panier\n",num);
	return;
}

void arriver(int num){
	printf("Le baigneur %i attend pour entrer\n",num);
	sem_wait(attente);
	printf("Le baigneur %i est entré\n",num);
	return;
}

void partir(int num){
	sem_post(attente);
	printf("Le baigneur %i s'en va !\n",num);
	return;
}


void parcourt(int num){
	arriver(num);
	prendrePanier(num);
	seChanger(num);
	seBaigner(num);
	seChanger(num);
	rendrePanier(num);
	partir(num);
	exit(0);
}

int main(int argc, char* argv[]){


	printf("Ouverture de la Piscine Municipale : \nNombre de places : %i \nNombre de cabine : %i\nNombre de panier : %i\n",NBPLACE,NBCABINE,NBPANIER);
	printf("Le nombre d'abonnés est estimé à %i\n",NBABONNES);
	printf("---------------------------------------\n\n\n");
	panier = sem_open("/panier",O_RDWR|O_CREAT,066,NBPANIER);
	cabine = sem_open("/cabine",O_RDWR|O_CREAT,066,NBCABINE);
	attente = sem_open("/attente",O_RDWR|O_CREAT,066,NBPLACE);

	int abonne = 0,j;
	int max = NBABONNES;
	pid_t pere = getpid();
	while( pere == getpid() && max >=1)
	{

		abonne++;
		max--;
		if(fork()==0) parcourt(abonne);

	}

	for( j = 0; j<abonne;j++) wait(NULL);
	sem_unlink("/panier");
	sem_unlink("/cabine");
	sem_unlink("/attente");
	sem_destroy(panier);
	sem_destroy(cabine);
	sem_destroy(attente);
	return 0;

}