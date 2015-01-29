    #include "OpenCVWidget.h"
#include "ui_OpenCVWidget.h"

OpenCVWidget::OpenCVWidget(QWidget *parent) :
        QWidget(parent),        
        ui(new Ui::OpenCVWidget)
{
    ui->setupUi(this);

    OverlayData* overlayData = new OverlayData(300, 300, this);
    QHBoxLayout* hlButtons = new QHBoxLayout();
    btPlay = new QPushButton(QIcon(":/imagenes/Play.png"), "", this);
    btStop = new QPushButton(QIcon(":/imagenes/Stop.png"), "", this);
    btFile = new QPushButton(QIcon(":/imagenes/Open.png"), "", this);
    btRTSP = new QPushButton(QIcon(":/imagenes/Radio.png"), "", this);
    btRecord = new QPushButton(QIcon(":/imagenes/Record.png"), "", this);
    btRecord->setCheckable(true);
    btRecord->setChecked(false);
    btRecord->setVisible(false);

    lbTitle = new QLabel("---");
    lbTitle->setMaximumHeight(15);

    hlButtons->addWidget(btPlay);
    hlButtons->addWidget(btStop);
    hlButtons->addWidget(btRTSP);
    hlButtons->addWidget(btFile);    
    hlButtons->addWidget(btRecord);

    connect(btPlay, SIGNAL(clicked()), overlayData, SLOT(playMovie()));
    connect(btStop, SIGNAL(clicked()), overlayData, SLOT(stopMovie()));
    connect(btFile, SIGNAL(clicked()), overlayData, SLOT(openFile()));
    connect(btRTSP, SIGNAL(clicked()), overlayData, SLOT(openRTSP()));
    connect(btRecord, SIGNAL(clicked(bool)), overlayData, SLOT(record(bool)));
    connect(overlayData, SIGNAL(emitCaptureImage(QImage)), this, SLOT(showCaptureImage(QImage)));
    connect(overlayData, SIGNAL(emitTitle(QString)), lbTitle, SLOT(setText(QString)));
    connect(this, SIGNAL(emitChangePath(QString)), overlayData, SLOT(changePATH(QString)));
    connect(overlayData, SIGNAL(emitRecord(bool)), this, SLOT(changeStatusRecord(bool)));
    connect(overlayData, SIGNAL(emitDistanceToCenter(int,int)), this, SIGNAL(emitDistanceToCenter(int,int)));
    connect(this, SIGNAL(setSavedAutomatic(bool)), overlayData, SLOT(setSavedAutomatic(bool)));

    QVBoxLayout* vlControls = new QVBoxLayout();

    vlControls->addWidget(overlayData);
    vlControls->addLayout(hlButtons);
    vlControls->addWidget(lbTitle);
    vlControls->setContentsMargins(10, 2, 10, 2);

    setLayout(vlControls);

    btRecord->setStyleSheet("background-color: rgb(11, 255, 0); border-color: rgb(10, 10, 10)");

    setWindowTitle("VIDEO");
}

OpenCVWidget::~OpenCVWidget()
{
    delete ui;
}

void OpenCVWidget::showCaptureImage(QImage img)
{
    Q_UNUSED(img);    
}

void OpenCVWidget::changePATH(const QString &path)
{
    emit emitChangePath(path);
}

void OpenCVWidget::changeStatusRecord(bool record)
{
    if(record)
    {
        btRecord->setStyleSheet("background-color: rgb(255, 5, 0); border-color: rgb(10, 10, 10)");
    }
    else
    {
        btRecord->setStyleSheet("background-color: rgb(11, 255, 0); border-color: rgb(10, 10, 10)");
    }
}

void OpenCVWidget::savedAutomatic(bool automatic)
{
    if(automatic)
    {
        btRecord->setVisible(false);
    }
    else
    {
        btRecord->setVisible(true);
    }

    setSavedAutomatic(automatic);
}
