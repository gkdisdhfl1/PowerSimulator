#include "main_window.h"
#include "simulation_engine.h"

#include <QApplication>
#include <memory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // SimulationEngine 객체를 main에서 생성하고 소유
    std::unique_ptr<SimulationEngine> engine = std::make_unique<SimulationEngine>();

    // MainWindow에 SimulationEngine 객체를 주입
    MainWindow w(engine.get());
    w.show();

    return a.exec();
}
