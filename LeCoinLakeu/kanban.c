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

#define NB_THREADS_MAX 100//=nombre de poste
#define NB_CONTENEURS NB_THREADS_MAX

#define SIGUSR2     12//contrainte liée à l'OS

int NB_THREADS;

int tableauConteneurs[NB_CONTENEURS];
int conteneursDispo = NB_CONTENEURS;

pthread_t tabThreads[NB_THREADS_MAX][2];//fait la correspondance entre le n° du poste et son id de thread
pthread_t threadTableauDeLancement;

pthread_mutex_t attributionConteneur;
pthread_cond_t attenteConteneur;
pthread_cond_t attenteAcces;
pthread_cond_t attenteLancement;

int PiecesProduites; //Nombre de pièces totales que doit faire le poste 0
/*
	Structure permettant de stocker les infos du poste/thread
*/
struct donneesInitThread
{
	int numeroPoste;
	int suivantNumero;
	pthread_t suivantID;
	int nbPrecedents;
	int precedentsNumero[NB_THREADS_MAX];
	pthread_t precedentsID[NB_THREADS_MAX];
	int nbPiecesAProduire;
	int tempsFabrication;
	int conteneurEntrant;
	int conteneurSortant;
};
/*
	Permet d'envoyer un message au tableau de lancement pour lui indiquer 
	le poste à reveiller
*/
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
//Données de tous les threads
struct donneesInitThread tabDonneesThread[NB_THREADS_MAX];

//Va servir pour compter les numeros de poste
int compteurNumeroPoste = 0;

int fluxCarteMagnetique;//une file de message destinée au tableau de lancement

void FermetureUsine();
/*
	Permet d'initialiser individuellement chaque poste de travail
	- son numéro
	- son temps de fabrication
	- le nombre de pièces à fabriquer
	- ses données (la file de message entrante)
*/
void initialisationPoste(int numero){

	int i,nb;
	printf("Combien de postes pour le poste %d ?\n",numero);
	scanf("%d",&nb);
	if(nb+compteurNumeroPoste +1> NB_THREADS){
		printf("ERREUR : Vous avez dépassé la capacité maximale de postes\nArret\n");
		FermetureUsine();
		exit(1);
	}
	printf("Combien de temps met-il pour produire 1 pièce ?");
	scanf("%d",&tabDonneesThread[numero].tempsFabrication);
	printf("Combien de pièces doit-il produire ?");
	scanf("%d",&tabDonneesThread[numero].nbPiecesAProduire);
	tabDonneesThread[numero].nbPrecedents = nb;
	tabDonneesThread[numero].numeroPoste = numero;

	if((tabDonneesThread[numero].conteneurEntrant = msgget((key_t)numero,IPC_CREAT|IPC_EXCL|0600))<=0){
		perror("Probleme lors de la création de la file de message\n");
		exit(1);
	}
	for(i=0;i<nb;i++){
		compteurNumeroPoste++;
		printf("Poste %i relié au poste %d\n",compteurNumeroPoste,numero);
		tabDonneesThread[numero].precedentsNumero[i] = compteurNumeroPoste;
		tabDonneesThread[compteurNumeroPoste].suivantNumero = numero;
		tabDonneesThread[compteurNumeroPoste].conteneurSortant = tabDonneesThread[numero].conteneurEntrant;
	}
	//on lance l'initialisation pour tous les postes reliés
	for(i=0;i<nb;i++){
		initialisationPoste(tabDonneesThread[numero].precedentsNumero[i]);
	}

}
/*
	Lance l'initialisation de tous les postes de travail
	Appel récursif de la fonction pour initialiser un poste
	permet ainsi de créer une arborescence dynamique 
*/

void initialisationPostes(){

	int nb,i,j;
	printf("-----------------------------------------------------\n");
	printf("Bienvenue dans le gestionnaire des postes de travail\n");
	printf("-----------------------------------------------------\n");
	printf("Combien doit on recruter de poste de travail ?");
	scanf("%d",&NB_THREADS);
	if(NB_THREADS>NB_THREADS_MAX){
		printf("L'usine n'est pas assez grande\nArrêt.\n");
		exit(1);
	}
	printf("-----------------------------------------------------\n");
	printf("Combien de pièces devons nous produire ?");
	scanf("%d",&PiecesProduites);
	printf("-----------------------------------------------------\n");
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

	if((tabDonneesThread[0].conteneurEntrant = msgget((key_t)0,IPC_CREAT|IPC_EXCL|0600))<=0){
		perror("Erreur lors de la création de la file de message pour le poste 0\n");
		exit(1);
	}

	for(i=1;i<tabDonneesThread[0].nbPrecedents+1;i++){
		tabDonneesThread[i].conteneurSortant = tabDonneesThread[0].conteneurEntrant;
	}
	sleep(1);

	printf("Bien, maintenant nous allons faire poste par poste donc ligne par ligne.\n");
	//On parcourt tous les postes du poste 0
	for(i=1;i<tabDonneesThread[0].nbPrecedents+1;i++) initialisationPoste(i);
}

/*
	Initialise le tableau de données de thread
*/

void initialisationTabThread(){
	int i,j;
	for(i=0;i<NB_THREADS;i++){
		tabThreads[i][0] = i;
	}
}
/*
	Va chercher un conteneur disposible, le réserve et donne le numéro au poste qui a appelé la fonction
*/
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

/*
	A partir du numéro donné, la fonction va libérer un conteneur
*/

void hommeFLux_rendreConteneur(int num){
	pthread_mutex_lock(&attributionConteneur);
	tableauConteneurs[num]=0;
	conteneursDispo++;
	pthread_cond_signal(&attenteConteneur);
	pthread_mutex_unlock(&attributionConteneur);
	return;
}

/*
	Fonction décrivant l'activité d'un poste de travail

	- On fait correspondre les ids des threads précédents par rapport au n°
	- On crée le stock : matrice 2 colonnes
		-> 1ère colonne le n° du thread précédent
		-> 2nde colone l'état du stock vide/plein
	- On crée un message factise à envoyer au tableau de lancement pour lui indiquer que l'on est prêt

	Pour le départ :
	- si on est le poste 0 on ne se met jamais en sigsuspend
	- sinon on attent le signal du tdl (tableau de lancement)

	Boucle de travail : 
	On répète autant de fois que l'on doit fabriquer de pièces
		- on vide le stock et on envoie un signal au tdl pour lui dire de refabriquer la pièce
		- on fabrique = sleep
		- on attend le nouveau stock
	Une fois le nb de pièce attent, on envoie la pièce au poste suivant 

	Un poste ne peut mourir qu'avec le signal Ctr+C
*/
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
	sigset_t ens,attente;
	sigemptyset(&ens);
	sigprocmask(SIG_SETMASK,&ens,NULL);

	while(PiecesProduites>0){

		if(mesDonnees->numeroPoste > 0 || depart)
			{
				sigpending(&attente);
				//pause();//On attent l'ordre du tableau de lancement
				if(!sigismember(&attente,SIGUSR2)){
					//printf("Poste %d aucun signal recu j'attent\n",mesDonnees->numeroPoste);
					sigsuspend(&ens);
				}
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
				hommeFLux_rendreConteneur(msg.numeroConteneur);
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
	printf("Poste %d pièce(s) fabriquée(s) et/ou envoyée(s)\n",mesDonnees->numeroPoste);

	//Quand le dernier poste produit une pièce, c'est qu'une pièce finale est produite
	if(mesDonnees->numeroPoste == 0){
		 PiecesProduites--;
		 hommeFLux_rendreConteneur(monConteneur);
		}
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
/*
La fonction se divise en deux parties :
Partie 1 : Initialisation et lancement
	Il attend de recevoir un message via l'homme flux (fluxCarteMagnetique) 
 	de tous les threads poste de travail avant d'envoyer un signal au poste final
 	pour lui dire de commencer à produire

Partie 2 : Production
	Les threads sont parcourus et dés qu'il reçoit un message de l'homme flux en provenance d'un poste i 
	qui indique qu'il faut réveille le poste au niveau n-1 par rapport à lui, il lui envoie donc un signal

*/
void* tableauDeLancement(void* donnee){

	int i;
	struct mymsg tampon;
	//
	for(i=1;i<=NB_THREADS;i++){
		msgrcv(fluxCarteMagnetique,&tampon,sizeof(struct mymsg),i,0);
		printf("Les ouvriers du poste %d sont arrivés.\n",i-1);
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
/*
La fonction annule tous les threads créés et supprime les files de messages qu'il y a entre 2 postes( de niveaux n et n-1) 
et la file de message fluxCarteMagnetique
*/
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
//Fonction de permettant d'ignorer le signal SIGUSR2 pour le main
//Le masque ne fonctionne pas pour le main ...
void tmp(){
	
}

/*
	Association du signal SIGINT à la fonction FermetureUsine pour la fermeture des threads et files de messages
	Appel de la fonction recursive d'initialisation des postes et des threads dans le tableau tabDonneesThread
	Initialisation du tableau de lancement
	Création des threads poste de travail avec appel de la fonction posteTravail et comme paramètres la structure du poste associé au poste correspondant depuis le tableau tabDonneesThread
	Création du thread tableau de lancement avec appel de la fonction tableauLancement sans paramètres
	Attente de la fin de tous les threads
	Suppression de la file de messages fluxCarteMagnetique
*/
int main(void){

	signal(SIGUSR2,tmp);
	sigset_t ens;
	sigemptyset(&ens);
	sigaddset(&ens,SIGUSR2);
	sigprocmask(SIG_SETMASK,&ens,NULL);
	

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

	for(j=0;j<NB_THREADS;j++){
		pthread_join(tabThreads[j][1],NULL);
	}
	FermetureUsine();
	pthread_join(threadTableauDeLancement,NULL);

	msgctl(fluxCarteMagnetique,IPC_RMID,0);
}