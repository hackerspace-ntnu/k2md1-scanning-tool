#ifndef SCANTHETHING_H
#define SCANTHETHING_H

#include <QMainWindow>

class QTimer;

namespace Ui {
class ScanTheThing;
}

class ScanTheThing : public QMainWindow
{
    Q_OBJECT

public:
    explicit ScanTheThing(QWidget *parent = 0);
    ~ScanTheThing();

private slots:
    void timerFire();

    void on_progressBar_valueChanged(int value);

private:
    Ui::ScanTheThing *ui;

    QTimer* m_testFire;
};

#endif // SCANTHETHING_H
