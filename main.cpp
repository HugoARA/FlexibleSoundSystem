#include "flexiblesoundsystem.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    FlexibleSoundSystem w;
    w.show();

    return a.exec();
}
