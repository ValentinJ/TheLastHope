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


#define NB_THREADS 10
#define NB_CONTENEURS 100

int tableauLancement[NB_THREADS][2];
int tableauConteneurs[NB_CONTENEURS];

pthread_t tabThreads[NB_THREADS];



pthread_mutex_t attributionConteneur;

void initialisationTableauLancement(){
	int i,j;
	for(i=0;i<NB_THREADS;i++){
		for(j=0;j<2;j++){
			tableauLancement[i][j]=0;
		}
	}
}

/*
Fonction décrivant l'activité d'un poste de travail
*/

int hommeFlux_attribuerConteneur(){
	pthread_mutex_lock(&attributionConteneur);

	int i,num_conteneur;
	for(i=0;i<NB_CONTENEURS;i++){
		if(tableauConteneurs[i]==0){
			num_conteneur = i;
			tableauConteneurs[i]=1;
			i=NB_CONTENEURS;
		}

	}

	pthread_mutex_unlock(&attributionConteneur);
	return num_conteneur;
}


void hommeFLux_rendreConteneur(int num){
	pthread_mutex_lock(&attributionConteneur);
	tableauConteneurs[num]=0;
	pthread_mutex_unlock(&attributionConteneur);
	return;
}

void* posteTravail(void* donnees){

	

return;
}

int main(void){
	
	pthread_mutex_init(&attributionConteneur,NULL);

	int j;	

	initialisationTableauLancement();//on initialise les stocks

	for(j=0;j<NB_THREADS;j++){
		pthread_create(&tabThreads[j],0,posteTravail,NULL);
	}
	for(j=0;j<NB_THREADS;j++){
		pthread_join(tabThreads[j],NULL);
	}
	/*
		Fonction pour envoyer un signal à un thread
		pthread_kill(tabThreads[2],SIGUSR1);
	*/
	
}