/*  Copyright 2000 - 2001 by Robert Voigt <robert.voigt@gmx.de>  */

#include <qapplication.h>
#include "testwindow.h"


int main(int argc, char **argv)
{
    QApplication a(argc, argv);

    Testwindow testw;

    a.setMainWidget(&testw);

    testw.show();

    return a.exec();
}
