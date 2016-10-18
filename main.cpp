#include "mainwindow.h"
#include "runguard.h"
#include <QApplication>

static constexpr auto APP_ID = "1600612773";

int main(int argc, char *argv[])
{
    RunGuard runGuard(APP_ID);
    if( ! runGuard.tryToRun())
    {
        return 1;
    }


    QApplication a(argc, argv);
    MainWindow w;

    //start on tray only
    //w.show()

    return a.exec();
}
