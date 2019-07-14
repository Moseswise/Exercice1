/* un serveur TCP qui reçoit le Bonjour du client
*  
*/
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h> 
#include <sys/shm.h>  
#include <sys/sem.h>
#include <errno.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>

#define MY_ADDR "127.0.0.1"
#define MY_PORT 56789
#define LISTEN_BACKLOG 50
#define ITER 3
void proc();

union semun {
   int  val;    /* Value for SETVAL */
   struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
   unsigned short  *array;  /* Array for GETALL, SETALL */
   struct seminfo  *__buf;  /* Buffer for IPC_INF
                              (Linux-specific) */
};
void traite()
{
    pid_t pid1, pid2;
    int status; // le status des fils
    union semun sem_arg; // paramètre pour configurer le semaphore
    // clé pour le semaphore 
    key_t sem_key = ftok("semfile",75);
    // on demande au system de nous créer le semaphore
    int semid = semget(sem_key, 1, 0666|IPC_CREAT);

    // la valeur du semaphore est initialisée à 1
    sem_arg.val = 1;
    if(semctl(semid, 0, SETVAL, sem_arg)==-1){
       perror("semctl");
       exit(1);
    }
    
    // clé pour la mémoire protégée
    key_t key = ftok("shmfile",65);

    int a = 0; // shared data (la variable partagée)

    // On demande au system de creer la memoire partagee 
    int shmid = shmget(key,1024,0666|IPC_CREAT); 
    // on attache la memoire partagee a str
    char *str = (char*) shmat(shmid,(void*)0,0); 
    // ecriture de 0 dans la mémoire partagée
    sprintf(str, "%d", a);
    pid1 = fork();
    if(pid1 < 0){
          perror("Erreur de création du processus\n");
          exit(EXIT_FAILURE);
    }

    if(pid1 == 0){
       proc();
    }
    else{
        pid2 = fork();
        sleep(3);
        if(pid2 < 0){
          perror("Erreur de création du second processus\n");
          pid1 = wait(&status);
          exit(EXIT_FAILURE);
        }

        if(pid2 == 0){
           proc();
        }

        else{
                // On attend la fin des deux processus
                pid1 = wait(&status);
                printf("Status de l'arret du fils %d %d\n", status, pid1);
                pid2 = wait(&status);
                printf("Status de l'arret du fils %d %d\n", status, pid2);

                // on lit la dernière valeur de la variable partagée
              
                }   
        }
               a = atoi(str);
                printf("Valeur Finale de a = %d\n", a);
                //le processus détache str de la mémoire partagée 
                shmdt(str); 
                // destruction de la mémoire 
                shmctl(shmid,IPC_RMID,NULL);
                /*des truction du semaphore
                if(semctl(semid, 0, IPC_RMID, NULL) == -1){
                   perror("semctl");
                   exit(1);
                   }*/ 
} 

void proc()
{
    // on genere la clé de la mémoire partagée
  key_t key = ftok("shmfile",65); 
  int i;
  int a = 0; // shared data
  struct sembuf sb = {0, 0, 0}; 
  union semun sem_arg;

  // clé pour le semaphore 
  key_t sem_key = ftok("semfile",75);
  // on demande au system de nour créer le semaphore
  int semid = semget(sem_key, 1, 0666);
    

  // shmget returns an identifier in shmid 
  int shmid = shmget(key,1024,0666); 
  // shmat to attach to shared memory 
  char *str = (char*) shmat(shmid,(void*)0,0); 


  for(i=0; i < ITER; i++){
     // tentative de verrou du semaphore
     sb.sem_op = -1;
     if(semop(semid, &sb,1) == -1){
       perror("sempo");
       exit(1);
     }
     // lecture de a
     a = atoi(str);
     printf("Proc %d %d\n", getpid(), a);
     // modification de a
     a = a + 1;
     // ecriture de a
     sprintf(str, "%d", a);
     sleep(2);
     // déverrouillage du semaphore
     sb.sem_op = 1;
     if(semop(semid, &sb,1) == -1){
       perror("sempo");
       exit(1);
     } 
  } 
  
}


int main(int argc, char *argv[])
{
    int sfd; // socket du serveur
    int cfd; // socket du client
    struct sockaddr_in my_addr; // socket addr du serveur
    struct sockaddr_in peer_addr; // socket addr d'un client

    socklen_t peer_addr_size; // la taille du sock
    pid_t child;

    char buffer[20]; // pour recevoir la salutation du client

    sfd = socket(AF_INET, SOCK_STREAM, 0); // création de la socket
    if (sfd < 0)  // Une erreur s'est produite la socket n'a pas pu être créer
    {
        printf("Le SE n'a pas pu créer la socket %d\n", sfd);
        exit(-1);
    }
	// traite();
    /* La structure est remise à 0 car elle doit servir à stocker
     * l'identité du serveur*/
    memset(&my_addr, 0, sizeof(struct sockaddr_in));

    /* donne une identite a la socket. */
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons (MY_PORT);
    inet_aton(MY_ADDR, (struct in_addr *)&my_addr.sin_addr.s_addr);

    /* on demande au SE de confirmer l'identité de la socket
     * Cela autorise le SE d'exploitation à forwarder les requête
     * Sur le port 56789 à ce processus */
    if (bind(sfd, (struct sockaddr *) &my_addr,
             sizeof(struct sockaddr_in)) < 0)
    {
        printf("bind error\n"); // l'association a echouée
        exit(-1);
    }

    /* on demande au SE de définir le nombre de clients
     * que le serveur peut
     * traiter en même temps dans sa file d'attente */
    if (listen(sfd, LISTEN_BACKLOG) < -1)
        perror("listen\n");

    // on se bloque en attendant les connexion des client
    peer_addr_size = sizeof(struct sockaddr_in);
    while(1)
    {
        cfd = accept(sfd, (struct sockaddr *) &peer_addr,
                     &peer_addr_size);
        if (cfd < 0)
        {
            perror("accept\n");
            exit(-1); // Une erreur s'est produite
        }
        /* Nous créons un fils pour gérer ce client */
        child = fork();
        if(child < 0)  // le fils n'a pas pu être créé
        {
            perror("errreur de création du fils\n");
        }
        if(child==0)
        {
            /* Nous sommes dans le fils nous attendons la requête du client */
            printf("indentité du client %d\n", peer_addr.sin_port);
            /*Lecture des donnees envoyées par le client*/
            while(1)
            {
            	read(cfd, buffer, 20);
                printf("%s", buffer);
                printf("\n");
                traite();
                sleep(5);
            	printf("\n");
            	
            }
               
        }
        /*Fin du traitement le fils se termine*/
        close(sfd);
        break;
    }    
        /*Dans le père: le père attent un autre client*/
        close(cfd);
    
}

