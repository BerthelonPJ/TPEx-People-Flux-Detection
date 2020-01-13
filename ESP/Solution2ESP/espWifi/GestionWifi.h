/*
  Créé par Pierre-Jean Berthelon

  Cette classe configure et permet d'utiliser le WiFi sur l'esp32
  Elle gère:
    - Client WiFi
    - Serveur WiFi
    - Access point
*/

#ifndef __GESTIONWIFI_H
#define __GESTIONWIFI_H
#include <WiFi.h>

class GestionWifi {

private: // Attributs

  /*
    Les deux pointeurs suivant pointents vers deux char, qui définissent les paramètres de connexion au point d'accès dans le cas d'une connexion cliente.
    Dans le cas de l'utilisation de l'esp32 en tant que AP, ces deux paramètres seront ceux de l'AP.
  */
  const char *ssid; // nom du réseau auquel se connecter.
  const char *passPhrase; // mot de passe.

  // Dans le cas où l'esp32 sert de serveur Web, on utilise ce pointeur.
  WiFiServer *m_server;

public:

  // constructeur par défaut, à utiliser dans le cas d'un usage en mode point d'accès.
  /*
    il va configurer le serveur web et initialiser le capteur pmodHYGRO.
  */
  GestionWifi();

  // constructeur à utiliser dans le cas d'une connexion cliente.
  /* Paramètres d'entrée :
    - le nom du réseau auquel se connecter.
    - le mot de passe associé à ce réseau.

  */
  GestionWifi(const char* name, const char* mdp);


  /*
    Fonction qui affiche sur la liaison série les paramètres du réseau auquel l'esp32 est connecté :
    adresse IP, Gateway, masque de sous réseau.
  */
  void afficheParametres();

  /*
    Fonction qui connecte l'esp32 sur un point d'accès.

    Elle est utilisée en premier lors d'une utilisation client (client ou serveur web)
  */
  void connexionWifi();

  /*
    Fonction qui lance le point d'accès de l'esp32.

    à utiliser quand on veut se servir de l'esp32 comme point d'accès pour un serveru web.
  */
  void launchAP();

  /*
    Fonction qui permet d'envoyer une requète sur le web ( à un serveur spécifique).

    La fonction affichera le résultat de la requète sur la liaison série.
  */
  void serverRequest();

  /*
    Fonction utilisée pour démarrer le serveur web.

    Elle affiche sur la liaison série l'adresse locale, à laquelle le client devra se connecter pour accéder au contenu.
  */
  void launchServer();

  /*
    Méthode qui gère les requètes de connexion au serveur web.
    Elle renvoie une page web au client qui la demande, affichant les valeurs de température et d'humidité lues sur le capteur pmodHYGRO.
  */
  void handleCompteurRequest(int);
};
#endif
