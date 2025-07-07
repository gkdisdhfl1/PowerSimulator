#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

/* ---------- 문제점 --------

시간 지연 존재
- 3초당 0.1초 정도 오차 발생

*/
