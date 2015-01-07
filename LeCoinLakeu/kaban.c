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
#include <signal.h>
#include <sys/syscall.h>

#define NB_SIGNAUX SIGRTMIN

#define NB_THREADS 10//=nombre de poste
#define NB_CONTENEURS 50

#define SIGUSR2     12//putain de os à la con

int tableauConteneurs[NB_CONTENEURS];
int conteneursDispo = NB_CONTENEURS;

pthread_t tabThreads[NB_THREADS][2];
pthread_t threadTableauDeLancement;

pthread_mutex_t attributionConteneur;
pthread_cond_t attenteConteneur;
pthread_cond_t attenteAcces;
pthread_cond_t attenteLancement;

int PiecesProduites;
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
	int conteneurEntrant;
	int conteneurSortant;
};
//A changer
struct mymsg {
      long      mtype;    /* message type */
      char mtext[256]; /* message text of length MSGSZ */
};

//Message communicant entre les postes <==> conteneurs
struct conteneur{
	long numeroPoste;
	int nombrePieceProduite;
	char nomPiece[256];
	int numeroConteneur;
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
	printf("Combien de postes pour le poste %d ?\n",numero);
	scanf("%d",&nb);
	printf("Combien de temps met-il pour produire 1 pièce ?");
	scanf("%d",&tabDonneesThread[numero].tempsFabrication);
	printf("Combien de pièces doit-il produire ?");
	scanf("%d",&tabDonneesThread[numero].nbPiecesAProduire);
	tabDonneesThread[numero].nbPrecedents = nb;
	tabDonneesThread[numero].numeroPoste = numero;

	//On initialise tous les postes reliés à celui-ci ainsi que le mutex
	pthread_mutex_init(&tabDonneesThread[numero].mutexConteneurEntrant,NULL);

	/* PROJET ABANDONNE 
	| 	NIVEAU DE SECURITE ALPHA 254
	|	CODE SECURITE : ROUGE
	|
	|//On initialise les clés pour coder le segment de mémoire partagée
	|//tabDonneesThread[numero].memoireEntrant = shmget(IPC_PRIVATE,sizeof(int),0666);
	|
	*/
	if((tabDonneesThread[numero].conteneurEntrant = msgget((key_t)numero,IPC_CREAT|IPC_EXCL|0600))<=0){
		perror("Probleme lors de la création de la file de message\n");
		exit(1);
	}
	for(i=0;i<nb;i++){
		compteurNumeroPoste++;
		printf("Poste %i relié au poste %d\n",compteurNumeroPoste,numero);
		tabDonneesThread[numero].precedentsNumero[i] = compteurNumeroPoste;
		tabDonneesThread[compteurNumeroPoste].suivantNumero = numero;
		tabDonneesThread[compteurNumeroPoste].mutexConteneurSortant = tabDonneesThread[numero].mutexConteneurEntrant;
		tabDonneesThread[compteurNumeroPoste].conteneurSortant = tabDonneesThread[numero].conteneurEntrant;
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
	printf("Combien de pièces devons nous produire ?");
	scanf("%d",&PiecesProduites);
	printf("Début de l'initialisation de l'usine :\n");
	sleep(1);
	printf("Nous allons commencer du dernier poste\n");
	sleep(1);
	printf("Combien de postes pour le poste final ?");
	scanf("%d",&nb);
	printf("Combien de temps mets il pour produire 1 pièce ?");
	scanf("%d",&tabDonneesThread[0].tempsFabrication);	
	printf("Combien de pièces doit il produire ?");
	scanf("%d",&tabDonneesThread[0].nbPiecesAProduire);

	tabDonneesThread[0].numeroPoste = 0;
	tabDonneesThread[0].nbPrecedents = nb;

	printf("Les postes suivants sont reliés au poste 0 : \n");
	for(i=1;i<nb+1;i++){
		printf("Poste %d\n",i);
		tabDonneesThread[0].precedentsNumero[i-1]=i;
		compteurNumeroPoste++;
	}

	//On va passer le mutex 
	pthread_mutex_init(&tabDonneesThread[0].mutexConteneurEntrant,NULL);

	if((tabDonneesThread[0].conteneurEntrant = msgget((key_t)0,IPC_CREAT|IPC_EXCL|0600))<=0){
		perror("Erreur lors de la création de la file de message pour le poste 0\n");
		exit(1);
	}

	for(i=1;i<tabDonneesThread[0].nbPrecedents+1;i++){
		tabDonneesThread[i].mutexConteneurSortant = tabDonneesThread[0].mutexConteneurEntrant;
		tabDonneesThread[i].conteneurSortant = tabDonneesThread[0].conteneurEntrant;


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

	int stock[mesDonnees->nbPrecedents][2];
	
	int monConteneur;
	int piecesAProduire;
	/*
		Dans la première colonne le numéro du poste
		Dans la seconde l'état du stock dispo/indispo <-> 1/0
	*/

	//Pour l'initialisation on va donner de facon arbitraire un stock
	for(i=0;i<mesDonnees->nbPrecedents;i++){
		stock[i][1] = 1;
		stock[i][0] = mesDonnees->precedentsNumero[i];
	}


	struct mymsg msgInit;
	msgInit.mtype = mesDonnees->numeroPoste+1;
	msgsnd(fluxCarteMagnetique,&msgInit,sizeof(struct mymsg),IPC_NOWAIT);


	int depart = 1; //permet de différencier la premiere boucle aux autres

	//On rentre dans la boucle de travail

	while(1 && PiecesProduites>0){

		if(mesDonnees->numeroPoste > 0 || depart)
			{
				pause();//On attent l'ordre du tableau de lancement
				depart = 0;
			}
		//msgrcv(fluxCarteMagnetique,&msgInit,sizeof(struct mymsg),mesDonnees->numeroPoste+1,0);

		piecesAProduire = mesDonnees->nbPiecesAProduire;
		printf("Poste %d : démarrage des machines\n",mesDonnees->numeroPoste);
		//On réclame un conteneur à l'homme flux
		monConteneur = hommeFlux_attribuerConteneur();

		for(piecesAProduire;piecesAProduire>0;piecesAProduire--){
			//On vide les stocks
			for(i=0;i<mesDonnees->nbPrecedents;i++){
				stock[i][1]=0;
				struct mymsg msg;
				msg.mtype = mesDonnees->precedentsNumero[i]+1;
				strcpy(msg.mtext,"VIDE");
				msgsnd(fluxCarteMagnetique,&msg,sizeof(struct mymsg),IPC_NOWAIT);
			}
			//Si pas de précédent
			if(mesDonnees->nbPrecedents==0) sleep(1);

			//On commence à produire
			printf("Poste %d : début fabrication\n",mesDonnees->numeroPoste);
			sleep(mesDonnees->tempsFabrication);
			printf("Poste %d : fin fabrication\n",mesDonnees->numeroPoste);

			//on a fini de produire

			//On va regarder si les pièces sont arrivées !
			struct conteneur msg;
			for(i=0;i<mesDonnees->nbPrecedents;i++){
				msgrcv(mesDonnees->conteneurEntrant,&msg,sizeof(struct conteneur),mesDonnees->precedentsNumero[i]+1,0);
				stock[i][1] = 1;
			}
			//Si pas de précédent
			if(mesDonnees->nbPrecedents==0) sleep(1);

			//on a toute les pièces, on peut retravailler ou dormir !
				printf("Poste %d : Stock rempli.\n",mesDonnees->numeroPoste);
	}
	//On a fini de produire
	struct conteneur produitFini;
	produitFini.numeroPoste = mesDonnees->numeroPoste+1;
	produitFini.numeroConteneur = monConteneur;
	produitFini.nombrePieceProduite = mesDonnees->nbPiecesAProduire;
	printf("Poste %d pièce(s) fabriquée(s) et envoyée(s)\n",mesDonnees->numeroPoste);

	//Quand le dernier poste produit une pièce, c'est qu'une pièce finale est produite
	if(mesDonnees->numeroPoste == 0) PiecesProduites--;

	msgsnd(mesDonnees->conteneurSortant,&produitFini,sizeof(struct conteneur),IPC_NOWAIT);
		printf("Poste %d : arrêt des machines.\n",mesDonnees->numeroPoste);

	}

	
return;
}
void initialisationTableauLancement(){
	if((fluxCarteMagnetique=msgget(123456,IPC_CREAT|0600))==-1){
		perror("La file de message n'a pas pu être crée.\n");
		exit(1);
	}
}

void* tableauDeLancement(void* donnee){

	int i;
	struct mymsg tampon;
	//
	for(i=1;i<=NB_THREADS;i++){
		msgrcv(fluxCarteMagnetique,&tampon,sizeof(struct mymsg),i,0);
		printf("Recu de %d\n",i-1);
	}
	pthread_kill(tabThreads[0][1],SIGUSR2);


	while(1){
		for(i=1;i<=NB_THREADS;i++){
			if(msgrcv(fluxCarteMagnetique,&tampon,sizeof(struct mymsg),i,IPC_NOWAIT)>0){
				printf("Message envoyé à %d\n",i-1);
				pthread_kill(tabThreads[i-1][1],SIGUSR2);
				//msgsnd(fluxCarteMagnetique,&tampon,sizeof(struct mymsg),IPC_NOWAIT);
			}

		}
	}
	return;
}
void FermetureUsine(){
	int i;
	printf("Fermeture de l'usine ! \n");

	for(i=0;i<NB_THREADS;i++){
		pthread_cancel(tabThreads[i][1]);
	}
	
	pthread_cancel(threadTableauDeLancement);

	for(i=0;i<NB_THREADS;i++){
		if(msgctl(tabDonneesThread[i].conteneurEntrant,IPC_RMID,0)==-1 ){
			perror("Probleme avec la suppression de la file de messages\n");
			}
		printf("Poste %d , je ferme\n",i);
	}
	pthread_kill(threadTableauDeLancement,SIGINT);
	msgctl(fluxCarteMagnetique,IPC_RMID,0);
	exit(1);
}
void tmp(){
	
}
int main(void){
	signal(SIGUSR2,tmp);
	signal(SIGINT,FermetureUsine);
	initialisationPostes();//On crée l'architecture
	initialisationTabThread();//On instancie la table des threads
	initialisationTableauLancement();
	pthread_mutex_init(&attributionConteneur,NULL);
	
	int j;	

	for(j=0;j<NB_THREADS;j++){
		pthread_create(&tabThreads[j][1],0,posteTravail, &tabDonneesThread[j]);
	}
	pthread_create(&threadTableauDeLancement,0,tableauDeLancement,NULL);

	sleep(1);

	//et Dieu créa la vie
	//pthread_kill(tabThreads[0][1],SIGUSR2);

	for(j=0;j<NB_THREADS;j++){
		pthread_join(tabThreads[j][1],NULL);
	}
	FermetureUsine();
	pthread_join(threadTableauDeLancement,NULL);

	msgctl(fluxCarteMagnetique,IPC_RMID,0);
}