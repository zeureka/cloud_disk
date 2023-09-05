#include "index.h"
#include "frmlogin.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
//    Index i;
//    i.show();
    FrmLogin w;
    w.show();
    return a.exec();
}
