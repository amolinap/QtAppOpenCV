#include "videoStabilizer.h"
#include <QDebug>
#include <iostream>
#include <QTimer>
#include <sys/times.h>
#include <unistd.h>
#include <imgproc/imgproc.hpp>
#include <core/types_c.h>


videoStabilizer::videoStabilizer(QRect videoSize, QObject *parent):
        QObject(parent),
        currentGrayCodeIndex(0),
        searchFactorWindow(SEARCH_FACTOR_P/2),
        vSearchOffset(videoSize.height()/2),
        hSearchOffset(videoSize.width()/2)
        //    #if USE_OPENCV
        //    duration(0.0),
        //    averageTime(0.0),
        //    aveCount(0)
        //  #else
        //    timerTicks(0),
        //    ticksPerSecond(sysconf(_SC_CLK_TCK))

        //  #endif
{
#if USE_OPENCV
    duration = 0.0;
    averageTime = 0.0;
    aveCount = 0;
#else
    timerTicks = 0;
    ticksPerSecond = sysconf(_SC_CLK_TCK);

#endif
    //TODO: Validate height and width > 0 and less than a sensible number
    videoHeight = videoSize.height();
    videoWidth  = videoSize.width();

    allocateAndInitialize();

    computeSearchWindows();

#if DO_FULL_CORRELATION
    for (uchar subframe = 0; subframe < 4; subframe++) {
        for (int m = 0; m < 2*SEARCH_FACTOR_P+1; m++ ){
            for (int n = 0; n < 2*SEARCH_FACTOR_P+1; n++){
                fullCorrelationMatrix[subframe][n][m].m = m -SEARCH_FACTOR_P;
                fullCorrelationMatrix[subframe][n][m].n = n -SEARCH_FACTOR_P;
                fullCorrelationMatrix[subframe][n][m].value = 0;
            }
        }
    }

#else

    for (uint subframe = 0; subframe < 4; subframe++){

        computeCorrelationLocations( subframe,
                                     subframeLocations[subframe].lx - searchFactorWindow,
                                     subframeLocations[subframe].ly - searchFactorWindow,
                                     0,
                                     searchFactorWindow,
                                     searchFactorWindow);
    }

#endif


}

videoStabilizer::~videoStabilizer(){
#if USE_OPENCV
    imageMatrix.release();
    grayCodeMatrix[0].release();
    grayCodeMatrix[1].release();

#else
    for (int ii = 0; ii < videoHeight; ii++){
        delete imageMatrix[ii];
    }
#endif
}

void videoStabilizer::allocateAndInitialize(){
    // Initialize each subframe correlation vectors
    for (uint subframe = 0; subframe < 4; subframe++) {
        memset(correlationMatrix[subframe],0, sizeof(tcorrMatElement)*18);
    }

#if USE_OPENCV
    /// allocate the memory of all the Matrices
    grayCodeMatrix[0] = cv::Mat::zeros(videoHeight, videoWidth, CV_8UC1);
    grayCodeMatrix[1] = cv::Mat::zeros(videoHeight, videoWidth, CV_8UC1);

    //imageMatrix = cv::Mat(videoHeight, videoWidth, CV_8UC1);

#else
    /// allocate the memory of all the Matrices
    grayCodeMatrix[0].resize(videoHeight);
    grayCodeMatrix[1].resize(videoHeight);

    imageMatrix.resize(videoHeight);

    for (int ii = 0; ii < videoHeight; ii++){
        grayCodeMatrix[0][ii].resize(videoWidth);
        grayCodeMatrix[1][ii].resize(videoWidth);
        imageMatrix[ii] = new uchar[videoWidth];
    }

#endif

    // Clear the motion vectors
    memset(&vg_tm1, 0, sizeof(tcorrMatElement));
    memset(&va_tm1, 0, sizeof(tcorrMatElement));
    memset(&va, 0, sizeof(tcorrMatElement));
}

void videoStabilizer::computeSearchWindows (){
    /// Compute the search windows
    // UL
    subframeLocations[0].lx = hSearchOffset/2 - HORIZ_WINDOW_M/2;
    subframeLocations[0].rx = hSearchOffset/2 + HORIZ_WINDOW_M/2;
    subframeLocations[0].ly = vSearchOffset/2 - VERT_WINDOW_N/2;
    subframeLocations[0].ry = vSearchOffset/2 + VERT_WINDOW_N/2;
    // UR
    subframeLocations[1].lx = hSearchOffset/2 - HORIZ_WINDOW_M/2 + hSearchOffset;
    subframeLocations[1].rx = hSearchOffset/2 + HORIZ_WINDOW_M/2 + hSearchOffset;
    subframeLocations[1].ly = vSearchOffset/2 - VERT_WINDOW_N/2;
    subframeLocations[1].ry = vSearchOffset/2 + VERT_WINDOW_N/2;
    // LL
    subframeLocations[2].lx = hSearchOffset/2 - HORIZ_WINDOW_M/2;
    subframeLocations[2].rx = hSearchOffset/2 + HORIZ_WINDOW_M/2;
    subframeLocations[2].ly = vSearchOffset/2 - VERT_WINDOW_N/2 + vSearchOffset;
    subframeLocations[2].ry = vSearchOffset/2 + VERT_WINDOW_N/2 + vSearchOffset;
    // LR
    subframeLocations[3].lx = hSearchOffset/2 - HORIZ_WINDOW_M/2 + hSearchOffset;
    subframeLocations[3].rx = hSearchOffset/2 + HORIZ_WINDOW_M/2 + hSearchOffset;
    subframeLocations[3].ly = vSearchOffset/2 - VERT_WINDOW_N/2 + vSearchOffset;
    subframeLocations[3].ry = vSearchOffset/2 + VERT_WINDOW_N/2 + vSearchOffset;
}

void videoStabilizer::computeCorrelationLocations(uint subframe,
                                                  uint seedX,
                                                  uint seedY,
                                                  uint index,
                                                  uint hFactor,
                                                  uint vFactor){

    for (uint yy = 0; yy < 3; yy++){
        for(uint xx = 0; xx < 3; xx++){
            correlationMatrix[subframe][index].x = seedX  + (xx*hFactor);
            correlationMatrix[subframe][index].y = seedY  + (yy*vFactor);

            correlationMatrix[subframe][index].m   = correlationMatrix[subframe][index].x - subframeLocations[subframe].lx;
            correlationMatrix[subframe][index].n = correlationMatrix[subframe][index].y - subframeLocations[subframe].ly;
            index++;
        }
    }
}

#if USE_OPENCV
void videoStabilizer::stabilizeImage(const cv::Mat &imageSrc, cv::Mat &imageDest){

    double tempDuration = static_cast<double>(cv::getTickCount());
    static double tickFreq = static_cast<double>(cv::getTickFrequency());
#else
    void videoStabilizer::stabilizeImage(QImage* imageSrc, QImage* imageDest){


        static long ticks;
        static uchar aveCount = 0;
        static struct tms timeVal;
        static clock_t st_time;
        static clock_t en_time;

        st_time = times(&timeVal);
        ticks = timeVal.tms_stime;
#endif

        convertImageToMatrix(imageSrc);
        getGrayCode();
        computeCorrelation();
        findMotionVector();

#if USE_OPENCV
        populateImageResult(imageDest);

        duration += (static_cast<double>(cv::getTickCount()) - tempDuration);
        aveCount++;

        if (aveCount == 10){
            averageTime = (duration/aveCount)/tickFreq;
            aveCount = 0;
            duration = 0;
            emit gotDuration(averageTime);
        }

        imageMatrix.release();
#else
        populateImageResult(imageDest);

        if (aveCount == 10){
            averageTime = (uint)(timerTicks)/(10);
            aveCount = 0;
            timerTicks = 0;
        }

        aveCount++;
        en_time = times(&timeVal);
        timerTicks += (en_time - st_time);
#endif

        currentGrayCodeIndex^=1;

    }

    void videoStabilizer::getAverageProcessTime(uint *timeInMs){

        *timeInMs = averageTime;
    }

#if USE_OPENCV
    inline void videoStabilizer::convertImageToMatrix(const cv::Mat &imageSrc){
        imageMatrix = imageSrc;
    }
#else
    void videoStabilizer::convertImageToMatrix(QImage* imageSrc){
        for (int ii=0; ii < videoHeight; ii++){
            imageMatrix[ii] = imageSrc->scanLine(ii);
        }

    }

#endif

    void videoStabilizer::getGrayCode(){

        for (int subframe = 0; subframe < 4; subframe++){
            getSubframeGrayCode(subframe);
        }
    }


    void videoStabilizer::getSubframeGrayCode (uchar subframe, BIT_PLANES bitPlane){
        for (uint jj = subframeLocations[subframe].ly - SEARCH_FACTOR_P;
             jj < subframeLocations[subframe].ry + SEARCH_FACTOR_P;
             jj++){

#if USE_OPENCV
            uchar* gcData= grayCodeMatrix[currentGrayCodeIndex].ptr<uchar>(jj);
            uchar* imData= imageMatrix.ptr<uchar>(jj);
#endif
            for (uint ii = subframeLocations[subframe].lx - SEARCH_FACTOR_P;
                 ii < subframeLocations[subframe].rx + SEARCH_FACTOR_P;
                 ii++){
#if USE_OPENCV
                //            *(gcData + ii) =  getByteGrayCode( *(imData + ii), bitPlane) == 0? 0: 255;
                *(gcData + ii) =  getByteGrayCode( *(imData + ii), bitPlane);
#else
                grayCodeMatrix[currentGrayCodeIndex][jj].setBit(ii, getByteGrayCode(imageMatrix[jj][ii], bitPlane));
#endif
            }
        }
    }


    inline bool videoStabilizer::getByteGrayCode (uchar value, BIT_PLANES bitPlane){

        switch (bitPlane){
        case GC_BP_3:
            return (bool) (((value & GC_BP_3)>>3) ^ ((value & GC_BP_4)>>4));
            break;
    case GC_BP_4:
            return (bool) (((value & GC_BP_4)>>4) ^ ((value & GC_BP_5)>>5));
            break;
    case GC_BP_5:
            return (bool) (((value & GC_BP_5)>>5) ^ ((value & GC_BP_6)>>6));
            break;
    case GC_BP_6:
    default:
            return (bool) (((value & GC_BP_6)>>6) ^ ((value & GC_BP_7)>>7));
            break;
        }


    }

#if USE_OPENCV
    void videoStabilizer::populateImageResult(cv::Mat &imageDest){

        //imageDest = grayCodeMatrix[currentGrayCodeIndex];

        for (uint ii = (uint)LMAX(0, va.n); ii < (uint)LMIN(videoHeight, videoHeight+va.n); ii ++){

            uchar* deData= imageDest.ptr<uchar>(ii);
            uchar* srData= imageMatrix.ptr<uchar>(ii - va.n);

            for (uint jj = (uint)LMAX(0, va.m); jj < (uint)LMIN (videoWidth, videoWidth+va.m); jj++){
                *(deData + jj) = *(srData + jj - va.m);
            }// for jj
        } // for ii

    }
#else
    void videoStabilizer::populateImageResult(QImage* imageDest){
        imageDest->fill(Qt::black);

        for (uint ii = MAX(0, va.n); ii < MIN(videoHeight, videoHeight+va.n); ii ++){
            for (uint jj = MAX(0, va.m); jj < MIN (videoWidth, videoWidth+va.m); jj++){
                imageDest->setPixel(jj,ii,imageMatrix[ii- va.n][jj - va.m]);
            }// for jj
        } // for ii
    }
#endif

    void videoStabilizer::computeCorrelation(){

        uchar t_m1 = currentGrayCodeIndex ^ 1;
        memset(localMinima,0,4*sizeof(tcorrMatElement));

        for (uchar subframe = 0; subframe < 4; subframe++) {

#if DO_FULL_CORRELATION

            computeFullCorrelation(subframe, t_m1);
#else

            computeSubframeCorrelation(0, subframe, t_m1);

            computeCorrelationLocations(subframe,
                                        localMinima[subframe].x - searchFactorWindow,
                                        localMinima[subframe].y - searchFactorWindow,
                                        9,
                                        searchFactorWindow,
                                        searchFactorWindow);

            // TODO: to enable this I need new matrices for each bitplane xor
            //getSubframeGrayCode(subframe,GC_BP_4);

            computeSubframeCorrelation(9,subframe,t_m1);
#endif
        }
    }

    void videoStabilizer::computeSubframeCorrelation (uint index, uchar subframe, uchar t_m1){

        localMinima[subframe].value = UINT_MAX;

        for (uint corrIndex = index; corrIndex < (index + 9); corrIndex++){
            correlationMatrix[subframe][corrIndex].value = 0;

            computeSingleCorrelation(subframe, t_m1,&correlationMatrix[subframe][corrIndex]);

            // find the minimimum
            if (localMinima[subframe].value > correlationMatrix[subframe][corrIndex].value){
                memcpy(&localMinima[subframe], &(correlationMatrix[subframe][corrIndex]), sizeof(tcorrMatElement));
            } // if localMinima
        }
    }


    void videoStabilizer::computeFullCorrelation (uchar subframe, uchar tm_1){

        localMinima[subframe].value = UINT_MAX;

        for (int m = 0; m < 2*SEARCH_FACTOR_P+1; m++ ){
            for (int n = 0; n < 2*SEARCH_FACTOR_P+1; n++){
                fullCorrelationMatrix[subframe][n][m].value = 0;

                computeSingleCorrelation(subframe,tm_1,&fullCorrelationMatrix[subframe][n][m]);

                // find the minimimum
                if (localMinima[subframe].value > fullCorrelationMatrix[subframe][n][m].value){
                    memcpy(&localMinima[subframe], &(fullCorrelationMatrix[subframe][n][m]), sizeof(tcorrMatElement));
                } // if localMinima
            }
        }
    }

    inline void videoStabilizer::computeSingleCorrelation (uchar subframe, uchar t_m1, tcorrMatElement* element){
        for (uint y = subframeLocations[subframe].ly;
             y < subframeLocations[subframe].ry;
             y++) {  // y is height
#if USE_OPENCV
            uchar* data     = grayCodeMatrix[currentGrayCodeIndex].ptr<uchar>(y);
            uchar* data_tm1 = grayCodeMatrix[t_m1].ptr<uchar>( y + element->n);
#endif

            for (uint x = subframeLocations[subframe].lx;
                 x < subframeLocations[subframe].rx;
                 x++) {     // x is width
#if USE_OPENCV
                element->value += *(data + x) ^ *(data_tm1 + x + element->m);
#else
                element->value += grayCodeMatrix[currentGrayCodeIndex][y].testBit(x) ^
                                  grayCodeMatrix[t_m1][y+element->n].testBit(x+element->m);

#endif
            }
        }
    }

    void videoStabilizer::findMotionVector (){

        int sortedMinimaM[5];
        int sortedMinimaN[5];

        for (int x = 0; x < 4; x++) {
            sortedMinimaM[x] = localMinima[x].m;
            sortedMinimaN[x] = localMinima[x].n;
        }
        sortedMinimaM[4] = vg_tm1.m;
        sortedMinimaN[4] = vg_tm1.n;


        sortLocalMinima(sortedMinimaM, 0, 5);
        sortLocalMinima(sortedMinimaN, 0, 5);

        va.m = PAN_FACTOR_D*va_tm1.m + sortedMinimaM[2];
        va.n = PAN_FACTOR_D*va_tm1.n + sortedMinimaN[2];

        va.m = va.m > MAX_M_MOTION ? MAX_M_MOTION : va.m;
        va.m = va.m < -MAX_M_MOTION ? -MAX_M_MOTION : va.m;

        va.n = va.n > MAX_N_MOTION ? MAX_N_MOTION : va.n;
        va.n = va.n < -MAX_N_MOTION ? -MAX_N_MOTION : va.n;

        vg_tm1.m = sortedMinimaM[2];
        vg_tm1.n = sortedMinimaN[2];

        memcpy(&va_tm1, &va, sizeof(tcorrMatElement));

    }

    void videoStabilizer::sortLocalMinima (int* sortedMinima, char beg, char end){
        /**

Source:  http://alienryderflex.com/quicksort/

void swap(int *a, int *b) {
  int t=*a; *a=*b; *b=t;
}
void sort(int arr[], int beg, int end) {
  if (end > beg + 1) {
    int piv = arr[beg], l = beg + 1, r = end;
    while (l < r) {
      if (arr[l] <= piv)
        l++;
      else
        swap(&arr[l], &arr[--r]);
    }
    swap(&arr[--l], &arr[beg]);
    sort(arr, beg, l);
    sort(arr, r, end);
  }
}

*/
        if (end > beg + 1){
            int piv = sortedMinima[beg];
            uchar l = beg + 1, r = end;

            while (l < r){
                if (sortedMinima[l] <= piv){
                    l++;
                } else {
                    swap(&sortedMinima[l], &sortedMinima[--r]);
                }
            }
            swap(&sortedMinima[--l], &sortedMinima[beg]);
            sortLocalMinima(sortedMinima, beg, l);
            sortLocalMinima(sortedMinima, r, end);
        }

    }

    inline void videoStabilizer::swap(int* a, int* b ){
        int t;
        t = *a;
        *a = *b;
        *b =t;
    }
