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
pthread_t threadTableauDeLancement;

pthread_mutex_t attributionConteneur;
pthread_cond_t attenteConteneur;
pthread_cond_t attenteAcces;
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
	int tempsFabrication;
	pthread_mutex_t mutexConteneurEntrant;
	pthread_mutex_t mutexConteneurSortant;
	int memoireSortant;
	int memoireEntrant;
};
//A changer
struct mymsg {
      long      mtype;    /* message type */
      char mtext[256]; /* message text of length MSGSZ */
};


struct donneesInitThread tabDonneesThread[NB_THREADS];

//Va servir pour compter les numeros de poste
int compteurNumeroPoste = 0;

int fluxCarteMagnetique;//une file de message destinée au tableau de lancement

/*
	Permet d'initialiser individuellement chaque poste de travail
*/
void initialisationPoste(int numero){

	int i,nb;
	printf("Combien de poste pour le poste %d ?\n",numero);
	scanf("%d",&nb);
	tabDonneesThread[numero].nbPrecedents = nb;
	tabDonneesThread[numero].numeroPoste = numero;

	//On initialise tous les postes reliés à celui-ci ainsi que le mutex
	pthread_mutex_init(&tabDonneesThread[numero].mutexConteneurEntrant,NULL);

	//On initialise les clés pour coder le segment de mémoire partagée
	tabDonneesThread[numero].memoireEntrant = shmget(IPC_PRIVATE,sizeof(int),0666);


	for(i=0;i<nb;i++){
		compteurNumeroPoste++;
		printf("Poste %i relié au poste %d\n",compteurNumeroPoste,numero);
		tabDonneesThread[numero].precedentsNumero[i] = compteurNumeroPoste;
		tabDonneesThread[compteurNumeroPoste].suivantNumero = numero;
		tabDonneesThread[compteurNumeroPoste].mutexConteneurSortant = tabDonneesThread[numero].mutexConteneurEntrant;
		tabDonneesThread[compteurNumeroPoste].memoireSortant = tabDonneesThread[numero].memoireEntrant;
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

	//On va passer le mutex & la memoire 
	pthread_mutex_init(&tabDonneesThread[0].mutexConteneurEntrant,NULL);
	tabDonneesThread[0].memoireEntrant = shmget(IPC_PRIVATE,sizeof(int),0666);
	for(i=1;i<tabDonneesThread[0].nbPrecedents+1;i++){
		tabDonneesThread[i].mutexConteneurSortant = tabDonneesThread[0].mutexConteneurEntrant;
		tabDonneesThread[i].memoireSortant = tabDonneesThread[0].memoireEntrant;

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
	//On tente de recuperer les données
	struct donneesInitThread *mesDonnees;
	mesDonnees = (struct donneesInitThread*) donnees;

	//1ère étape récupérer les pid des prédécesseurs
	sleep(1);//afin d'attendre tous les threads

	int i;
	for(i=0;i<mesDonnees->nbPrecedents;i++){
		mesDonnees->precedentsID[i] = tabThreads[mesDonnees->precedentsNumero[i]][1];
	}

	//2nde étape récupérer le pid du suivant
	if(mesDonnees->numeroPoste != 0)
		mesDonnees->suivantID = tabThreads[mesDonnees->suivantNumero][1];
		//pthread_mutex_lock(&attributionConteneur);
		
	int* in;
	int* out;
	in = (int*) shmat(mesDonnees->memoireEntrant,NULL,0);

	*in = mesDonnees->numeroPoste;
	if(mesDonnees->numeroPoste > 0){
		out = (int*) shmat(mesDonnees->memoireSortant,NULL,0);
	}
	struct mymsg message;

	strcpy(message.mtext ,  "Bonjour");
	message.mtype = mesDonnees->numeroPoste+1;

	if(msgsnd(fluxCarteMagnetique,&message,sizeof(struct mymsg),IPC_NOWAIT)==-1){
		perror("HOUSTON , WE HAVE A FUCKING PROBLEM !\n");
		msgctl(fluxCarteMagnetique,IPC_RMID,0);
		exit(1);
	}
		//pthread_mutex_unlock(&attributionConteneur);
return;
}
void initialisationTableauLancement(){
	if((fluxCarteMagnetique=msgget(123456,IPC_CREAT|IPC_EXCL|0600))==-1){
		perror("La file de message n'a pas pu être crée.\n");
		exit(1);
	}
}

void* tableauDeLancement(void* donnee){

	//ajouter le handler pour le Ctr+c
	int i;
	struct mymsg tampon;
	//while(1){
		for(i=1;i<=NB_THREADS;i++){
			if(msgrcv(fluxCarteMagnetique,&tampon,sizeof(struct mymsg),i,0)==-1){
				perror("HOUSTON, WE WILL DIE\n");
				msgctl(fluxCarteMagnetique,IPC_RMID,0);
				exit(1);
			}
			printf("J'ai recu %s\n",tampon.mtext);
		}
	//}
	return;
}

int main(void){
	
	initialisationPostes();//On crée l'architecture
	initialisationTabThread();//On instancie la table des threads
	initialisationTableauLancement();
	pthread_mutex_init(&attributionConteneur,NULL);
		
	int j;	
	for(j=0;j<NB_THREADS;j++){
		pthread_create(&tabThreads[j][1],0,posteTravail, &tabDonneesThread[j]);
	}
	pthread_create(&threadTableauDeLancement,0,tableauDeLancement,NULL);

	for(j=0;j<NB_THREADS;j++){
		pthread_join(tabThreads[j][1],NULL);
	}
	pthread_join(threadTableauDeLancement,NULL);
	/*
		Fonction pour envoyer un signal à un thread
		pthread_kill(tabThreads[2],SIGUSR1);
	*/
	
	msgctl(fluxCarteMagnetique,IPC_RMID,0);
}