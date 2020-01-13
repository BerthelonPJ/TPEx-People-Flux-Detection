#include <Arduino.h>
#include <esp_camera.h>
#include "GestionWifi.h"


GestionWifi gestionnaireWifi;

void setup() {
  Serial.begin(115200);
  delay(1000);

  gestionnaireWifi = GestionWifi("iPhone de Pierre-Jean", "88888888");

  gestionnaireWifi.connexionWifi();

  //gestionnaireWifi.launchAP();

  gestionnaireWifi.launchServer();

  //gestionnaireWifi.serverRequest();

  //gestionnaireWifi.afficheParametres();
}

void loop() {
  //gestionnaireWifi.handleHygroRequest();
  delay(100);
  // put your main code here, to run repeatedly:
}
