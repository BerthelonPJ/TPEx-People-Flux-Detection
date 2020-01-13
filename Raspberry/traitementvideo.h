#ifndef TRAITEMENTVIDEO_H
#define TRAITEMENTVIDEO_H

#include "ui_traitementvideo.h"

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgproc/types_c.h"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <QMainWindow>
#include <raspicam/raspicam_cv.h>
#include <QTimer>
#include <QDebug>
#include <QImage>
#include <QPixmap>
#include <QtGlobal>
#include <QTimer>
#include <iostream>
#include <stdio.h>
#include <opencv/cv.h>
#include <QTcpServer>
#include <QTcpSocket>
#include <string>


using namespace cv;
using namespace std;

class TraitementVideo : public QMainWindow, private Ui::TraitementVideo
{
    Q_OBJECT
    public:
    explicit TraitementVideo(QWidget *parent = 0);
    /*
         * Fonction qui configure la raspicam au lancement de l'application
         */
        void configureCamera();

        /*
         * Fonction de détection de visage.
         * Prend en entrée l'image à analyser, et en sortie renvoie l'image avec les rectangles tracés autour des visages détectés.
         *
         */
        Mat detectBody(Mat);

        /*
         *Fonction qui convertit une Matrice openCV en QImage pouvant être affichée sur une interface.
         */
        inline QImage cvMatToQImage(const cv::Mat &inMat);

        void countFlux(int);

        int getCompteur();

        void start();



    private slots:

        /*
         *Fonction qui s'occupe de capturer une image et de l'afficher dans le label associé.
         */
        void capturePicture();


        void on_pushButton_clicked();

        void newConnection();

        void readyRead();


    private:
        // Objet caméra pour utiliser la raspicam.
        raspicam::RaspiCam_Cv camera;
        // Image renvoyée par la camera.
        cv::Mat image;
        // image affichée dans l'interface de l'application.
        QImage image2;
        // Timer utilisé pour la capture vidéo (intervalle entre chque prise d'image)
        QTimer* timer;
        //
        Mat frame;

        // La base de données de la reconnaissance de visage
        CascadeClassifier body_cascade;

        String body_cascade_path = "/usr/share/opencv/haarcascades/haarcascade_mcs_upperbody.xml";

        // Rectangle de visage calculé ( ) qui définit la taille du visage détecté.
        int plusGrandRectangle=0;
        // Deux indices utilisés pour pointer vers le plus grand visage détecté dans le champ de la caméra.
        int indicePlusGrand =0;

        int abscisseLoaded =-1;

        int compteur = 0;

        bool dejaCompteurCroissant;
        bool dejaCompteurDecroissant;
        bool dejaCompte;
        // définit le centre du visage détecté ( pour le suivi de visage)
        int faceCenterX;
        int faceCenterY;
        QTcpServer *server;
        QTcpSocket *socket;
};

#endif // TRAITEMENTVIDEO_H
