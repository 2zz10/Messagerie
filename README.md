# Messagerie

# Application de Chat Multicanal

Une application de chat en C permettant à plusieurs utilisateurs de se connecter à un serveur, de rejoindre ou de créer des canaux, et de communiquer en temps réel.

## Fonctionnalités

- **Serveur** :
  - Gère plusieurs clients via des threads.
  - Création et gestion de canaux avec historique des messages.
  - Diffusion des messages à tous les membres d’un canal.

- **Client** :
  - Connexion au serveur avec choix du nom d’utilisateur et du canal.
  - Envoi et réception de messages en temps réel.

## Installation

1. **Compiler** :
   - Serveur : `gcc -pthread serveur.c -o serveur`
   - Client : `gcc -pthread client.c -o client`

2. **Exécuter** :
   - Serveur : `./serveur`
   - Client : `./client`

## Utilisation

1. Lancer le serveur : `./serveur`
2. Lancer un client : `./client`, puis suivre les instructions pour :
   - Entrer un nom d'utilisateur.
   - Rejoindre ou créer un canal.
3. Envoyer et recevoir des messages.
