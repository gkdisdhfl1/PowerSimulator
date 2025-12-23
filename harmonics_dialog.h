#ifndef HARMONICS_DIALOG_H
#define HARMONICS_DIALOG_H

#include <QDialog>
#include "shared_data_types.h"

class HarmonicEditor;
class QTabWidget;

class HarmonicsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HarmonicsDialog(QWidget* parent = nullptr);

    // 엔진의 현재 데이터로 UI 초기화
    void setHarmonics(const HarmonicList& voltageHarmonics, const HarmonicList& currentHarmonics);

signals:
    // 실시간 업데이트를 위한 시그널
    void voltageHarmonicsChanged(const HarmonicList& harmonics);
    void currentHarmonicsChanged(const HarmonicList& harmonics);

private:
    HarmonicEditor* m_voltageEditor;
    HarmonicEditor* m_currentEditor;
    QTabWidget* m_tabWidget;
};

#endif // HARMONICS_DIALOG_H
