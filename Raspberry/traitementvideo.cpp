#include "traitementvideo.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgproc/types_c.h"

TraitementVideo::TraitementVideo(QWidget *parent) :
    QMainWindow(parent)
{
    setupUi(this);

    configureCamera();

    body_cascade.load(body_cascade_path);

    server = new QTcpServer(this);
    socket = new QTcpSocket(this);
       connect(server,SIGNAL(newConnection()), this, SLOT(newConnection()));
       if(!server->listen(QHostAddress::Any,8080))
       {
           qDebug() << "Server ould not start";
       }
                       else {
                       qDebug() << "Server started";
       }

}
/*
 * Fonction qui configure la raspicam au lancement de l'application
 */
void TraitementVideo::configureCamera(){

    camera.set( CV_CAP_PROP_FORMAT, CV_8UC1 ); // format de la prise de photo ( ici en noir et blanc ).
    camera.set(CV_CAP_PROP_FRAME_WIDTH, 640); // format VGA : largeur de 640 pixels
    camera.set(CV_CAP_PROP_FRAME_HEIGHT, 480); // format VGA : hauteur de 480 pixels
}

/*
 *Fonction qui convertit une Matrice openCV en QImage pouvant être affichée sur une interface.
 */
inline QImage TraitementVideo::cvMatToQImage(const cv::Mat &inMat)
   {
      switch ( inMat.type() )
      {
         // 8-bit, 4 channel
         case CV_8UC4:
         {
            QImage image( inMat.data,
                          inMat.cols, inMat.rows,
                          static_cast<int>(inMat.step),
                          QImage::Format_ARGB32 );

            return image;
         }

         // 8-bit, 3 channel
         case CV_8UC3:
         {
            QImage image( inMat.data,
                          inMat.cols, inMat.rows,
                          static_cast<int>(inMat.step),
                          QImage::Format_RGB888 );

            return image.rgbSwapped();
         }

         // 8-bit, 1 channel
          // C'est celui q'on utilisera car notre image est prise en noir et blanc ( en 8UC1)
         case CV_8UC1:
         {
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
            QImage image( inMat.data,
                          inMat.cols, inMat.rows,
                          static_cast<int>(inMat.step),
                          QImage::Format_Grayscale8 );
#else
            static QVector<QRgb>  sColorTable;

            // only create our color table the first time
            if ( sColorTable.isEmpty() )
            {
               sColorTable.resize( 256 );

               for ( int i = 0; i < 256; ++i )
               {
                  sColorTable[i] = qRgb( i, i, i );
               }
            }

            QImage image( inMat.data,
                          inMat.cols, inMat.rows,
                          static_cast<int>(inMat.step),
                          QImage::Format_Indexed8 );

            image.setColorTable( sColorTable );
#endif

            return image;
         }

         default:
            qWarning() << "ASM::cvMatToQImage() - cv::Mat image type not handled in switch:" << inMat.type();
            break;
      }
      return QImage();
   }

/*
 * Fonction de détection de visage.
 * Prend en entrée l'image à analyser, et en sortie renvoie l'image avec les rectangles tracés autour des visages détectés.
 *
 */
void TraitementVideo::countFlux(int abscisse){
    if(abscisseLoaded==-1){
        abscisseLoaded=abscisse;
        if ( abscisse < 200 || abscisse > 440){
            dejaCompteurCroissant = false;
            dejaCompteurDecroissant = false;
            dejaCompte = false;
        } else {
            dejaCompte = true;
        }
    }else if(abscisse>abscisseLoaded){
        if (dejaCompteurDecroissant){
            compteur++;
            dejaCompteurCroissant = true;
            dejaCompteurDecroissant = false;
        }
        if(!dejaCompte){
            compteur++;
            dejaCompte = true;
            dejaCompteurCroissant = true;
        }
        abscisseLoaded=abscisse;
    }else if(abscisse<abscisseLoaded){
        if (dejaCompteurCroissant){
            compteur--;
            dejaCompteurCroissant = false;
            dejaCompteurDecroissant = true;
        }
        if(!dejaCompte){
            compteur--;
            dejaCompte = true;
            dejaCompteurDecroissant = true;
        }
        abscisseLoaded=abscisse;
    }
}

Mat TraitementVideo::detectBody(Mat frame){
    vector<Rect> body;
    body_cascade.detectMultiScale(frame,body,1.1,2,0 | CV_HAAR_SCALE_IMAGE,Size(90,90)); // detection de visages sur l'image (de taille minimum 90 px * 90 px)
    if (body.size()>0){ // Si au moins un visage est détecté.
        debuglbl->setText("body detecteProjetSY25maind !");
        plusGrandRectangle=0;
        indicePlusGrand =0;
        for(size_t i=0;i<body.size();i++) // Boucle qui vient chercher le plus grand des visages détectés.
        {
            if (body[i].width*body[i].height>plusGrandRectangle){ // Si le rectangle ainsi calculé est le plus grand.
                plusGrandRectangle = body[i].width*body[i].height; // On calcule la taille du rectangle
                indicePlusGrand = i;
            }
        }
        rectangle(frame, body[indicePlusGrand], CV_RGB(0, 0,0), 2); // Dessine un rectangle autour du visage détecté.
        faceCenterX = body[indicePlusGrand].x +0.5*body[indicePlusGrand].width; // calcule l'abscisse du centre du visage
        faceCenterY = body[indicePlusGrand].y + 0.5*body[indicePlusGrand].height; // calcule l'ordonnée du centre du visage
        Point centreBody(faceCenterX, faceCenterY); // définit un point.
        countFlux(faceCenterX);
        circle(frame, centreBody, 2, CV_RGB(0, 0,0), 2, 8,0 ); // trace un cercle autour de ce point.
    }else { // Si on a pas réussi à identifier un visage, on affiche une croix sur le panneau led.
        debuglbl->setText("pas de body detected !");
        abscisseLoaded = -1 ;
    }
return frame;
}

/*
 *Fonction qui s'occupe de capturer une image et de l'afficher dans le label associé.
 */
void TraitementVideo::capturePicture(){

    camera.grab(); // On capture une image
    camera.retrieve(image); // On stocke l'image dans une image (sous forme de Mat)
    debuglbl->setText("photo prise !");
    flip(image,image,0); // On la retourne (à l'envers par défaut)
    detectBody(image); // On fait toutes les détections.
    QString compteurString = QString::number(compteur);
    compteurlbl->setText("compteur : "+compteurString);
    QImage image2 = cvMatToQImage(image); // On convertit l'image en QImage
    debuglbl->setPixmap(QPixmap::fromImage(image2)); // Puis on l'affiche dans l'interface utilisateur.
    debuglbl->resize(debuglbl->pixmap()->size());
}



void TraitementVideo::on_pushButton_clicked()
{
        if(camera.open()){ // si la caméra s'est bien ouverte
               pushButton->setText("Stop");
               QTimer *timer = new QTimer();
               connect(timer, SIGNAL(timeout()), this, SLOT(capturePicture())); // lorsqu'on arrive à la fin du timer on prend une photo
               timer->setInterval(120); // définition de l'intervalle du timer (on prendra une photo toutes les 120 ms).
               timer->start(); // on démarre le timer
        }else{
            pushButton->setText("launch Server");
            camera.release(); // On libère la caméra.
        }
}

int TraitementVideo::getCompteur() {
    return compteur;
}

// Fonction qui avertit d'une nouvelle connection et lance la fonction pour la requête du client
void TraitementVideo::newConnection()
{
    qDebug() << "new connection";
    socket = server->nextPendingConnection();
    connect(socket,&QIODevice::readyRead,this,&TraitementVideo::readyRead);
}

// Fonction GUI qui gère la requête du client en transmettant la variable compteu sur la page web
void TraitementVideo::readyRead() {
    while(socket->canReadLine()){
        char webBrowerRXData[1000];
        int sv=socket->read(webBrowerRXData,1000);
        cout<<"\nreading web browser data=\n";
        for(int i=0;i<sv;i++)cout<<webBrowerRXData[i];
        cout<<"\n";

        socket->write("HTTP/1.1 200 OK\r\n");
        socket->write("Content-Type: text/html\r\n");
        socket->write("Connection: close\r\n");
        socket->write("\r\n\r\n");     //refreshes web browser     every second. Require two \r\n.
        socket->write("<!DOCTYPE html>\r\n");
        socket->write("<html><body>Number of persons in the room (compteur) : ");
        QByteArray str;
        str.setNum(compteur);   //convert int to string
        socket->write(str);
        socket->write(" </body>\n</html>\n");
        socket->flush();
        connect(socket, SIGNAL(disconnected()),socket, SLOT(deleteLater()));
        socket->disconnectFromHost();
   }
}

