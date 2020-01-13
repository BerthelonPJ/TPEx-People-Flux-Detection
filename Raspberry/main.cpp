#include "traitementvideo.h"
#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    TraitementVideo w ;

    w.show();

    return a.exec();
}
