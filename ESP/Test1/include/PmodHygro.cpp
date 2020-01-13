/*
  Créé par Pierre-Jean Berthelon

  Cette classe permet d'utiliser le module PModHYGRO sur l'esp32
*/
#include <Arduino.h>
#include <Wire.h>
#include "pmodhygro.h"

  // Constructeur par défaut
  PmodHygro::PmodHygro(){}
  // Constrcteur utilisé.
  PmodHygro::PmodHygro(int address){
    adresse = address;
    temperature = 0;// Initialisation des deux valeurs à 0 pour être sûr qu'elles ne contiennent pas des valeurs.
    humidite = 0;
  }
  //Fonction qui initialise et paramètre le capteur I2C ( ici PModHYGRO)
  void PmodHygro::initialiser(){
    Wire.begin(); // On démarre la communication I2C
    Wire.write(CONFIGURATION);//On vient écrire dans le registre de configuration
    Wire.write(0x9000);// La valeur 0x9000 signifie qu'on va venir lire sur 14 bits les valeurs de température et d'humidité.
    Wire.endTransmission();// On ferme la transmission une fois la configuration effectuée.
  }
  // Fonction qui va lire la valeur de température sur le capteur via liaison I2C
  void PmodHygro::readTemperature(){

    Wire.beginTransmission(adresse);// On démarre la communication avec le composant.
    Wire.write(REG_TEMP); // On vient demander une lecture du registre de temperature
    Wire.endTransmission();

    delay(13);//delay à ne pas oublier sinon ne fonctionne pas ! (la liaison I2C est lente)
    Wire.requestFrom(adresse, 2); // on attend de recevoir 2 octets de la part du composant.
    if (Wire.available()) {
      byte octetMSB = Wire.read();// On lit l'octet de poids fort en premier.
      byte octetLSB = Wire.read();

      temperature = ((word(octetMSB,octetLSB))/pow(2, 16))*165 - 40;// Conversion de la valeur reçue ( en mot de 16 bit puis en réel double précision)
    }
  }
  // Fonction qui va lire la valeur d'humidité sur le capteur via liaison I2C
  void PmodHygro::readHumidity(){
    Wire.beginTransmission(adresse);// On démarre la communication avec le composant.
    Wire.write(REG_HUM); // On vient demander une lecture du registre d'humidité
    Wire.endTransmission();

    delay(13);//delay à ne pas oublier sinon ne fonctionne pas ! (la liaison I2C est lente)
    Wire.requestFrom(adresse, 2); // on attend de recevoir 2 octets de la part du composant.
    if (Wire.available()) {
      byte octetMSB = Wire.read();// On lit l'octet de poids fort en premier.
      byte octetLSB = Wire.read();

      humidite = (((word(octetMSB,octetLSB)))/ pow(2, 16)) * 100.0; // concaténation des deux octets puis conversion en réel double précision.
    }
  }
  // fonction qui calcule et retourne la valeur de température lue par le capteur
  double PmodHygro::getTemp(){
    readTemperature(); // pour être sûr que la valeur renvoyée est bien celle lue à l'instant t.
    return temperature;
  }
  // fonction qui calcule et retourne la valeur de l'humidité lue par le capteur
  double PmodHygro::getHum(){
    readHumidity(); // pour être sûr que la valeur renvoyée est bien celle lue à l'instant t.
    return humidite;
  }
  // fonction qui va afficher les valeurs de capteurs dans la liaison série
  void PmodHygro::afficherValeurs(){
    Serial.print("Temperature : ");
    Serial.println(temperature);
    Serial.print("Humidite : ");
    Serial.println(humidite);
  }
