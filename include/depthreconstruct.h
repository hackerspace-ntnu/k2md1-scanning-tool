#ifndef DEPTHRECONSTRUCT_H
#define DEPTHRECONSTRUCT_H

#include <QObject>
#include <QRunnable>

class DepthReconstruct : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit DepthReconstruct(QObject *parent = 0);

signals:

public slots:

    // QRunnable interface
public:
    void run();
};

#endif // DEPTHRECONSTRUCT_H
