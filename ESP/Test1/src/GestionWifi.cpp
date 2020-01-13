/*
  Créé par Pierre-Jean Berthelon

  Cette classe configure et permet d'utiliser le WiFi sur l'esp32
  Elle gère:
    - Client WiFi
    - Serveur WiFi
    - Access point
*/
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include "GestionWifi.h"
#include "GestionCamera.h"


const int LED = 5; // led utilisée pour signifier de l'état du WiFi.

const String pageHTMLHead = "HTTP/1.1 200 OK \n\
Content-type:text/html\n\
\n\
<DOCTYPE HTML>\n\
<HTML>\n\
<HEAD>\n\
<TITLE>Données des capteurs</TITLE>\n\
</HEAD>\n\
<BODY>\n\
<h1 style=\"color: #5e9ca0;\">Donn&eacute;es des capteurs:</h1>\n\
<p>&nbsp;</p>\n\
<p><strong>&nbsp;Temperature :&nbsp;</strong></p>\n\
<p>"; // début de la page HTML ( utilisé pour être recomposé lors de la réponse à une connexion cliente au serveur web.)

const String pageHTMLMiddle = "&deg;C</p>\n\
<p><strong>Humidit&eacute; :&nbsp;</strong></p>\n\
<p>"; // suite de la page HTML ( entre les deux valeurs ( température et humidité )).

const String pageHMTLEnd = "%</p>\n\
</BODY>\n\
</HTML>\n";// footer de la page html.

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { text-align:center; }
    .vert { margin-bottom: 10%; }
    .hori{ margin-bottom: 0%; }
  </style>
</head>
<body>
  <div id="container">
    <h2>ESP32-CAM Last Photo</h2>
    <p>It might take more than 5 seconds to capture a photo.</p>
    <p>
      <button onclick="rotatePhoto();">ROTATE</button>
      <button onclick="capturePhoto()">CAPTURE PHOTO</button>
      <button onclick="location.reload();">REFRESH PAGE</button>
    </p>
  </div>
  <div><img src="saved-photo" id="photo" width="70%"></div>
</body>
<script>
  var deg = 0;
  function capturePhoto() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', "/capture", true);
    xhr.send();
  }
  function rotatePhoto() {
    var img = document.getElementById("photo");
    deg += 90;
    if(isOdd(deg/90)){ document.getElementById("container").className = "vert"; }
    else{ document.getElementById("container").className = "hori"; }
    img.style.transform = "rotate(" + deg + "deg)";
  }
  function isOdd(n) { return Math.abs(n % 2) == 1; }
</script>
</html>)rawliteral";


// constructeur par défaut.
GestionWifi::GestionWifi(){}

// constructeur à utiliser pour une connexion cliente ou en tant que AP.
GestionWifi::GestionWifi( const char *name, const char *mdp){
  ssid = name;
  passPhrase = mdp;
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);// on eteint la led de status.
  m_server = new WiFiServer(80);// on instantie un objet serveur wifi sur le port 80
  //m_hygro = new PmodHygro(0x40);// On instantie un objet pmodHygro.
  //m_hygro->initialiser();// On initialise le capteur pmodHYgro pour le lire plus tard.
}

// Fonction qui affiche les paramètres de la connexion Wifi sur la liaison série.
void GestionWifi::afficheParametres(){
  byte adrMAC[6];
  IPAddress ip,gw,masque;

  WiFi.macAddress(adrMAC);// on récupère puis on affiche l'adresse MAC de la carte WiFi.
  for(int i = 0; i < 6; i++){
    Serial.print(adrMAC[i], HEX);
    Serial.print(":");
  }
  Serial.println();
  Serial.print("ip : ");
  ip = WiFi.localIP(); // On affiche notre IP locale sur la liaison série.
  Serial.println(ip);
  Serial.print("gw : ");
  gw = WiFi.gatewayIP(); // On affiche l'adresse IP de la passerelle sur la liaison série
  Serial.println(gw);
  Serial.print("mask : ");
  masque = WiFi.subnetMask(); //Puis on finit par afficher le masque de sous réseau sur la liaison série.
  Serial.println(masque);
}

// Fonction qui connecte l'esp32 au réseau choisi dans le constructeur.
void GestionWifi::connexionWifi(){
  int etatled=0;
  Serial.print("Connect to ");
  Serial.println(ssid);
  WiFi.begin(ssid, passPhrase); // On demande la connexion au réseau ssid et de mot de passe passphrase.
  delay(100);

  while(WiFi.status() != WL_CONNECTED){ // Tant qu'on est pas connecté
    Serial.print(".");
    delay(1000);
    etatled ^= 1; // On fait blinker la led et on affiche un point dans la liaison série pour notifier l'utilisateur qu'on tente de se connecter à un réseau.
    digitalWrite(LED, etatled);
  }

  Serial.println("");
  Serial.println("WiFi Connected."); // une fois connecté au réseau on l'écrit sur la liaison série et on affiche l'IP locale.
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// Fonction utilisée pour lancer le serveur Web dans le cas d'une utilisation en serveur.
void GestionWifi::launchServer(){
  m_server->begin(); // démarrage du serveur.
  Serial.println("Server launched...");
  Serial.println("Waiting for connection...");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); // On notifie l'utilisateur puis on affiche l'adresse WiFi sur la liaison série.
}

// Fonction utilisée dans le cas d'un usage de l'esp32 en mode AP. Elle permet de lancer l'AP.
void GestionWifi::launchAP(){
  Serial.print("Setting soft-AP ... ");
  boolean result = WiFi.softAP(ssid, passPhrase); // paramètres de l'AP.
  // ssid et passphrase sont ceux spécifiés par l'utilisateur à l'instantiation de cette classe.
  if(result == true) // Si la création de l'AP réussi.
  {
    Serial.println("Ready");
  }
  else // sinon on notifie l'utilisateur de l'échec.
  {
    Serial.println("Failed!");
  }
  IPAddress IP = WiFi.softAPIP(); // On affiche l'adresse de l'access point sur le port série.
  Serial.print("AP IP address: ");
  Serial.println(IP);

}

// fonction qui gère les connexions au serveur web qui permet de lire les valeurs de PmodHYGRO.
void GestionWifi::handleHygroRequest(){
  WiFiClient client = m_server->available(); // on attend une connexion au serveur
  if (client){ // Si un client souhaite se connecter.
    Serial.println("Nouveau client");
    String lines = "";

    while (client.connected()){ // Si le client est connecté
      if (client.available()){  // et qu'il est disponible.
        char c = client.read(); // On lit la requète du client.
        lines +=c;
        if (lines.endsWith("\r\n\r\n")){ // Si le client veut le contenu de la page WEB.
          Serial.println(lines);
          client.println(pageHTMLHead /*+ m_hygro->getTemp()*/ + pageHTMLMiddle /*+ m_hygro->getHum() */+ pageHMTLEnd); // On concatène les différentes parties de la page web puis on l'envoie en entier au client.
          client.stop(); // puis on stoppe la connexion.
        }
      }
    }
    Serial.println("Client Disconnected.");
  }
}

// Requete en mode client à un server web.
void GestionWifi::serverRequest(){

  // J'ai pris pour cette fonction qui n'était pas demandé une requète google.
  // cette fonction recherche donc "arduino" sur google France
  WiFiClient client;
  IPAddress server(216,58,210,227);  // Google.fr

  Serial.println("Connected to wifi");
  Serial.println("\nStarting connection...");
  // if you get a connection, report back via serial:
  if (client.connect(server, 80)) { // On se connecte en tant que client sur le serveur.
    Serial.println("connected");
    // Make a HTTP request:
    Serial.println("Envoi d'une requete get");
    client.print("GET /search?q=arduino HTTP/1.0 \r\n\r\n");// on envoie la requete HTTP.
    Serial.println("Reponse:");
    while (client.connected() || client.available()){
      if (client.available()){
        String line = client.readStringUntil('\n'); // On vient lire la réponse à la requète et on l'affiche sur la liaison série.
        Serial.println(line);
      }
    }
    client.stop(); // On se déconnecte.
    Serial.println("Deconnecte");
  }
  else { // Si on a pas réussi à se connecter au serveur.
    Serial.println("connexion echouee");
    client.stop();
  }
}
