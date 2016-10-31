#include <QObject>

/* Xlib is a terrible library. We can't even combine it with Qt. */
typedef struct _XDisplay Display;
using Window = unsigned long;

struct XWindow
{
    Display* display;
    Window window;
};

class ForegroundInfo : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int images READ images WRITE setImages NOTIFY imagesChanged)
    Q_PROPERTY(void* windowPtr READ windowPtr WRITE setWindowPtr NOTIFY windowPtrChanged)

    Q_PROPERTY(bool calibrated READ calibrated WRITE setCalibrated)
    Q_PROPERTY(bool scanningTime READ scanningTime WRITE setScanningTime)
    Q_PROPERTY(bool endScan READ endScan WRITE setEndScan)

    int m_images;
    void* m_windowPtr = nullptr;
    bool m_scanningTime = false;
    bool m_endScan = false;
    bool m_calibrated = false;

public:
    ForegroundInfo(QObject* parent = NULL) : QObject(parent) {}
    int images() const
    {
        return m_images;
    }
    void* windowPtr() const
    {
        return m_windowPtr;
    }
    bool scanningTime() const
    {
        return m_scanningTime;
    }
    bool endScan() const
    {
        return m_endScan;
    }
    bool calibrated() const
    {
        return m_calibrated;
    }

public slots:
    void setImages(int images)
    {
        if (m_images == images)
            return;

        m_images = images;
        emit imagesChanged(images);
    }
    void setWindowPtr(void* windowPtr)
    {
        if (m_windowPtr == windowPtr)
            return;

        m_windowPtr = windowPtr;
        emit windowPtrChanged(windowPtr);
    }
    void setScanningTime(bool scanningTime)
    {
        m_scanningTime = scanningTime;
    }
    void setEndScan(bool endScan)
    {
        m_endScan = endScan;
    }

    void setCalibrated(bool calibrated)
    {
        m_calibrated = calibrated;
    }

signals:
    void imagesChanged(int images);
    void windowPtrChanged(void* windowPtr);

    void kinectNotFound();

    void finishedCalibration();
    void finishedScanning();
};

extern ForegroundInfo f_info;
