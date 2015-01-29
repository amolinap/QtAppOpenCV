/*===================================================================
======================================================================*/

#include "OverlayData.h"

static int secondTracking = 2;
static int distanceCenter = 20;

template<typename T>
inline bool isnan(T value)
{
    return value != value;
}

template<typename T>
inline bool isinf(T value)
{
    return std::numeric_limits<T>::has_infinity && (value == std::numeric_limits<T>::infinity() || (-1*value) == std::numeric_limits<T>::infinity());
}

OverlayData::OverlayData(int width, int height, QWidget* parent)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent),        
    mode(tr("UNKNOWN MODE")),
    state(tr("UNKNOWN STATE")),
    navigation(tr("UNKNOWN STATE")),
    xCenterOffset(0.0f),
    yCenterOffset(0.0f),
    vwidth(200.0f),
    vheight(150.0f),
    defaultColor(QColor(255, 255, 255)),
    infoColor(QColor(255, 255, 255)),
    refreshTimer(new QTimer(this))//,
//    airSpeed(0.0f),
//    battery(0.0f),
//    lat(0.0),
//    lon(0.0),
//    alt(0.0),
//    telemetryData(false),
//    videoStabilizated(false),
//    videoEnabled(true),
//    video(NULL),
//    isRecord(false),
//    existFileMovie(false),
//    pathVideo(tr("/")),
//    activeUAS(NULL),
//    countSubTitle(0),
//    isSubTitles(false),
//    startTime(-1),
//    fileSubtitles(NULL),
//    urlVideo(""),
//    sendTrackingVideo(false),
//    videoTracking(false),
//    savedAutomatic(true)
{    
    airSpeed = 0.0f;
    battery = 0.0f;
    lat = 0.0;
    lon = 0.0;
    alt = 0.0;
    telemetryData = false;
    videoStabilizated = false;
    videoEnabled = true;
    video = NULL;
    isRecord = false;
    existFileMovie = false;
    pathVideo = tr("/");
    //activeUAS = NULL;
    countSubTitle = 0;
    isSubTitles = false;
    startTime = -1;
    fileSubtitles = NULL;
    urlVideo = "";
    sendTrackingVideo = false;
    videoTracking = false;
    savedAutomatic = true;

    setAutoFillBackground(false);
    setMinimumSize(80, 60);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    //connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(activeUasSet(UASInterface*)));

    QImage fill = QImage(width, height, QImage::Format_Indexed8);
    fill.setColorCount(3);
    fill.setColor(0, qRgb(0, 0, 0));
    fill.setColor(1, qRgb(0, 0, 0));
    fill.setColor(2, qRgb(0, 0, 0));
    fill.fill(0);

    refreshTimer->setInterval(updateInterval);

    connect(refreshTimer, SIGNAL(timeout()), this, SLOT(paintTelemetry()));

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(refreshTimeOut()));
    timer->start(1000);

    createActions();
    initializeTracking();
    setVisible(true);
}

OverlayData::~OverlayData()
{
    refreshTimer->stop();
}

QSize OverlayData::sizeHint() const
{
    return QSize(width(), (width()*3.0f)/4);
}

void OverlayData::showEvent(QShowEvent* event)
{
    // React only to internal (pre-display)
    // events
    Q_UNUSED(event)
    //refreshTimer->start(updateInterval);
    }

void OverlayData::hideEvent(QHideEvent* event)
{
    // React only to internal (pre-display)
    // events
    Q_UNUSED(event);
    //refreshTimer->stop();
}

void OverlayData::contextMenuEvent (QContextMenuEvent* event)
{
    QMenu menu(this);    
    enableTelemetry->setChecked(telemetryData);
    enableStabilization->setChecked(videoStabilizated);
    enableTracking->setChecked(videoTracking);
    enableSendTracking->setChecked(sendTrackingVideo);

    menu.addAction(enableTelemetry);
    menu.addAction(enableStabilization);
    menu.addAction(enableTracking);

    if(videoTracking)
    {
        menu.addAction(enableSendTracking);
    }

    menu.exec(event->globalPos());
}

void OverlayData::createActions()
{
    enableTelemetry = new QAction(tr("Habilitar vista de datos"), this);
    enableTelemetry->setCheckable(true);
    enableTelemetry->setChecked(telemetryData);
    connect(enableTelemetry, SIGNAL(triggered(bool)), this, SLOT(enableTelemetryData(bool)));

    enableStabilization = new QAction(tr("Habilitar estabilizacion de video"), this);
    enableStabilization->setCheckable(true);
    enableStabilization->setChecked(videoStabilizated);
    connect(enableStabilization, SIGNAL(triggered(bool)), this, SLOT(enableStabilizationVideo(bool)));

    enableTracking = new QAction(tr("Habilitar seguimiento de posicion"), this);
    enableTracking->setCheckable(true);
    enableTracking->setChecked(videoTracking);
    connect(enableTracking, SIGNAL(triggered(bool)), this, SLOT(enableTrackingPosition(bool)));

    enableSendTracking = new QAction(tr("Habilitar envio de seguimiento de ordenes"), this);
    enableSendTracking->setCheckable(true);
    enableSendTracking->setChecked(sendTrackingVideo);
    connect(enableSendTracking, SIGNAL(triggered(bool)), this, SLOT(enableSendTrackingPosition(bool)));
}

float OverlayData::refToScreenX(float x)
{
    //qDebug() << "sX: " << (scalingFactor * x);
    return (scalingFactor * x);
}

float OverlayData::refToScreenY(float y)
{
    //qDebug() << "sY: " << (scalingFactor * y);
    return (scalingFactor * y);
}

void OverlayData::paintCenterBackground(float roll, float pitch, float yaw)
{
    Q_UNUSED(pitch);
    // Center indicator is 100 mm wide
    float referenceWidth = 70.0;
    float referenceHeight = 70.0;

    // HUD is assumed to be 200 x 150 mm
    // so that positions can be hardcoded
    // but can of course be scaled.

    double referencePositionX = vwidth / 2.0 - referenceWidth/2.0;
    double referencePositionY = vheight / 2.0 - referenceHeight/2.0;

    //this->width()/2.0+(xCenterOffset*scalingFactor), this->height()/2.0+(yCenterOffset*scalingFactor);

    setupGLView(referencePositionX, referencePositionY, referenceWidth, referenceHeight);

    // Store current position in the model view
    // the position will be restored after drawing
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    // Move to the center of the window
    glTranslatef(referenceWidth/2.0f,referenceHeight/2.0f,0);

    // Move based on the yaw difference
    glTranslatef(yaw, 0.0f, 0.0f);

    // Rotate based on the bank
    glRotatef((roll/M_PI)*180.0f, 0.0f, 0.0f, 1.0f);

    // Translate in the direction of the rotation based
    // on the pitch. On the 777, a pitch of 1 degree = 2 mm
    //glTranslatef(0, ((-pitch/M_PI)*180.0f * vPitchPerDeg), 0);
    glTranslatef(0.0f, 0.0f, 0.0f);

    // Ground
    glColor3ub(179,102,0);

    glBegin(GL_POLYGON);
    glVertex2f(-300,-300);
    glVertex2f(-300,0);
    glVertex2f(300,0);
    glVertex2f(300,-300);
    glVertex2f(-300,-300);
    glEnd();

    // Sky
    glColor3ub(0,153,204);

    glBegin(GL_POLYGON);
    glVertex2f(-300,0);
    glVertex2f(-300,300);
    glVertex2f(300,300);
    glVertex2f(300,0);
    glVertex2f(-300,0);

    glEnd();
}

void OverlayData::paintText(QString text, QColor color, float fontSize, float refX, float refY, QPainter* painter)
{
    QPen prevPen = painter->pen();
    float pPositionX = refToScreenX(refX) - (fontSize*scalingFactor*0.072f);
    float pPositionY = refToScreenY(refY) - (fontSize*scalingFactor*0.212f);

    QFont font("Bitstream Vera Sans");
    // Enforce minimum font size of 5 pixels
    int fSize = qMax(5, (int)(fontSize*scalingFactor*1.26f));
    font.setPixelSize(fSize);

    QFontMetrics metrics = QFontMetrics(font);
    int border = qMax(4, metrics.leading());
    QRect rect = metrics.boundingRect(0, 0, width() - 2*border, int(height()*0.125),
                                      Qt::AlignLeft | Qt::TextWordWrap, text);
    painter->setPen(color);
    painter->setFont(font);
    painter->setRenderHint(QPainter::TextAntialiasing);
    painter->drawText(pPositionX, pPositionY,
                      rect.width(), rect.height(),
                      Qt::AlignCenter | Qt::TextWordWrap, text);
    painter->setPen(prevPen);
}

void OverlayData::initializeGL()
{
    bool antialiasing = true;

    // Antialiasing setup
    if(antialiasing)
    {
        glEnable(GL_MULTISAMPLE);
        glEnable(GL_BLEND);

        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_POINT_SMOOTH);
        glEnable(GL_LINE_SMOOTH);

        glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    }
    else
    {
        glDisable(GL_BLEND);
        glDisable(GL_POINT_SMOOTH);
        glDisable(GL_LINE_SMOOTH);
    }
}

void OverlayData::setupGLView(float referencePositionX, float referencePositionY, float referenceWidth, float referenceHeight)
{
    int pixelWidth  = (int)(referenceWidth * scalingFactor);
    int pixelHeight = (int)(referenceHeight * scalingFactor);
    // Translate and scale the GL view in the virtual reference coordinate units on the screen
    int pixelPositionX = (int)((referencePositionX * scalingFactor) + xCenterOffset);
    int pixelPositionY = this->height() - (referencePositionY * scalingFactor) + yCenterOffset - pixelHeight;

    //qDebug() << "Pixel x" << pixelPositionX << "pixelY" << pixelPositionY;
    //qDebug() << "xCenterOffset:" << xCenterOffset << "yCenterOffest" << yCenterOffset


    //The viewport is established at the correct pixel position and clips everything
    // out of the desired instrument location
    glViewport(pixelPositionX, pixelPositionY, pixelWidth, pixelHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // The ortho projection is setup in a way that so that the drawing is done in the
    // reference coordinate space
    glOrtho(0, referenceWidth, 0, referenceHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    //glScalef(scaleX, scaleY, 1.0f);
}

void OverlayData::paintEvent(QPaintEvent *e)
{
    // Event is not needed
    // the event is ignored as this widget
    // is refreshed automatically
    Q_UNUSED(e);
}

void OverlayData::paintTelemetry()
{
    if (isVisible())
    {
        makeCurrent();

        if (videoEnabled)
        {
            if(captureVideo.isOpened())
            {
                if (video == NULL )
                {
                    QRect imageSize;
                    imageSize.setWidth(captureVideo.get(CV_CAP_PROP_FRAME_WIDTH));
                    imageSize.setHeight(captureVideo.get(CV_CAP_PROP_FRAME_HEIGHT));

                    //qDebug()<<"width: "<< captureVideo.get(CV_CAP_PROP_FRAME_WIDTH);
                    //qDebug()<<"height: "<< captureVideo.get(CV_CAP_PROP_FRAME_HEIGHT);

                    video = new videoStabilizer(imageSize);
                    connect(video,SIGNAL(gotDuration(double&)), this, SLOT(updateTimeLabel(double&)));
                }

                cv::Mat frame;

                if(captureVideo.read(frame))
                {
                    if(isRecord)
                    {
                        if(!existFileMovie)
                        {
                            QString fileName = QDate::currentDate().toString("yyyyMMdd")+QTime::currentTime().toString("HHmmss");
                            QString path = pathVideo+fileName+".avi";
                            writerMovie.open(path.toLatin1().data(), CV_FOURCC('D','I','V','X'), 30, frame.size(), true);

                            createFileSubTitles(pathVideo+fileName);

                            existFileMovie = true;
                        }                        

                        writerMovie << frame;
                    }



                    if(videoTracking)
                    {
                        if(selected)
                        {
                            this->frame = frame;

                            try
                            {
                                areaInterest = frame(Rect(trackingPoint.x - sizeAreaInterest/2.0, trackingPoint.y - sizeAreaInterest/2.0, sizeAreaInterest, sizeAreaInterest));

                                // Detect the keypoints
                                featureDetectInterest->detect(areaInterest, keyPointsInterest); // NOTE: featureDetector is a pointer hence the '->'.
                                //   printf("Termina featureDetectorObject %d\n" , time.mds);
                                //Similarly, we create a smart pointer to the SIFT extractor.
                                Ptr<DescriptorExtractor> featureExtractorObject = DescriptorExtractor::create(FeatureMethod);

                                // Compute the 128 dimension SIFT descriptor at each keypoint.
                                // Each row in "descriptors" correspond to the SIFT descriptor for each keypoint

                                featureExtractorObject->compute(areaInterest, keyPointsInterest, descriptorsInterest);

                                if(drawDescriptors)
                                {
                                    // If you would like to draw the detected keypoint just to check
                                    Scalar keypointColorObject = Scalar(255, 0, 0);     // Blue keypoints.
                                    drawKeypoints(areaInterest, keyPointsInterest, areaInterest, keypointColorObject, DrawMatchesFlags::DEFAULT);
                                }

                                selected = false;
                                tracking = true;
                            }
                            catch(...)
                            {
                                qDebug()<<"Error...";
                            }

                        }

                        processTracking();
                    }

                    if(videoStabilizated)
                    {
                        cv::cvtColor(frame,frame, CV_BGR2GRAY);
                        cv::Mat output = cv::Mat::zeros(frame.rows, frame.cols, CV_8UC1);
                        video->stabilizeImage(frame,output);
                        cv::cvtColor(output,output, CV_GRAY2RGB);

                        glImage = QImage((const unsigned char*)(output.data), output.cols, output.rows, QImage::Format_RGB888);
                    }
                    else
                    {
                        cv::cvtColor(frame,frame, CV_BGR2RGB);
                        glImage = QImage((const unsigned  char*)(frame.data), frame.cols, frame.rows, QImage::Format_RGB888);
                    }

                    scalingFactor = this->width()/vwidth;
                    double scalingFactorH = this->height()/vheight;
                    if (scalingFactorH < scalingFactor)
                        scalingFactor = scalingFactorH;

                    QPainter painter;
                    painter.begin(this);
                    painter.setRenderHint(QPainter::Antialiasing, true);
                    painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
                    painter.translate((this->vwidth/2.0+xCenterOffset)*scalingFactor, (this->vheight/2.0+yCenterOffset)*scalingFactor);

                    if (telemetryData)
                    {
                        // Update scaling factor
                        // adjust scaling to fit both horizontally and vertically
                        //scalingFactor = this->width()/vwidth;


//                        line(frame, Point(0, frame.rows/2.0), Point(frame.cols, frame.rows/2.0), Scalar(255,255,255));
//                        line(frame, Point(frame.cols/2.0, 0), Point(frame.cols/2.0, frame.rows), Scalar(255,255,255));

//                        QPainter painter;
//                        painter.begin(this);
//                        painter.setRenderHint(QPainter::Antialiasing, true);
//                        painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
//                        painter.translate((this->vwidth/2.0+xCenterOffset)*scalingFactor, (this->vheight/2.0+yCenterOffset)*scalingFactor);

                        painter.fillRect(refToScreenX((-vwidth/2.0)), refToScreenY(-vheight/2.0), this->width(), this->height(), Qt::black);
                        painter.drawImage(refToScreenX((-vwidth/2.0)), refToScreenY(-vheight/2.0), glImage.scaled(this->width(), this->height(), Qt::KeepAspectRatio));
                        //painter.fillRect(0, 0, this->width(), this->height(), Qt::black);
                        //painter.drawImage(0,0, glImage.scaled(this->width(), this->height(), Qt::KeepAspectRatio));
                        //painter.drawImage(0,0, glImage.scaled(this->width(), this->height(), Qt::KeepAspectRatioByExpanding));

                        double tempLat = 0;//UASManager::instance()->getHomeLatitude();
                        double tempLon = 0;//UASManager::instance()->getHomeLongitude();
                        //Coordinate* home = new Coordinate(tempLat, tempLon);
                        //Coordinate* position = new Coordinate(lat, lon);
                        //double distance = Geography::DistanceCoordinate(home, position, Geography::KM);

                        QString latitude("Distancia: %1 km");
                        paintText(latitude.arg(0.0f, 4, 'f', 2, '0'), infoColor, 3.0f, (-vwidth/2.0) + 10, -vheight/2.0 + 10, &painter);

                        //delete home, position;

                        QString speed("Bateria: %1 v");
                        paintText(speed.arg(battery, 4, 'f', 2, '0'), infoColor, 3.0f, (-vwidth/2.0) + 80, -vheight/2.0 + 10, &painter);

                        /*if(activeUAS!= NULL)
                        {
                            quint64 filterTime = activeUAS->getUptime() / 1000;
                            int sec = static_cast<int>(filterTime - static_cast<int>(filterTime / 60) * 60);
                            int min = static_cast<int>(filterTime / 60)-((static_cast<int>(filterTime / 60)/60)*60);
                            int hours = static_cast<int>((filterTime / 60)/60);
                            QString timeText;
                            timeText = timeText.sprintf("T. Vuelo %02d:%02d:%02d", hours, min, sec);
                            paintText(timeText, infoColor, 3.0f, (-vwidth/2.0) + 150, -vheight/2.0 + 10, &painter);
                        }*/

                        paintText(mode, infoColor, 3.0f, (-vwidth/2.0) + 10, -vheight/2.0 + 15, &painter);

                        paintText(navigation, infoColor, 3.0f, (-vwidth/2.0) + 80, -vheight/2.0 + 15, &painter);

                        paintText(state, infoColor, 3.0f, (-vwidth/2.0) + 150, -vheight/2.0 + 15, &painter);

                        QString speed4("Latitud: %1 N");
                        paintText(speed4.arg(lat, 4, 'f', 4, '0'), infoColor, 3.0f, (-vwidth/2.0) + 10, vheight/2 - 35, &painter);

                        QString speed5("Longitud: %1 O");
                        paintText(speed5.arg(lon, 4, 'f', 4, '0'), infoColor, 3.0f, (-vwidth/2.0) + 80, vheight/2 - 35, &painter);

                        QString altitude("Altura: %1 m");
                        paintText(altitude.arg(alt, 4, 'f', 2, '0'), infoColor, 3.0f, (-vwidth/2.0) + 150, vheight/2 - 35, &painter);

//                        const float centerWidth = 4.0f;
//                        const float centerCrossWidth = 10.0f;

                        //painter.setPen(defaultColor);
//                        painter.drawLine(QPointF(refToScreenX(-centerWidth / 1.0f), refToScreenY(0.0f)), QPointF(refToScreenX(-centerCrossWidth / 1.0f), refToScreenY(0.0f)));
//                        painter.drawLine(QPointF(refToScreenX(centerWidth / 1.0f), refToScreenY(0.0f)), QPointF(refToScreenX(centerCrossWidth / 1.0f), refToScreenY(0.0f)));
//                        painter.drawLine(QPointF(refToScreenX(0.0f), refToScreenY(-centerWidth / 1.0f)), QPointF(refToScreenX(0.0f), refToScreenY(-centerCrossWidth / 1.0f)));
//                        painter.drawLine(QPointF(refToScreenX(0.0f), refToScreenY(+centerWidth / 1.0f)), QPointF(refToScreenX(0.0f), refToScreenY(+centerCrossWidth / 1.0f)));

                        //drawVerticalIndicator(-90.0f, -60.0f, 120.0f, -90.0f, 90.0f, viewTime(), &painter);
                        //drawHorizontalIndicator(-50.0f, vheight/2 - 15, 120.0f, -180.0f, 180.0f, viewTime(), &painter);

                        painter.end();
                    }
                    else
                    {
//                        double scalingFactorH = this->height()/vheight;
//                        if (scalingFactorH < scalingFactor)
//                            scalingFactor = scalingFactorH;

                        //QPainter painter;
//                        painter.begin(this);
//                        painter.setRenderHint(QPainter::Antialiasing, true);
//                        painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
//                        painter.translate((this->vwidth/2.0+xCenterOffset)*scalingFactor, (this->vheight/2.0+yCenterOffset)*scalingFactor);

                        //painter.fillRect(0, 0, this->width(), this->height(), Qt::black);
                        //painter.drawImage(0,0, glImage.scaled(this->width(), this->height(), Qt::KeepAspectRatio));
                        painter.fillRect(refToScreenX((-vwidth/2.0)), refToScreenY(-vheight/2.0), this->width(), this->height(), Qt::black);
                        painter.drawImage(refToScreenX((-vwidth/2.0)), refToScreenY(-vheight/2.0), glImage.scaled(this->width(), this->height(), Qt::KeepAspectRatio));

                        painter.end();
                    }
                }
            }
        }
        else
        {            
            paintCenterBackground(0, 0, 0);
        }
    }
}

float OverlayData::refLineWidthToPen(float line)
{
    return line * 2.50f;
}

//void OverlayData::drawVerticalIndicator(float xRef, float yRef, float height, float minRate, float maxRate, float value, QPainter* painter)
//{
//    QBrush brush(defaultColor, Qt::NoBrush);
//    painter->setBrush(brush);
//    QPen rectPen(Qt::SolidLine);
//    rectPen.setWidth(0);
//    rectPen.setColor(defaultColor);
//    painter->setPen(rectPen);

//    float scaledValue = value;

//    // Saturate value
//    if (value > maxRate) scaledValue = maxRate;
//    if (value < minRate) scaledValue = minRate;

//    //           x (Origin: xRef, yRef)
//    //           -
//    //           |
//    //           |
//    //           |
//    //           =
//    //           |
//    //   -0.005 >|
//    //           |
//    //           -

//    const float width = height / 8.0f;
//    const float lineWidth = 0.5f;

//    // Indicator lines
//    // Top horizontal line
//    drawLine(xRef-width/4.0f, yRef, xRef+width/4.0f, yRef, lineWidth, defaultColor, painter);
//    // Vertical main line
//    //drawLine(xRef+width/2.0f, yRef, xRef+width/2.0f, yRef+height, lineWidth, defaultColor, painter);
//    drawLine(xRef, yRef, xRef, yRef+height, lineWidth, defaultColor, painter);
//    // Zero mark
//    drawLine(xRef-width/4.0f, yRef+height/2.0f, xRef+width/4.0f, yRef+height/2.0f, lineWidth, defaultColor, painter);
//    // Horizontal bottom line
//    drawLine(xRef-width/4.0f, yRef+height, xRef+width/4.0f, yRef+height, lineWidth, defaultColor, painter);

//    // Text
//    QString label;
//    label.sprintf("< %+06.2f", value);
//    paintText(label, defaultColor, 3.0f, xRef+1.0f, yRef+height-((scaledValue - minRate)/(maxRate-minRate))*height - 1.6f, painter);
//}

//void OverlayData::drawHorizontalIndicator(float xRef, float yRef, float height, float minRate, float maxRate, float value, QPainter* painter)
//{
//    QBrush brush(defaultColor, Qt::NoBrush);
//    painter->setBrush(brush);
//    QPen rectPen(Qt::SolidLine);
//    rectPen.setWidth(0);
//    rectPen.setColor(defaultColor);
//    painter->setPen(rectPen);

//    float scaledValue = -value;

//    // Saturate value
//    if (value > maxRate) scaledValue = maxRate;
//    if (value < minRate) scaledValue = minRate;

//    //                                      v
//    //           x (Origin: xRef, yRef) |-------|-------|

//    const float width = height / 8.0f;
//    const float lineWidth = 0.5f;

//    //line main horizontal
//    drawLine(xRef, yRef, xRef+height, yRef, lineWidth, defaultColor, painter);
//    drawLine(xRef, yRef+width/4.0f, xRef, yRef-width/4.0f, lineWidth, defaultColor, painter);
//    drawLine(xRef+height/2.0f, yRef+width/4.0f, xRef+height/2.0f, yRef-width/4.0f, lineWidth, defaultColor, painter);
//    drawLine(xRef+height, yRef+width/4.0f, xRef+height, yRef-width/4.0f, lineWidth, defaultColor, painter);

//    // Text
//    QString label;
//    label.sprintf("%+06.2f", value);
//    paintText(label, defaultColor, 3.0f, xRef+height-((scaledValue - minRate)/(maxRate-minRate))*height - 4.8f, yRef-width/2.0f - 2.0f, painter);

//    QString label2;
//    label2.sprintf("v", value);
//    paintText(label2, defaultColor, 3.0f, xRef+height-((scaledValue - minRate)/(maxRate-minRate))*height - 1.6, yRef-4.0f, painter);
//}

//void OverlayData::drawLine(float refX1, float refY1, float refX2, float refY2, float width, const QColor& color, QPainter* painter)
//{
//    QPen pen(Qt::SolidLine);
//    pen.setWidth(refLineWidthToPen(width));
//    pen.setColor(color);
//    painter->setPen(pen);
//    painter->drawLine(QPoint(refToScreenX(refX1), refToScreenY(refY1)), QPoint(refToScreenX(refX2), refToScreenY(refY2)));
//}

//void OverlayData::resizeGL(int w, int h)
//{
////    glViewport(0, 0, w, h);
////    glMatrixMode(GL_PROJECTION);
////    glLoadIdentity();
////    paintTelemetry();
//}

void OverlayData::enableTelemetryData(bool enabled)
{
    telemetryData = enabled;
}

void OverlayData::enableStabilizationVideo(bool enabled)
{
    videoStabilizated = enabled;
}

void OverlayData::enableTrackingPosition(bool enabled)
{
    videoTracking = enabled;
}

void OverlayData::enableSendTrackingPosition(bool enabled)
{
    sendTrackingVideo = enabled;
}

void OverlayData::openFile()
{
    QString filename = QFileDialog::getOpenFileName(this, "Abrir Video", this->pathVideo, "Archivos (*.mp4 | *.mpg | *.avi | *.mov)");

    if(!filename.isEmpty())
    {
        setURL(filename);
    }
}

void OverlayData::openRTSP()
{
    bool ok;
    QString filename = QInputDialog::getText(this, tr("Ingrese la URL"), tr("URL RTSP Axis:"), QLineEdit::Normal, tr("rtsp://192.168.1.90:554/axis-media/media.amp"), &ok);

    if (ok && !filename.isEmpty())
    {
        setURL(filename);
    }
}

void OverlayData::record(bool value)
{
    isRecord = value;

    if(!isRecord)
    {
        existFileMovie = false;

        if(fileSubtitles != NULL && fileSubtitles->isOpen())
        {
            isSubTitles = false;
            countSubTitle =0;
            startTime=-1;
            fileSubtitles->close();
            fileSubtitles->flush();
            qDebug()<<"closing file subtittles";
        }
    }

    emit emitRecord(isRecord);
}

void OverlayData::setURL(QString url)
{
    videoStabilizated = false;
    captureVideo.release();

    if(!captureVideo.open(url.toLatin1().data()))
    {
        emit emitTitle("Error al abrir video...");
        return;
    }

    this->urlVideo = url;
    emit emitTitle(urlVideo);
}

void OverlayData::playMovie()
{
    if(!refreshTimer->isActive())
    {
        if(!captureVideo.isOpened())
        {
            if(!captureVideo.open(urlVideo.toLatin1().data()))
            {
                emit emitTitle("Error al abrir video...");
                return;
            }
        }

        refreshTimer->start(updateInterval);

        if(savedAutomatic)
        {
            record(true);
        }
    }
}

void OverlayData::stopMovie()
{
    if(refreshTimer->isActive())
    {
        if(savedAutomatic)
        {
            record(false);
        }

        refreshTimer->stop();      

        videoStabilizated = false;
        captureVideo.release();
    }
}

void OverlayData::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        int xMouse, yMouse;

        xMouse = event->x();
        yMouse = event->y();

        //        qDebug()<<"X: "<<xMouse;
        //        qDebug()<<"Y: "<<yMouse;

        //        qDebug()<<"height: "<<this->size().height();
        //        qDebug()<<"width: "<<this->size().width();

        double x = 480 / (double)this->size().height();
        double y = 704 / (double)this->size().width();

        //        qDebug()<<"Xx: "<<y;
        //        qDebug()<<"Yy: "<<x;

        //        qDebug()<<"Pointx: "<<xMouse*y;
        //        qDebug()<<"Pointy: "<<yMouse*x;

        emit emitPositionTracking(xMouse*y, yMouse*x);
        trackingPoint = cvPoint(xMouse*y, yMouse*x);
        selected = true;

        if(!glImage.isNull())
        {
            emit emitCaptureImage(glImage.copy((xMouse*y)-20, (yMouse*x)-20, sizeAreaInterest, sizeAreaInterest));
        }
    }

    QWidget::mousePressEvent(event);
}

void OverlayData::updateMode(int uas,QString mode,QString description)
{
    Q_UNUSED(uas);
    Q_UNUSED(description);
    this->mode = mode;
}

void OverlayData::updateModeNavigation(int uasid, int mode, const QString &text)
{
    Q_UNUSED(uasid);
    Q_UNUSED(mode);
    this->navigation = "NAVEGACION "+text;
}

void OverlayData::setAlertBattery(int id, double battery)
{
    Q_UNUSED(id);

    this->battery = battery;
}

void OverlayData::setAlertAirSpeed(double airSpeed)
{
    this->airSpeed = airSpeed;
}

void OverlayData::updateAltitude(double z)
{
    this->alt = z;
}

void OverlayData::changePATH(const QString &path)
{
    this->pathVideo = path+"/";
}

quint64 OverlayData::getGroundTimeNow()
{
    QDateTime time = QDateTime::currentDateTime();
    time = time.toUTC();
    quint64 milliseconds = time.toTime_t() * static_cast<quint64>(1000);
    return static_cast<quint64>(milliseconds + time.time().msec());
}

void OverlayData::createFileSubTitles(QString file)
{
    QString fileName(file);

    if (fileName.isEmpty())
        return;

    QString tempFile(QFileInfo(fileName).baseName());
    QString tempPath(QFileInfo(fileName).dir().path()+"/");

    if(fileSubtitles== NULL)
    {
        fileSubtitles = new QFile();
    }

    if(fileSubtitles->fileName() != tempPath + "/" + tempFile + ".srt")
    {
        if(!QFileInfo(tempPath+"/"+tempFile+".srt").exists())
        {
            fileSubtitles->setFileName(file+".srt");
            fileSubtitles->open(QIODevice::WriteOnly | QIODevice::Append);
        }
    }

    this->isSubTitles = true;
}

void OverlayData::refreshTimeOut()
{
    if(isSubTitles)
    {
        if(fileSubtitles->exists())
        {
            QTextStream streamData(fileSubtitles);

            if(startTime== (quint64)-1)
            {
                startTime = getGroundTimeNow();
            }

            quint64 filterTime = (getGroundTimeNow()-startTime)/1000;
            int sec = static_cast<int>(filterTime - static_cast<int>(filterTime / 60) * 60);
            int min = static_cast<int>(filterTime / 60)-((static_cast<int>(filterTime / 60)/60)*60);
            int hours = static_cast<int>((filterTime / 60)/60);
            QString timeText1;
            timeText1 = timeText1.sprintf("%02d:%02d:%02d", hours, min, sec);

            streamData
                    << countSubTitle << "\r\n"
                    << timeText1 << " <i> Lat: "<<this->lat<<" - Lon: "<<this->lon<<" - Alt: "<<this->alt<<"</i> \r\n"
                    << "\r\n\n";

            countSubTitle++;
        }
    }

    if(sendTrackingVideo)
    {
        if(moveTracking == secondTracking)
        {
            if(abs(videoCenter.x - trackingPoint.x) > distanceCenter || abs(videoCenter.y - trackingPoint.y) > distanceCenter)
            {
                emit emitDistanceToCenter(videoCenter.x - trackingPoint.x, videoCenter.y - trackingPoint.y);
            }

            moveTracking = 0;
        }

        moveTracking++;
    }
}

void OverlayData::initializeTracking()
{
    drawDescriptors = false;
    useBruteMatch = false;
    useKalman = true;
    drawCenter = true;
    selected = false;
    moveTracking = 0;

    //Kalman
    KalmanFilter KF2(4, 2, 0);
    kalmaFilter = KF2;
    kalmaFilter.transitionMatrix = *(Mat_<float>(4, 4) << 1,0,1,0,   0,1,0,1,  0,0,1,0,  0,0,0,1);
    Mat_<float> measurement2(2,1);
    measurement = measurement2;
    measurement.setTo(Scalar(0));

    if(useKalman)
    {
        // init...
        kalmaFilter.statePre.at<float>(0) = trackingPoint.x;
        kalmaFilter.statePre.at<float>(1) = trackingPoint.y;
        kalmaFilter.statePre.at<float>(2) = trackingPoint.x;
        kalmaFilter.statePre.at<float>(3) = trackingPoint.y;
        setIdentity(kalmaFilter.measurementMatrix);
        setIdentity(kalmaFilter.processNoiseCov, Scalar::all(1e-4));
        setIdentity(kalmaFilter.measurementNoiseCov, Scalar::all(1e-1));
        setIdentity(kalmaFilter.errorCovPost, Scalar::all(.1));
    }
    //kalman

    FeatureMethod = "SURF"; // use SIFT or SURF

    // Create smart pointer for SURF feature detector for Main Video.
    featureDetectSearch = FeatureDetector::create(FeatureMethod);

    // Create smart pointer for SURF feature detector for Object.
    featureDetectInterest = FeatureDetector::create(FeatureMethod);

    sizeAreaInterest = 100;
    sizeAreaSearch = 150;

    minimumMatchesSearch = 3;
    refreshSearch = 0;
    frameRefreshSearch = 20;
}

void OverlayData::processTracking()
{
    if(!areaInterest.empty() && tracking)
    {
        try
        {
            videoCenter = Point(frame.cols/2.0,frame.rows/2.0);


            Rect RectAreaInteres = Rect(trackingPoint.x-sizeAreaSearch/2.0, trackingPoint.y -sizeAreaSearch/2.0,
                                        sizeAreaSearch, sizeAreaSearch);

            areaSearch = frame(RectAreaInteres);

            rectangle(frame, RectAreaInteres, Scalar(255,0,0));
            //rectangle(outputFrame, Rect(selectedPoint.x - 50, selectedPoint.y - 50, 100, 100), Scalar(100,100,0));
            rectangle(frame, Rect(trackingPoint.x - 50, trackingPoint.y - 50, 100, 100), Scalar(100,100,0));

            // Detect the keypoints
            featureDetectSearch->detect(areaSearch, keyPointsSearch);
            // NOTE: featureDetector is a pointer hence the '->'.
            //Similarly, we create a smart pointer to the SIFT extractor.
            Ptr<DescriptorExtractor> featureExtractor = DescriptorExtractor::create(FeatureMethod);
            // Compute the 128 dimension SIFT descriptor at each keypoint.
            // Each row in "descriptors" correspond to the SIFT descriptor for each keypoint

            featureExtractor->compute(areaSearch, keyPointsSearch, descriptorsSearch);

            if(drawDescriptors)
            {
                // If you would like to draw the detected keypoint just to check
                Scalar keypointColor = Scalar(0, 255, 0);     // Blue keypoints.
                drawKeypoints(areaSearch, keyPointsSearch, areaSearch, keypointColor, DrawMatchesFlags::DEFAULT);
            }

            if(useKalman)
            {
                kalmaFilter.predict();

                // Get mouse point
                measurement(0) = trackingPoint.x;
                measurement(1) = trackingPoint.y;

                // The "correct" phase that is going to use the predicted value and our measurement
                Mat estimated = kalmaFilter.correct(measurement);
                statePoint.x = estimated.at<float>(0);
                statePoint.y = estimated.at<float>(1);

                DrawCrossHair(frame, statePoint, 10, Scalar(255,255,255));
            }

            BruteForceMatcher<L2<float> > matcher;

            std::vector<DMatch> good_matches;

            if(useBruteMatch)
            {
                matcher.match(descriptorsInterest, descriptorsSearch,  matches);

                if(drawDescriptors)
                {
                    Mat goodmatchesBrute;

                    drawMatches(areaSearch, keyPointsSearch, areaInterest, keyPointsInterest, matches, goodmatchesBrute, Scalar::all(-1),
                                Scalar::all(-1),vector<char>(),DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

                    imshow("Good Matches", goodmatchesBrute);
                }
            }
            else
            {
                FlannBasedMatcher flanMatcher;
                std::vector<DMatch> flanMatches;

                try
                {
                    flanMatcher.match(descriptorsInterest, descriptorsSearch, flanMatches);

                    double max_dist = 0;
                    double min_dist = 100;

                    //quick calculation of max and min distances between keypoints
                    for (int i = 0; i <descriptorsInterest.rows; i++)
                    {
                        double dist = flanMatches[i].distance;
                        if(dist < min_dist) min_dist = dist;
                        if(dist > max_dist) max_dist = dist;
                    }

                    for (int i = 0; i < descriptorsInterest.rows; i++)
                    {
                        if (flanMatches[i].distance < 2*min_dist)
                        {
                            good_matches.push_back(flanMatches[i]);
                        }
                    }

                    realMatchesUsed = good_matches;

                    if(drawDescriptors)
                    {
                        Mat goodmatches;

                        drawMatches(areaSearch,keyPointsSearch, areaInterest, keyPointsInterest, good_matches, goodmatches, Scalar::all(-1),
                                    Scalar::all(-1),vector<char>(),DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

                        imshow("Good Matches", goodmatches);
                    }
                }
                catch(...)
                {
                    printf("FALLO drawMatches");
                }
            }

            if (realMatchesUsed.size() > 0)
            {
                int contador = 0;
                float ObjPosX = 0.0;
                float ObjPosY = 0.0;

                if(useBruteMatch)
                {
                    for (int i = 0; i < (int)matches.size(); i++)
                    {
                        Point lugar(keyPointsSearch[matches[i].trainIdx].pt.x,
                                    keyPointsSearch[matches[i].trainIdx].pt.y );

                        if(drawDescriptors)
                        {
                            circle(areaSearch,lugar,5,Scalar(255,255,255),1);
                        }

                        ObjPosX += keyPointsSearch[matches[i].trainIdx].pt.x;
                        ObjPosY += keyPointsSearch[matches[i].trainIdx].pt.y;

                        contador++;
                    }
                }
                else
                {
                    for (int i = 0; i < (int)good_matches.size(); i++)
                    {

                        Point lugar(keyPointsSearch[good_matches[i].trainIdx].pt.x,
                                    keyPointsSearch[good_matches[i].trainIdx].pt.y );

                        if(drawDescriptors)
                        {
                            circle(areaSearch,lugar,5,Scalar(255,255,255),1);
                        }

                        ObjPosX += keyPointsSearch[good_matches[i].trainIdx].pt.x;
                        ObjPosY += keyPointsSearch[good_matches[i].trainIdx].pt.y;

                        contador++;
                    }
                }

                if (!useBruteMatch)
                    minimumMatchesSearch = 1;
                else
                    minimumMatchesSearch = 3;

                if (contador >= minimumMatchesSearch) // number of times good findings are got
                {
                    DrawCrossHair(areaSearch,
                                  Point((ObjPosX/contador), (ObjPosY/contador)),
                                  10,
                                  Scalar(0,0,255));

                    trackingPoint.x = (ObjPosX/contador) + (trackingPoint.x - sizeAreaSearch/2.0);
                    trackingPoint.y = (ObjPosY/contador) + (trackingPoint.y - sizeAreaSearch/2.0);

                    if((trackingPoint.x - sizeAreaSearch/2) <= 0) trackingPoint.x = 0 + sizeAreaSearch/2;
                    if((trackingPoint.x + sizeAreaSearch/2) >= frame.cols) trackingPoint.x = frame.cols - sizeAreaSearch/2;

                    if((trackingPoint.y - sizeAreaSearch/2) <= 0) trackingPoint.y = 0 + sizeAreaSearch/2;
                    if((trackingPoint.y + sizeAreaSearch/2) >= frame.rows) trackingPoint.y = frame.rows - sizeAreaSearch/2;

                    refreshSearch++;

                    if ((refreshSearch >= frameRefreshSearch) && (contador > minimumMatchesSearch))
                    {
                        selected = true;
                        refreshSearch = 0;

                        if(useKalman)
                        {
                            trackingPoint.x = statePoint.x;
                            trackingPoint.y = statePoint.y;

                            if((trackingPoint.x - sizeAreaSearch/2) <= 0) trackingPoint.x = 0 + sizeAreaSearch/2;
                            if((trackingPoint.x + sizeAreaSearch/2) >= frame.cols) trackingPoint.x = frame.cols - sizeAreaSearch/2;

                            if((trackingPoint.y - sizeAreaSearch/2) <= 0) trackingPoint.y = 0 + sizeAreaSearch/2;
                            if((trackingPoint.y + sizeAreaSearch/2) >= frame.rows) trackingPoint.y = frame.rows - sizeAreaSearch/2;
                        }

                        //qDebug()<<"REFRESCO OBJECTO";
                    }
                }
                else
                {
                    //qDebug()<<" - - - - - - - -WARNING!!!! LOST OBJECT - - - - - - - \n";
                    tracking = false;
                }
            }

            if(useBruteMatch)
            {
                for (int i=0;i<(int)matches.size();i++)
                {
                    realMatchesUsed.push_back(matches[i]);
                    realMatchesUsed.size();
                }
            }            
        }
        catch(...)
        {
            tracking = false;
        }
    }
}

void OverlayData::setSavedAutomatic(bool automatic)
{
    savedAutomatic = automatic;

    if(!savedAutomatic && isRecord)
    {
        record(false);
    }
}
