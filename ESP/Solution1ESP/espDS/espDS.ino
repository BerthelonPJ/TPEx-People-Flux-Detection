#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "camera_index.h"
#include "Arduino.h"

#include "fb_gfx.h"
#include "fd_forward.h"
#include "fr_forward.h"

// Select camera model
#define CAMERA_OUR_MODEL_AI_THINKER // Correspond à notre configuration de caméra.
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
//#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h"

RTC_DATA_ATTR int compteur = 0; // Nombre de personnes présentes à un instant t dans la pièce

int bootCount = 0; // Variable utilisée pour gérer les moments où aucun mouvement n'a été détecté

int frameDetect = 0; // Nombre de frames sur lesquelles un visage a été détecté
int frameNonDetect = 0; //Nombre de frames sur lesquelles aucun visage n'a été détecté

boolean useVideo = false;

int inputPin = 33;              // pin pour le capteur IR.
int pirState = LOW;             // On démarre en supposant que le capteur est à LOW.
int val = 0;                    // Variable pour lire le statut du capteur.

static mtmn_config_t mtmn_config = {0};
static int8_t detection_enabled = 1; // On active la détection de visage.

/*
 * Fonction qui dessine les rectangles autour des visages détectés dans l'image.
 * 
 * Prend en entrée : L'image à traiter, les rectangles qui sont associés, les id de chaque visage.
 * 
 */

/*
 * Fonction qui capture une image et effectue une détection de visage sur l'image ainsi capturée.
 * 
 * Ne prend pas de paramètre d'entrée et renvoie un boolean, qui correspond à une détection ou non d'un visage (true si détecté et false sinon).
 * 
 */
static boolean startCameraServer(){

    camera_fb_t * fb = NULL; // image qui sera récupérée par le capteur. 
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];
    dl_matrix3du_t *image_matrix = NULL;
    bool detected = false;
    
    if (useVideo){ // toujours true.
      
      detected = false;
      fb = esp_camera_fb_get(); // On récupère l'image. 
      
      if (!fb) // Si une erreur lors de la capture. 
      {
          Serial.println("Camera capture failed");
      } 
      else 
      {      
          if(!detection_enabled || fb->width > 400) 
          {
              if(fb->format != PIXFORMAT_JPEG)
              {
                  bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                  esp_camera_fb_return(fb);
                  fb = NULL;
                  if(!jpeg_converted)
                  {
                      Serial.println("JPEG compression failed");
                  }
              } 
              else
              {
                  _jpg_buf_len = fb->len;
                  _jpg_buf = fb->buf;
              }
          } 
          else
          {
              image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3); // on alloue à la matrice qui sera utilisée pour la détection de visage la taille de l'image qui a été capturée.
              if (!image_matrix) // Si cela n'a pas fonctionné.
              {  
                  Serial.println("dl_matrix3du_alloc failed");  
              }
              else
              {
                  if(!fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item))
                  {
                      Serial.println("fmt2rgb888 failed");
                  }
                  else
                  {
                      box_array_t *net_boxes = NULL;
                      
                      if(detection_enabled) // Si la détection est activée.
                      {
                          net_boxes = face_detect(image_matrix, &mtmn_config); // On fait une détection de visage, avec la matrice correspondant à l'image et la configuration du capteur.
                      }
                      if (net_boxes || fb->format != PIXFORMAT_JPEG) 
                      {
                          if(net_boxes) // si on a détecté un visage
                          {
                              detected = true; // on prépare le booléen que l'on renverra ensuite ! 
                              Serial.println("On detecte un visage");
                              free(net_boxes->score);
                              free(net_boxes->box);
                              free(net_boxes->landmark);
                              free(net_boxes);
                          }
                          
                          if(!fmt2jpg(image_matrix->item, fb->width*fb->height*3, fb->width, fb->height, PIXFORMAT_RGB888, 90, &_jpg_buf, &_jpg_buf_len))
                          {
                              Serial.println("fmt2jpg failed");
                          }
                          esp_camera_fb_return(fb); // retourne le buffer pour être utilisé à nouveau
                          fb = NULL; // repasse NULL à buffer pour être sûr qu'il soit vide pour la prochaine Photo
                      }
                      else
                      {
                          _jpg_buf = fb->buf;
                          _jpg_buf_len = fb->len;
                      }
                  }
                  dl_matrix3du_free(image_matrix); // On libère la mémoire qui stockait l'image sous forme de matrice.
              }
          }
      }
      
      if(fb)
      {
          esp_camera_fb_return(fb);
          fb = NULL;
          _jpg_buf = NULL;
      }
      else if(_jpg_buf)
      {
          free(_jpg_buf);
          _jpg_buf = NULL;
      }
  }
  else
  {
    Serial.println("video non active");
  }
  return detected;  
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.setDebugOutput(true);
  Serial.println();

  frameDetect = 0; // Initialisation du nombre de frame avec visage détecté ou non au démarrage de l'esp.
  frameNonDetect = 0;

  pinMode(inputPin, INPUT); // On set le capteur IR en input

  camera_config_t config; // Configuration des pins de la caméra conformément à la doc fournie par LilyGo pour notre esp32
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_QVGA; // Format QVGA pour la capture d'image
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QVGA; // Format QVGA pour la capture d'image
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config); // Initilisationd e la caméra avec les paramètres définis ci-dessus
  if (err != ESP_OK) { // Si problème à l'initialisation
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get(); // On récupère le capteur photo
  //initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);//flip it back
    s->set_brightness(s, 1);//up the blightness just a bit
    s->set_saturation(s, -2);//lower the saturation
  }
  //drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

  mtmn_config.type = FAST;
  mtmn_config.min_face = 80;
  mtmn_config.pyramid = 0.707;
  mtmn_config.pyramid_times = 4;
  mtmn_config.p_threshold.score = 0.6;
  mtmn_config.p_threshold.nms = 0.7;
  mtmn_config.p_threshold.candidate_number = 20;
  mtmn_config.r_threshold.score = 0.7;
  mtmn_config.r_threshold.nms = 0.7;
  mtmn_config.r_threshold.candidate_number = 10;
  mtmn_config.o_threshold.score = 0.7;
  mtmn_config.o_threshold.nms = 0.7;
  mtmn_config.o_threshold.candidate_number = 1;
  
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 1); // 0=LOW, 1=HIGH // On set le réveil du Deep Sleep quand le capteur IR passe à 1
}

void loop() {
  
val = digitalRead(inputPin);  // On lit la valeur du capteur IR
  if (val == HIGH) {            // Si la valeur est haute (si on a détecté une présence)
    Serial.println("PIR HIGH");
    useVideo = true;
    boolean detectedFace = startCameraServer(); // On prend une photo et on renvoie si un visage a été détecté ou non.

    if (detectedFace){ // Si on visage est détecté
      frameDetect++; // On incrémente le nombre de frames sur lequel un visage est détecté
      if (frameDetect == 2){ // Si on détecte un visage sur 2 frames
        frameDetect = 0; // On a donc détecté un visage, une personne est entrée dans la pièce
        ++compteur; // On incrémente le compteur
        Serial.println("Face detected, +1 personn.");
        Serial.println(compteur);
        esp_deep_sleep_start();
      }
    } else { // Si aucun visage n'a été détecté, même process mais dans le cas présent on décrémente le compteur au bout de 2 frames
      frameNonDetect++;
      if (frameNonDetect == 2){
        frameNonDetect = 0;
        --compteur;
        Serial.println("Face not detected, -1 personn.");
        Serial.println(compteur);
        esp_deep_sleep_start();
      }
    }
    if (pirState == LOW) {
      pirState = HIGH;
    }
  } else { // Si aucune présence détectée
    Serial.println("PIR LOW");
    useVideo = true;
    
    ++bootCount;
    if (bootCount == 9){ // Au bout de 9 tours de boucle on reset les deux indices pour être sûr qu'en cas de détection ils valent 0.
      bootCount = 0;
      frameDetect = 0;
      frameNonDetect = 0;
      esp_deep_sleep_start();
      
    }
    if (pirState == HIGH){
      pirState = LOW;
    }
  }
}
