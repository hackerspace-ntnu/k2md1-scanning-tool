#include "scanthething.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ScanTheThing w;
    w.show();

    return a.exec();
}
