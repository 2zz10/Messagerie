#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h> // Pour utiliser les threads

#define PORT 8080
#define BUFFER_SIZE 1024

void *receive_messages(void *sock_ptr);

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];

    // Créer le socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erreur socket");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convertir l'adresse IP en format binaire
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Adresse invalide");
        return -1;
    }

    // Se connecter au serveur
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connexion échouée");
        return -1;
    }

    // Demander le nom d'utilisateur
    printf("Entrez votre nom d'utilisateur : ");
    if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';  // Supprimer le \n s'il existe
        send(sock, buffer, strlen(buffer), 0);
    }

    // Attendre la confirmation du serveur
    int valread = read(sock, buffer, sizeof(buffer) - 1);
    if (valread > 0) {
        buffer[valread] = '\0';
        printf("%s\n", buffer);
    } else {
        perror("Erreur de lecture depuis le serveur");
        close(sock);
        return -1;
    }

    // Demander le nom du canal
    printf("Entrez le nom du canal : ");
    if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
        send(sock, buffer, strlen(buffer), 0);
    }

    // Attendre la confirmation du serveur
    valread = read(sock, buffer, sizeof(buffer) - 1);
    if (valread > 0) {
        buffer[valread] = '\0';
        printf("%s\n", buffer);
    } else {
        perror("Erreur de lecture depuis le serveur");
        close(sock);
        return -1;
    }

    // Créer un thread pour recevoir les messages en arrière-plan
    pthread_t receive_thread;
    if (pthread_create(&receive_thread, NULL, receive_messages, (void *)&sock) != 0) {
        perror("Erreur lors de la création du thread");
        close(sock);
        return -1;
    }

    // Boucle pour envoyer des messages
    while (1) {
        if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
            buffer[strcspn(buffer, "\n")] = '\0';  // Supprimer le \n
            if (strlen(buffer) > 0) {
                send(sock, buffer, strlen(buffer), 0);
            }
        }
    }

    // Fermer la connexion
    close(sock);
    return 0;
}

// Fonction pour recevoir les messages du serveur
void *receive_messages(void *sock_ptr) {
    int sock = *(int *)sock_ptr;
    char buffer[BUFFER_SIZE];

    // Lire les messages du serveur en continu
    while (1) {
        int valread = read(sock, buffer, sizeof(buffer) - 1);
        if (valread <= 0) {
            break; // Arrêter la lecture si la connexion est fermée ou en cas d'erreur
        }
        buffer[valread] = '\0';  // Terminer correctement la chaîne
        printf("%s\n", buffer);
    }

    printf("Connexion fermée ou erreur de lecture.\n");
    return NULL;
}
