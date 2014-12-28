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
#include <semaphore.h>


#define NB_THREADS 10//=nombre de poste
#define NB_CONTENEURS 50

int tableauConteneurs[NB_CONTENEURS];
int conteneursDispo = NB_CONTENEURS;

pthread_t tabThreads[NB_THREADS][2];

pthread_mutex_t attributionConteneur;
pthread_cond_t attenteConteneur;
/*
	Structure permettant de stocker les infos du poste/thread
*/
struct donneesInitThread
{
	int numeroPoste;
	int suivantNumero;
	pthread_t suivantID;
	int nbPrecedents;
	int precedentsNumero[NB_THREADS];
	pthread_t precedentsID[NB_THREADS];
	int nbPiecesAProduire;
};

struct donneesInitThread tabDonneesThread[NB_THREADS];

//Va servir pour compter les numeros de poste
int compteurNumeroPoste = 0;

/*
	Permet d'initialiser individuellement chaque poste de travail
*/
void initialisationPoste(int numero){

	int i,nb;
	printf("Combien de poste pour le poste %d ?\n",numero);
	scanf("%d",&nb);
	tabDonneesThread[numero].nbPrecedents = nb;
	tabDonneesThread[numero].numeroPoste = numero;

	//On initialise tous les postes reliés à celui-ci
	for(i=0;i<nb;i++){
		compteurNumeroPoste++;
		printf("Poste %i relié au poste %d\n",compteurNumeroPoste,numero);
		tabDonneesThread[numero].precedentsNumero[i] = compteurNumeroPoste;
		tabDonneesThread[compteurNumeroPoste].suivantNumero = numero;
	}
	//on lance l'initialisation pour tous les postes reliés
	for(i=0;i<nb;i++){
		initialisationPoste(tabDonneesThread[numero].precedentsNumero[i]);
	}

}
/*
	Lance l'initialisation de tous les postes de travail
*/
void initialisationPostes(){

	int nb,i,j;
	printf("Bienvenue dans le gestionnaire des postes de travail\n");
	sleep(1);
	printf("Nous allons commencer du dernier poste\n");
	sleep(1);
	printf("Combien de postes pour le poste final ?");
	scanf("%d",&nb);

	tabDonneesThread[0].numeroPoste = 0;
	tabDonneesThread[0].nbPrecedents = nb;

	//tabThreads[0][0] = 0;
	printf("Les postes suivants sont reliés au poste 0 : \n");
	for(i=1;i<nb+1;i++){
		printf("Poste %d\n",i);
		tabDonneesThread[0].precedentsNumero[i-1]=i;
		compteurNumeroPoste++;
	}

	sleep(1);
	printf("Bien, maintenant nous allons faire poste par poste donc ligne par ligne.\n");
	//On parcourt tous les postes du poste 0
	for(i=1;i<tabDonneesThread[0].nbPrecedents+1;i++) initialisationPoste(i);
}



void initialisationTabThread(){
	int i,j;
	for(i=0;i<NB_THREADS;i++){
		tabThreads[i][0] = i;
	}
}

int hommeFlux_attribuerConteneur(){
	pthread_mutex_lock(&attributionConteneur);

	int i,num_conteneur;

	if(conteneursDispo==0)
		pthread_cond_wait(&attenteConteneur,&attributionConteneur);

	for(i=0;i<NB_CONTENEURS;i++){
		if(tableauConteneurs[i]==0){
			num_conteneur = i;
			tableauConteneurs[i]=1;
			i=NB_CONTENEURS;
		}

	}

	conteneursDispo--;
	pthread_mutex_unlock(&attributionConteneur);
	return num_conteneur;
}


void hommeFLux_rendreConteneur(int num){
	pthread_mutex_lock(&attributionConteneur);
	tableauConteneurs[num]=0;
	conteneursDispo++;
	pthread_cond_signal(&attenteConteneur);
	pthread_mutex_unlock(&attributionConteneur);
	return;
}

void* posteTravail(void* donnees){
	//On va tenter de recuperer les données
	struct donneesInitThread *mesDonnees;
	mesDonnees = (struct donneesInitThread*) donnees;
	printf("%d\n",mesDonnees->numeroPoste);
	int num_conteneur = hommeFlux_attribuerConteneur();

		printf("Mon conteneur %d\n",num_conteneur);

		sleep(2);

		printf("Je rend mon conteneur %d\n",num_conteneur);
		hommeFLux_rendreConteneur(num_conteneur);
return;
}

int main(void){
	
	initialisationPostes();//On crée l'architecture
	initialisationTabThread();//On instancie la table des threads

	pthread_mutex_init(&attributionConteneur,NULL);
		
	int j;	
	for(j=0;j<NB_THREADS;j++){
		pthread_create(&tabThreads[j][1],0,posteTravail, &tabDonneesThread[j]);
	}
	for(j=0;j<NB_THREADS;j++){
		pthread_join(tabThreads[j][1],NULL);
	}
	/*
		Fonction pour envoyer un signal à un thread
		pthread_kill(tabThreads[2],SIGUSR1);
	*/
	
}