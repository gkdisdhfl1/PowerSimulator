#include "system_controller.h"
#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //  ---- 스타일시트 적용 코드 -----
    QFile styleFile(":/styles/stylesheet.qss"); // 리소스 경로 사용
    if(styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        a.setStyleSheet(styleSheet);
        styleFile.close();
    }

    // SystemController가 모든 것을 관리
    SystemController system;
    system.initialize();

    return a.exec();
}
