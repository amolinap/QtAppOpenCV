QMAKE_POST_LINK += echo "Copying files"

    QMAKE_POST_LINK += && cp -r /usr/local/opt/opencv@2/lib/libopencv_core.dylib $$TARGETDIR/QtOpenCV.app/Contents/MacOS/lib
    QMAKE_POST_LINK += && cp -r /usr/local/opt/opencv@2/lib/libopencv_highgui.dylib $$TARGETDIR/QtOpenCV.app/Contents/MacOS/lib
    QMAKE_POST_LINK += && cp -r /usr/local/opt/opencv@2/lib/libopencv_imgproc.dylib $$TARGETDIR/QtOpenCV.app/Contents/MacOS/lib
