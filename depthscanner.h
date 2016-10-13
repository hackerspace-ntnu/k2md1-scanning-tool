#ifndef DEPTHSCANNER_H
#define DEPTHSCANNER_H

#include <QObject>
#include <QRunnable>

class DepthScanner : public QObject, QRunnable
{
    Q_OBJECT

    Q_PROPERTY(QString destinationPath READ destinationPath WRITE setDestinationPath NOTIFY destinationPathChanged)

    /* Data storage */
    QString m_destinationPath;

    /* Core functionality */
public:
    explicit DepthScanner(QObject *parent = 0);

signals:
    void updateImageCount(int images);
    void finishedScan(int images);


public slots:
    void startScan();
    void stopScanning();

    /* Property functions */
public:
    QString destinationPath() const
    {
        return m_destinationPath;
    }
signals:
    void destinationPathChanged(QString destinationPath);
public slots:
    void setDestinationPath(QString destinationPath)
    {
        if (m_destinationPath == destinationPath)
            return;

        m_destinationPath = destinationPath;
        emit destinationPathChanged(destinationPath);
    }
};

#endif // DEPTHSCANNER_H
