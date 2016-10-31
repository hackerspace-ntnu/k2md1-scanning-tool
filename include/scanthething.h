#ifndef SCANTHETHING_H
#define SCANTHETHING_H

#include <QMainWindow>
#include <QThreadPool>

class DepthScanner;
class DepthOrient;
class DepthReconstruct;
class QMessageBox;

namespace Ui {
class ScanTheThing;
}

class ScanTheThing : public QMainWindow
{
    Q_OBJECT

public:
    explicit ScanTheThing(QWidget *parent = 0);
    ~ScanTheThing();

public slots:
    void displayErrorMessage(QString const& error);
    void resetView();

    /* Unlocking new stages of the process */
    void activateScanning();
    void activateReconstruction();

    /* Tracking progress */
    void setProgress(bool continuous, int progress, int progressMax, QString const& text);

    void updateImageCount(int imgs);

private slots:
    void on_progressBar_valueChanged(int value);

    void on_calibrateButton_clicked();
    void on_scanButton_clicked();

    void on_reconButton_clicked();

private:
    QThreadPool m_threadPool;

    DepthScanner* m_scanner = nullptr;
    DepthOrient* m_orient = nullptr;
    DepthReconstruct* m_reconstruct = nullptr;

    Ui::ScanTheThing *ui;

    QMessageBox* m_errorBox = nullptr;
};

#endif // SCANTHETHING_H
