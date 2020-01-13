/*
  Créé par Pierre-Jean Berthelon


*/

#ifndef __GESTIONCAMERA_H
#define __GESTIONCAMERA_H

#include <esp_camera.h>

//#define PWDN_GPIO_NUM
#define XCLK_GPIO_NUM 4
#define SIOD_GPIO_NUM 18
#define SIOC_GPIO_NUM 23
#define Y9_GPIO_NUM 36
#define Y8_GPIO_NUM 15
#define Y7_GPIO_NUM 12
#define Y6_GPIO_NUM 39
#define Y5_GPIO_NUM 35
#define Y4_GPIO_NUM 14
#define Y3_GPIO_NUM 13
#define Y2_GPIO_NUM 34
#define VSYNC_GPIO_NUM 5
#define HREF_GPIO_NUM 27
#define PCLK_GPIO_NUM 25

class GestionCamera {

private: // Attributs

public:

  GestionCamera();

  void configureCamera();


};
#endif
