#-------------------------------------------------
#
# Project created by QtCreator 2016-10-04T17:30:49
#
#-------------------------------------------------

QT       += core gui
CONFIG   += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ScanTheThing
TEMPLATE = app

INCLUDEPATH += include

FORMS    += ui/scanthething.ui

unix:!macx: LIBS += -L$$PWD/lib/lin64/ -lfreenect2 -lglfw -lturbojpeg -lX11
LIBS += -lOpenCL

INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include

SOURCES += \
        main.cpp\
        \
        src/scanthething.cpp \
        src/depthscanner.cpp \
        src/depthreconstruct.cpp \
        src/depthorient.cpp \
        \
        src/meshing/normals.cpp \       # Calculates normals
        src/pointcloud/points.cpp \     # Showing the points graphically
        src/recorder/foreground.cpp \   # Scanning
        src/tracker/dualdvo.cpp \       # Somethingsomething matrices
        \
        src/dataio.cpp \
        src/foreground_info.cpp \
        src/orientation_info.cpp

HEADERS  += \
        include/scanthething.h \
        include/depthscanner.h \
        include/depthreconstruct.h \
        include/depthorient.h \
        \
        include/meshing.h \
        include/pointcloud.h \
        include/recorder.h \
        include/tracker.h \
        include/foreground_info.h \
        include/orientation_info.h

# Install OpenCL programs
resource_install.commands += $(COPY_DIR) $$PWD/rsc/tracker/ $$OUT_PWD/data && $(COPY_DIR) $$PWD/extern/ssd/ $$OUT_PWD/bin
first.depends = $(first) resource_install
export(first.depends)
export(resource_install.commands)
QMAKE_EXTRA_TARGETS += first resource_install
