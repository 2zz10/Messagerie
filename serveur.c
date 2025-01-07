#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define MAX_CHANNELS 10
#define MAX_HISTORY 100
#define BUFFER_SIZE 1024

typedef struct {
    char message[BUFFER_SIZE];
    time_t timestamp;
} Message;

typedef struct {
    char name[50];
    int clients[MAX_CLIENTS];
    int client_count;
    Message history[MAX_HISTORY];
    int history_count;
} Channel;

Channel channels[MAX_CHANNELS];
int channel_count = 0;
pthread_mutex_t channel_mutex = PTHREAD_MUTEX_INITIALIZER;

// Nouveau tableau pour stocker les noms d'utilisateur
char *client_names[MAX_CLIENTS];  // Tableau des noms d'utilisateur
int client_sockets[MAX_CLIENTS];  // Tableau des sockets associés

void *client_handler(void *client_socket);
void create_or_join_channel(int client_socket, const char *channel_name, const char *username);
void send_message_to_channel(const char *channel_name, const char *message, int sender_socket, const char *username);
void send_channel_history(const char *channel_name, int client_socket);

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Création du socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Erreur socket");
        exit(EXIT_FAILURE);
    }

    // Options du socket
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Erreur setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Lier le socket au port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Erreur bind");
        exit(EXIT_FAILURE);
    }

    // Écouter les connexions
    if (listen(server_fd, 3) < 0) {
        perror("Erreur listen");
        exit(EXIT_FAILURE);
    }

    printf("Serveur en attente de connexions sur le port %d...\n", PORT);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Erreur accept");
            continue;
        }

        printf("Nouvelle connexion acceptée.\n");

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, client_handler, (void *)&new_socket);
        pthread_detach(thread_id);
    }

    return 0;
}

void *client_handler(void *client_socket) {
    int sock = *(int *)client_socket;
    char buffer[BUFFER_SIZE];
    char channel_name[50];
    char username[50];

    // Demande au client son nom d'utilisateur
    send(sock, "Entrez votre nom d'utilisateur : ", 33, 0);
    int valread = read(sock, username, sizeof(username) - 1);
    if (valread <= 0) {
        close(sock);
        return NULL;
    }
    username[valread] = '\0';  // Ajouter un null-terminator sans retirer le dernier caractère

    // Enregistrer le nom d'utilisateur et le socket
    client_names[sock] = strdup(username);
    client_sockets[sock] = sock;

    // Demande au client le canal à rejoindre
    send(sock, "Entrez le nom du canal : ", 25, 0);
    valread = read(sock, channel_name, sizeof(channel_name) - 1);
    if (valread <= 0) {
        close(sock);
        return NULL;
    }
    channel_name[valread] = '\0';  // Ajouter un null-terminator sans retirer le dernier caractère

    // Créer ou rejoindre un canal
    create_or_join_channel(sock, channel_name, username);

    // Envoyer l'historique des messages
    send_channel_history(channel_name, sock);

    // Avertir que le client peut envoyer des messages
    send(sock, "Vous pouvez maintenant envoyer des messages.\n", 44, 0);

    // Boucle de réception des messages
    while ((valread = read(sock, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[valread] = '\0';  // Ajouter un null-terminator
        send_message_to_channel(channel_name, buffer, sock, username);
    }

    // Déconnexion
    close(sock);
    pthread_exit(NULL);
}


void create_or_join_channel(int client_socket, const char *channel_name, const char *username) {
    pthread_mutex_lock(&channel_mutex);

    // Recherche si le canal existe déjà
    for (int i = 0; i < channel_count; i++) {
        if (strcmp(channels[i].name, channel_name) == 0) {
            // Si le canal existe, ajoutez le client au canal
            if (channels[i].client_count < MAX_CLIENTS) {
                channels[i].clients[channels[i].client_count++] = client_socket;
                // Confirmation au client
                char welcome_message[BUFFER_SIZE];
                snprintf(welcome_message, sizeof(welcome_message), "Bienvenue dans le canal '%s'.\n", channel_name);
                send(client_socket, welcome_message, strlen(welcome_message), 0);
            }
            pthread_mutex_unlock(&channel_mutex);
            return;
        }
    }

    // Si le canal n'existe pas, créez-le
    if (channel_count < MAX_CHANNELS) {
        strcpy(channels[channel_count].name, channel_name);
        channels[channel_count].clients[0] = client_socket;
        channels[channel_count].client_count = 1;
        channels[channel_count].history_count = 0;
        channel_count++;

        // Confirmation au client
        char welcome_message[BUFFER_SIZE];
        snprintf(welcome_message, sizeof(welcome_message), "Le canal '%s' a été créé. Vous êtes le premier à le rejoindre.\n", channel_name);
        send(client_socket, welcome_message, strlen(welcome_message), 0);
    }

    pthread_mutex_unlock(&channel_mutex);
}

void send_message_to_channel(const char *channel_name, const char *message, int sender_socket, const char *username) {
    pthread_mutex_lock(&channel_mutex);

    // Parcours de tous les canaux
    for (int i = 0; i < channel_count; i++) {
        if (strcmp(channels[i].name, channel_name) == 0) {
            // Enregistrer le message dans l'historique du canal
            if (channels[i].history_count < MAX_HISTORY) {
                // Ajouter le message à l'historique
                snprintf(channels[i].history[channels[i].history_count].message, BUFFER_SIZE - 1, "%s: %s", username, message);
                channels[i].history[channels[i].history_count].message[BUFFER_SIZE - 1] = '\0'; // Assurer le null-terminator
                channels[i].history[channels[i].history_count].timestamp = time(NULL); // Stocker l'heure d'envoi
                channels[i].history_count++;
            }

            // Envoi du message à tous les autres clients du même canal
            for (int j = 0; j < channels[i].client_count; j++) {
                // Ne pas renvoyer le message à celui qui l'a envoyé
                if (channels[i].clients[j] != sender_socket) {
                    // Envoie du message avec le nom d'utilisateur
                    send(channels[i].clients[j], channels[i].history[channels[i].history_count - 1].message, strlen(channels[i].history[channels[i].history_count - 1].message), 0);
                }
            }
            break;
        }
    }

    pthread_mutex_unlock(&channel_mutex);
}

void send_channel_history(const char *channel_name, int client_socket) {
    pthread_mutex_lock(&channel_mutex);

    for (int i = 0; i < channel_count; i++) {
        if (strcmp(channels[i].name, channel_name) == 0) {
            for (int j = 0; j < channels[i].history_count; j++) {
                char formatted_message[BUFFER_SIZE];
                time_t timestamp = channels[i].history[j].timestamp;
                struct tm *tm_info = localtime(&timestamp);
                char time_str[26];
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

                snprintf(formatted_message, BUFFER_SIZE - 1, "[%s] %.900s\n", time_str, channels[i].history[j].message);
                formatted_message[BUFFER_SIZE - 1] = '\0';

                send(client_socket, formatted_message, strlen(formatted_message), 0);
            }
            break;
        }
    }

    pthread_mutex_unlock(&channel_mutex);
}
