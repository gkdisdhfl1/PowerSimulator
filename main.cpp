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
- 9초당 1초 정도 오차 발생

 그래프가 꽉 차지 않음

*/
