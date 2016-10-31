#include <QObject>

class Orientation_Info : public QObject
{
    Q_OBJECT

    Q_PROPERTY(const char* viewFile READ viewFile WRITE setViewFile NOTIFY viewFileChanged)

    const char* m_viewFile;

public:
    Orientation_Info(QObject* parent = NULL) : QObject(parent) {}

    const char* viewFile() const
    {
        return m_viewFile;
    }
public slots:
    void setViewFile(const char* viewFile)
    {
        if (m_viewFile == viewFile)
            return;

        m_viewFile = viewFile;
        emit viewFileChanged(viewFile);
    }
signals:
    void viewFileChanged(const char* viewFile);
};
