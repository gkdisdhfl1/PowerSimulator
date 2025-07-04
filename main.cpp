#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

// 전압값 다이얼로 돌리면 제한 없이 돌아감
//

// 시간 지연 존재
// - 9초당 1초 정도 오차 발생

