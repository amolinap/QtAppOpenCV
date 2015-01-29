#-------------------------------------------------
#
# Project created by QtCreator 2014-05-21T15:43:08
#
#-------------------------------------------------

QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QtOpenCV
TEMPLATE = app

BASEDIR = $$IN_PWD
TARGETDIR = $$OUT_PWD
BUILDDIR = $$TARGETDIR/build
LANGUAGE = C++
OBJECTS_DIR = $$BUILDDIR/obj
MOC_DIR = $$BUILDDIR/moc
UI_HEADERS_DIR = src/ui/generated

ICON = icono.icns

include(QtOpenCV.pri)

INCLUDEPATH += /opt/local/include/opencv2/ \

LIBS += /opt/local/lib/libopencv_core.dylib \
        /opt/local/lib/libopencv_highgui.dylib \
        /opt/local/lib/libopencv_imgproc.dylib \
        /opt/local/lib/libopencv_features2d.dylib

LIBS += -L/opt/local/lib  -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_contrib -lopencv_calib3d
LIBS += -L/opt/local/lib -lopencv_features2d -lopencv_video -lopencv_objdetect  -lopencv_legacy -lopencv_flann -lopencv_gpu

SOURCES += src/main.cpp \
    src/OpenCVWidget.cpp \
    src/OverlayData.cpp \
    src/videoStabilizer.cc

HEADERS  += \
    src/OpenCVWidget.h \
    src/OverlayData.h \
    src/videoStabilizer.h

FORMS    += \
    src/OpenCVWidget.ui

RESOURCES += \
    recursos.qrc
