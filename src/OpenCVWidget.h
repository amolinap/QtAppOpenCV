#ifndef OPENCVWIDGET_H
#define OPENCVWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QDebug>
#include <QFileDialog>
#include <QMouseEvent>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

#include "OverlayData.h"

namespace Ui {
    class OpenCVWidget;
}

/** @brief The OpenCV widget defines the interface for displaying real-time video in the control station.. */
class OpenCVWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief This is the class constructor.
     *
     * @param  parent   Parent the video widget
    **/
    explicit OpenCVWidget(QWidget *parent = 0);
    ~OpenCVWidget();

private:
    Ui::OpenCVWidget *ui;    

    QPushButton* btPlay;
    QPushButton* btStop;
    QPushButton* btFile;
    QPushButton* btRTSP;
    QPushButton* btRecord;
    QLabel* lbTitle;

public slots:    
    void showCaptureImage(QImage img);
    /**
     * @brief This method allows assign new path to video open/store.
     *
     * @param path The path where the video open/store
     **/
    void changePATH(const QString& path);
    /**
     * @brief This method allows assign if video is record.
     *
     * @param record If video is recording
     **/
    void changeStatusRecord(bool record);

    void savedAutomatic(bool automatic);

signals:
    /** @brief Emit the change path for store record video.*/
    void emitChangePath(const QString& path);
    /** @brief Emit the distance in pixels to the center of the image
    *   (+x, +y)   |   (-x, +y)
    *              |
    *              |
    *--------------|-------------
    *   (+x, -y)   |   (-x, -y)
    *              |
    *              |
    */
    void emitDistanceToCenter(int x, int y);

    void setSavedAutomatic(bool automatic);
};

#endif // OPENCVWIDGET_H
