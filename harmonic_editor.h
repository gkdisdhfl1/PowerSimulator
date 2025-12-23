#ifndef HARMONIC_EDITOR_H
#define HARMONIC_EDITOR_H

#include "shared_data_types.h"
#include <QWidget>

class QVBoxLayout;
class HarmonicEditor : public QWidget
{
    Q_OBJECT
public:
    explicit HarmonicEditor(QWidget *parent = nullptr);

    void setHarmonics(const HarmonicList& harmonics);
    HarmonicList getHarmonics() const;

signals:
    void harmonicsChanged(const HarmonicList& harmonics);

private slots:
    void onAddClicked();
    void onItemChanged(); // 하위 아이템 변경 시 호출
    void onItemDeleted();

private:
    void addRow(const HarmonicComponent& data);

    QVBoxLayout* m_listLayout;
};

#endif // HARMONIC_EDITOR_H
