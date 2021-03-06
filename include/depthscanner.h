#ifndef DEPTHSCANNER_H
#define DEPTHSCANNER_H

#include <QObject>
#include <QRunnable>

class ScanTheThing;
class QWindow;

class DepthScanner : public QObject, public QRunnable
{
    Q_OBJECT

public:
    enum ScannerState
    {
        None,
        Calibrating,
        Calibrated,
        Scanning,
        Scanned
    };
    Q_ENUM(ScannerState)

private:
    Q_PROPERTY(ScannerState state READ state WRITE setState NOTIFY stateChanged)
    Q_PROPERTY(QString destinationPath READ destinationPath WRITE setDestinationPath NOTIFY destinationPathChanged)

    /* Data storage */
    QString m_destinationPath;

    ScanTheThing* m_userInterface;

    /* Core functionality */
    ScannerState m_state = None;

public:

    explicit DepthScanner(ScanTheThing* interface, QObject *parent = 0);
    void run();

    QString destinationPath() const
    {
        return m_destinationPath;
    }

    ScannerState state() const
    {
        return m_state;
    }

signals:
    void destinationPathChanged(QString destinationPath);
    void stateChanged(ScannerState state);

    void sendXWindow(QWindow* window);

public slots:
    void setDestinationPath(QString destinationPath)
    {
        if (m_destinationPath == destinationPath)
            return;

        m_destinationPath = destinationPath;
        emit destinationPathChanged(destinationPath);
    }

    void pushErrorMessage();

    void setState(ScannerState state);

    void receiveXWindow(void* winPtr);
};


#endif // DEPTHSCANNER_H
