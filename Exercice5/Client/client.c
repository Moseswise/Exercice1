/* un client TCP qui dit BONJOUR au serveur
*  Auteurs: emery.assogba@gmail.com
*  Abomey-Calavi 09 Juillet 2019
*/
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <unistd.h>

#define GREETING "BONJOUR SERVEUR"
int main(int argc, char *argv[])
{
    int cfd; // socket du client
    int success; // etat de la connection
    int port;
    struct sockaddr_in srv_addr; // socket addr du serveur

    if(argc!=3)
    {
        printf("USAGE: ./client <serveur_ip_addr> <serveur_port>\n");
        exit(-1);
    }

    cfd = socket(AF_INET, SOCK_STREAM, 0); // création de la socket
    if (cfd < 0)  // Une erreur s'est produite la socket n'a pas pu être créer
    {
        printf("Le SE n'a pas pu créer la socket %d\n", cfd);
        exit(-1);
    }

    port = atoi(argv[2]);

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons (port);
    inet_aton(argv[1], (struct in_addr *)&srv_addr.sin_addr.s_addr);


    success = connect(cfd, (struct sockaddr *) &srv_addr,
                      sizeof(struct sockaddr_in));
    if(success < 0)  // la connexion a echouée
    {
        printf("Impossible de se connecter au serveur %s:%d error %d\n",
               argv[1], port, success);
        exit(-1);
    }
    while(1)
    {
        send(cfd, GREETING, sizeof(GREETING), 0);
        pause(10);
    }

    /* Après avoir dit bonjour au serveur nous demandons au SE de libérer
     * la socket */
    close(cfd);
    return(0);
}

