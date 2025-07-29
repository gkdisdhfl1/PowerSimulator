#include "main_window.h"
#include "simulation_engine.h"
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

    // SimulationEngine 객체를 main에서 생성하고 소유
    std::unique_ptr<SimulationEngine> engine = std::make_unique<SimulationEngine>();

    // MainWindow에 SimulationEngine 객체를 주입
    MainWindow w(engine.get());
    w.show();

    return a.exec();
}
