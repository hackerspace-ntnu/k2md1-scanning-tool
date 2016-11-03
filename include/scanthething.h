#ifndef SCANTHETHING_H
#define SCANTHETHING_H

#include <QMainWindow>
#include <QThreadPool>

class DepthScanner;
class DepthOrient;
class DepthReconstruct;
class QMessageBox;
class QWindow;

namespace Ui {
class ScanTheThing;
}

class ScanTheThing : public QMainWindow
{
    Q_OBJECT

    Q_PROPERTY(int numImages READ numImages WRITE setNumImages)

public:
    explicit ScanTheThing(QWidget *parent = 0);
    ~ScanTheThing();

    int numImages() const
    {
        return m_numImages;
    }

public slots:
    void displayErrorMessage(QString const& error);
    void resetEverything();

    /* Unlocking new stages of the process */
    void activateScanning();
    void activateReconstruction();

    /* Tracking progress */
    void setProgress(bool continuous, int progress, int progressMax, QString const& text);

    void setProgress(int value, int max);

    void updateImageCount(int imgs);

    void insertXWindow(QWindow* window);

    void setNumImages(int numImages)
    {
        m_numImages = numImages;
    }

    void finishedProcess();

private slots:
    void on_progressBar_valueChanged(int value);

    void on_calibrateButton_clicked();
    void on_scanButton_clicked();

    void on_reconButton_clicked();

    void on_actionQuit_triggered();

    void on_actionAbout_Qt_triggered();

    void on_actionAbout_triggered();

    void on_actionReset_triggered();

private:
    QWindow* m_currentWindow;
    QWidget* m_currentWindowContainer;

    QThreadPool m_threadPool;

    DepthScanner* m_scanner = nullptr;
    DepthReconstruct* m_reconstruct = nullptr;

    Ui::ScanTheThing *ui;

    QMessageBox* m_errorBox = nullptr;
    int m_numImages;
};

#endif // SCANTHETHING_H
