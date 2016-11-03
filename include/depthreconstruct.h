#ifndef DEPTHRECONSTRUCT_H
#define DEPTHRECONSTRUCT_H

#include <QObject>
#include <QRunnable>

class ScanTheThing;

class DepthReconstruct : public QObject, public QRunnable
{
    Q_OBJECT

    Q_PROPERTY(int numImages READ numImages WRITE setNumImages)
    Q_PROPERTY(QString finalPlyFile READ finalPlyFile WRITE setFinalPlyFile)

    int m_numImages = 0;

    QString m_finalPlyFile;

    bool m_doFinishReconstruct = true;

    ScanTheThing* m_userInterface;

public:
    explicit DepthReconstruct(ScanTheThing *interface, QObject *parent = 0);
    void run();

    int numImages() const
    {
        return m_numImages;
    }

    QString finalPlyFile() const
    {
        return m_finalPlyFile;
    }

signals:
    void finishedDvoSomething();
    void finishedNormals();
    void finishedReconstruction();
    void finishedPoints();

public slots:

    void setNumImages(int numImages)
    {
        m_numImages = numImages;
    }
    void setFinalPlyFile(QString finalPlyFile)
    {
        m_finalPlyFile = finalPlyFile;
    }
};

#endif // DEPTHRECONSTRUCT_H
