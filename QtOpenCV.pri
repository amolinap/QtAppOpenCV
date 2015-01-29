QMAKE_POST_LINK += echo "Copying files"

    QMAKE_POST_LINK += && cp -r /opt/local/lib/libopencv_core.dylib $$TARGETDIR/QtOpenCV.app/Contents/MacOS/lib
    QMAKE_POST_LINK += && cp -r /opt/local/lib/libopencv_highgui.dylib $$TARGETDIR/QtOpenCV.app/Contents/MacOS/lib
    QMAKE_POST_LINK += && cp -r /opt/local/lib/libopencv_imgproc.dylib $$TARGETDIR/QtOpenCV.app/Contents/MacOS/lib
