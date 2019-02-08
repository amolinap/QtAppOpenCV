/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

#ifndef OverlayData_H
#define OverlayData_H

#include <QImage>
#include <QGLWidget>
#include <QPainter>
#include <QFontDatabase>
#include <QTimer>
#include <QTime>
#include <QInputDialog>
#include <QShowEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QDesktopServices>
#include <QFileDialog>

#include <QDebug>
#include <cmath>
#include <qmath.h>
#include <limits>

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "videoStabilizer.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/video/tracking.hpp"
#include "opencv2/legacy/legacy.hpp"

//#include "UASManager.h"
//#include "SlugsMAV.h"

using namespace cv;
using namespace std;

/** @brief Displays the telemetry overlay video. */
class OverlayData : public QGLWidget
{
    Q_OBJECT
public:
    /**
     * @brief This is the class constructor.
     *
     * @param  width    Width the video widget
     * @param  height   Height the video widget
     * @param  parent   Parent the video widget
    **/
    OverlayData(int width = 640, int height = 480, QWidget* parent = NULL);
    ~OverlayData();

    //void resizeGL(int w, int h);

    /** @brief OpenCV class for holds URL video */
    cv::VideoCapture captureVideo;
    /** @brief OpenCV class for writer video */
    cv::VideoWriter writerMovie;
    /** This is the video stabilizer algorithm class*/
    videoStabilizer* video;
    /** @brief Used to holds if record video */
    bool isRecord;
    bool existFileMovie;    

public slots:    
    /** @brief Initialize the opengl graphics */
    void initializeGL();
    /** @brief Enable the telemetry data view
      *
      * @param enabled Enable visualization telemetry on display
    */
    void enableTelemetryData(bool enabled);
    /** @brief Enable the stabilization of video
      *
      * @param enabled Enable stabilization video
    */
    void enableStabilizationVideo(bool enabled);
    /** @brief Enable the tracking of position
      *
      * @param enabled Enable tracking
    */
    void enableTrackingPosition(bool enabled);
    /** @brief Enable the send tracking of position for orders camera
      *
      * @param enabled Enable send tracking
    */
    void enableSendTrackingPosition(bool enabled);
    /**
      * @brief Receive UAS currently selected
      *
      * @param UAS UAS currently selected
    */
    //void activeUasSet(UASInterface* uas);
    /** @brief Receive update the mode of flight
      *
      * @param sysId        The id of active UAS
      * @param status       The mode flight
      * @param description  Description of status mode
    */
    void updateMode(int uas,QString mode,QString description);
    /** @brief Receive update the mode of navigation used in flight
      *
      * @param uasid    The id of active UAS
      * @param mode     The mode navigation
      * @param text     Description of status mode navigation
    */
    void updateModeNavigation(int uasid, int mode, const QString &text);
    /** @brief Receive update the status of flight
      *
      * @param status   The status of flight
    */
    //void updateStatusNavigation(int status);
    /**
      * @brief Receive updating battery voltage
      *
      * @param id       The id of active UAS
      * @param battery  Voltage of battery
    */
    void setAlertBattery(int id, double battery);
    /**
      * @brief Receive updating airspeed
      *
      * @param airSpeed Airspeed flight
    */
    void setAlertAirSpeed(double airSpeed);
    /** @brief Receive update the new position GPS global
      *
      * @param uas  The active UAS
      * @param lat  Latitude GPS
      * @param lon  Longitude GPS
      * @param alt  Altitude GPS
      * @param usec The UNIX timestamp in milliseconds
    */
    //void updateGlobalPosition(UASInterface * uas, double lat, double lon, double alt, quint64 usec);
    /**
      * @brief Receive updating altitude
      *
      * @param altitude Altitude plane z
    */
    void updateAltitude(double z);
    /**
     * @brief This method allows assign new path to video open/store.
     *
     * @param path The path where the video open/store
     **/
    void changePATH(const QString& path);

    void setSavedAutomatic(bool automatic);

public slots:
    /** @brief This functions works in the OpenGL view, which is already translated by the x and y center offsets. */
    void paintCenterBackground(float roll, float pitch, float yaw);
    /**
     * @brief Paint text on top of the image and OpenGL drawings
     *
     * @param text chars to write
     * @param color text color
     * @param fontSize text size in mm
     * @param refX position in reference units (mm of the real instrument). This is relative to the measurement unit position, NOT in pixels.
     * @param refY position in reference units (mm of the real instrument). This is relative to the measurement unit position, NOT in pixels.
     */
    void paintText(QString text, QColor color, float fontSize, float refX, float refY, QPainter* painter);
    /**
     * @brief Setup the OpenGL view for drawing a sub-component of the HUD
     *
     * @param referencePositionX horizontal position in the reference mm-unit space
     * @param referencePositionY horizontal position in the reference mm-unit space
     * @param referenceWidth width in the reference mm-unit space
     * @param referenceHeight width in the reference mm-unit space
     */
    void setupGLView(float referencePositionX, float referencePositionY, float referenceWidth, float referenceHeight);
    /** @brief Paint the overlay of telemetry data */
    void paintTelemetry();
//    /**
//     * @brief Draw line overlay video
//     *
//     * @param refX1     Reference position X point 1
//     * @param refY1     Reference position Y point 1
//     * @param refX2     Reference position X point 2
//     * @param refY2     Reference position Y point 2
//     * @param width     Width line
//     * @param color     Color line
//     * @param painter   Object for paint
//     */
//    void drawLine(float refX1, float refY1, float refX2, float refY2, float width, const QColor& color, QPainter* painter);
//    /**
//     * @brief Draw vertical line indicator
//     *
//     * @param refX1     Reference position X point 1
//     * @param refY1     Reference position Y point 1
//     * @param height    Height line
//     * @param minRate   Value minimun
//     * @param maxRate   Value maximun
//     * @param value     Value to paint
//     * @param painter   Object for paint
//     */
//    void drawVerticalIndicator(float xRef, float yRef, float height, float minRate, float maxRate, float value, QPainter* painter);
//    /**
//     * @brief Draw horizontal line indicator
//     *
//     * @param refX1     Reference position X point 1
//     * @param refY1     Reference position Y point 1
//     * @param height    Height line
//     * @param minRate   Value minimun
//     * @param maxRate   Value maximun
//     * @param value     Value to paint
//     * @param painter   Object for paint
//     */
//    void drawHorizontalIndicator(float xRef, float yRef, float height, float minRate, float maxRate, float value, QPainter* painter);
    /** @brief This method playback starts */
    void playMovie();
    /** @brief This method playback stops. */
    void stopMovie();
    /** @brief This method playback starts. */
    void openRTSP();
    /** @brief This method open file in a directory. */
    void openFile();
    /** @brief This method video storage begins. */
    void record(bool value);
    /**
     * @brief This method add new URL media for video player.
     *
     * @param url The URL where the video store
     **/
    void setURL(QString url);    
    /**
     * @brief This method return the time actual in milliseconds.
     *
     * @return Time in milliseconds
     **/
    quint64 getGroundTimeNow();
    /**
     * @brief This method receive the directory for creates the subtitle file to store GPS positions.
     *
     * @param file Subtitle file for store GPS positions
     */
    void createFileSubTitles(QString file);
    /** @brief This method refresh values timeout 1 second. */
    void refreshTimeOut();

protected:    
    /** @brief Converted to reference mm units
     *
     * @param x coordinate in pixels to be converted to reference mm units
     * @return the screen coordinate relative to the QGLWindow origin
     */
    float refToScreenX(float x);
    /** @brief Converted to reference mm units
     *
     * @param y coordinate in pixels to be converted to reference mm units
     * @return the screen coordinate relative to the QGLWindow origin
     */
    float refToScreenY(float y);
    /** @brief Convert mm line widths to QPen line widths */
    float refLineWidthToPen(float line);
    /** @brief Preferred Size */
    QSize sizeHint() const;
    /** @brief Start updating widget */
    void showEvent(QShowEvent* event);
    /** @brief Stop updating widget */
    void hideEvent(QHideEvent* event);
    /**
     * @brief This method manage the opening a context menu.
     *
     * @param event Describe a context menu
     **/
    void contextMenuEvent (QContextMenuEvent* event);
    /** @brief Create the actions for context menu */
    void createActions();
    /** @brief Overrides the event MousePress
     * @param e The event mouse received
     */
    void mousePressEvent(QMouseEvent *e);
    /** @brief Overrides the event PaintEvent
     * @param e The event paint received
     */
    void paintEvent(QPaintEvent *e);
    /** @brief Initializate variables for tracking position */
    void initializeTracking();
    /** @brief Process video for tracking position */
    void processTracking();

private:
    static const int updateInterval = 40;

    QImage glImage;

    QString mode;
    QString state;
    QString navigation;
    QString pathVideo;
    double airSpeed;
    double battery;
    double lat;
    double lon;
    double alt;

    double scalingFactor;
    float xCenterOffset, yCenterOffset;
    float vwidth;
    float vheight;
    int xCenter;
    int yCenter;
    QColor defaultColor;
    QColor infoColor;
    QTimer* refreshTimer;
    //QFont font;

    bool telemetryData;
    bool videoStabilizated;
    bool videoEnabled;
    bool videoTracking,
        sendTrackingVideo;
    //UASInterface* activeUAS;

    QAction* enableTelemetry;
    QAction* enableStabilization;
    QAction* enableTracking,
        *enableSendTracking;
    bool isSubTitles, savedAutomatic;
    QFile *fileSubtitles;
    quint64 startTime;
    int countSubTitle;
    QTextStream streamData;
    QString urlVideo;

    //::Tracking
    KalmanFilter kalmaFilter;
    vector<KeyPoint> keyPointsSearch, keyPointsInterest;

    Point trackingPoint,
        videoCenter,
        statePoint;

    Mat frame,
        //outputFrame,
        areaInterest,
        areaSearch,
        descriptorsSearch,
        descriptorsInterest;

    Ptr<FeatureDetector> featureDetectSearch,
        featureDetectInterest;

    bool selected,
        tracking,
        useKalman,
        drawDescriptors,
        drawCenter,
        useBruteMatch;

    int sizeAreaInterest,
        sizeAreaSearch,
        minimumMatchesSearch,
        refreshSearch,
        frameRefreshSearch,
        moveTracking;

    vector<DMatch> matches,
        realMatchesUsed;

    String FeatureMethod;

    Mat_<float> measurement;

    void DrawCrossHair(Mat& image, Point center, int size, Scalar color)
    {
        line(image, Point(center.x - size,center.y), Point(center.x + size,center.y), color);
        line(image, Point(center.x,center.y + size), Point(center.x,center.y - size), color);
    }

signals:
    /** @brief Emit the image selected for mouse */
    void emitCaptureImage(QImage img);
    /** @brief Emit the title of video selected */
    void emitTitle(QString title);
    /** @brief Emit if video is record */
    void emitRecord(bool record);
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
    /** @brief Emit the position of tracking */
    void emitPositionTracking(int x, int y);
};

#endif // OverlayData_H
