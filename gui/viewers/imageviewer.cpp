#include "imageviewer.h"

// TODO: split into ImageViewerPrivate

ImageViewer::ImageViewer(QWidget *parent) : QWidget(parent),
    image(NULL),
    animation(NULL),
    isDisplaying(false),
    mouseWrapping(false),
    checkboardGridEnabled(false),
    expandImage(false),
    smoothAnimatedImages(true),
    mCurrentScale(1.0),
    fitWindowScale(0.125),
    minScale(0.125),
    maxScale(maxScaleLimit),
    imageFitMode(FIT_ORIGINAL)
{
    this->setMouseTracking(true);
    posAnimation = new QPropertyAnimation(this, "drawPos");
    posAnimation->setEasingCurve(QEasingCurve::OutCubic);
    posAnimation->setDuration(animationSpeed);
    animationTimer = new QTimer(this);
    animationTimer->setSingleShot(true);
    cursorTimer = new QTimer(this);
    readSettings();
    connect(settings, SIGNAL(settingsChanged()),
            this, SLOT(readSettings()));
    connect(cursorTimer, SIGNAL(timeout()),
            this, SLOT(hideCursor()),
            Qt::UniqueConnection);
    desktopSize = QApplication::desktop()->size();
}

ImageViewer::~ImageViewer() {
}

void ImageViewer::startAnimation() {
    if(animation) {
        stopAnimation();
        connect(animationTimer, SIGNAL(timeout()), this, SLOT(nextFrame()));
        startAnimationTimer();
    }
}

void ImageViewer::stopAnimation() {
    if(animation) {
        animationTimer->stop();
        disconnect(animationTimer, SIGNAL(timeout()), this, SLOT(nextFrame()));
        animation->jumpToFrame(0);
    }
}
void ImageViewer::nextFrame() {
    if(animation) {
        if(!animation->jumpToNextFrame()) {
            animation->jumpToFrame(0);
        }
        QPixmap *newFrame = new QPixmap();
        *newFrame = animation->currentPixmap();
        if(animation->frameCount() > 1) {
            startAnimationTimer();
        }
        updateFrame(newFrame);
    }
}

void ImageViewer::startAnimationTimer() {
    if(animationTimer && animation) {
        animationTimer->start(animation->nextFrameDelay());
    }
}

void ImageViewer::displayAnimation(QMovie *_animation) {
    if(_animation && _animation->isValid()) {
        reset();
        animation = _animation;
        animation->jumpToFrame(0);
        readjust(animation->currentPixmap().size(),
                 animation->currentPixmap().rect());
        image = new QPixmap();
        *image = animation->currentPixmap().transformed(transform, Qt::SmoothTransformation);
        if(settings->transparencyGrid())
            drawTransparencyGrid();
        //update();
        startAnimation();
        hideCursorTimed(false);
    }
}

// display & initialize
void ImageViewer::displayImage(QPixmap *_image) {
    reset();
    if(_image) {
        image = _image;
        readjust(image->size(), image->rect());
        if(settings->transparencyGrid())
            drawTransparencyGrid();
        update();
        requestScaling();
        hideCursorTimed(false);
    }
}

// reset state, remove image & stop animation
void ImageViewer::reset() {
    isDisplaying = false;
    stopPosAnimation();
    if(animation) {
        stopAnimation();
        delete animation;
        animation = NULL;
    }
    if(image) {
        delete image;
        image = NULL;
    }
}

// unsetImage, then update and show cursor
void ImageViewer::closeImage() {
    reset();
    update();
    showCursor();
}

// apply new image dimensions and fit mode
void ImageViewer::readjust(QSize _sourceSize, QRect _drawingRect) {
    isDisplaying = true;
    mSourceSize  = _sourceSize;
    drawingRect =  _drawingRect;
    updateMinScale();
    updateMaxScale();
    setScale(1.0f);
    if(imageFitMode == FIT_FREE)
        imageFitMode = FIT_WINDOW;
    applyFitMode();
}

// takes scaled image
void ImageViewer::updateFrame(QPixmap *newFrame) {
    if(!animation && newFrame->size() != drawingRect.size()) {
        delete newFrame;
        return;
    }
    delete image;
    image = newFrame;
    if(checkboardGridEnabled)
        drawTransparencyGrid();
    update();
}

void ImageViewer::scrollUp() {
    scroll(0, -SCROLL_DISTANCE, true);
}

void ImageViewer::scrollDown() {
    scroll(0, SCROLL_DISTANCE, true);
}

void ImageViewer::scrollLeft() {
    scroll(-SCROLL_DISTANCE, 0, true);
}

void ImageViewer::scrollRight() {
    scroll(SCROLL_DISTANCE, 0, true);
}

void ImageViewer::readSettings() {
    smoothAnimatedImages = settings->smoothAnimatedImages();
    expandImage = settings->expandImage();
    maxResolutionLimit = (float)settings->maxZoomedResolution();
    maxScaleLimit = (float)settings->maximumZoom();
    updateMinScale();
    updateMaxScale();
    setFitMode(settings->imageFitMode());
    mouseWrapping = settings->mouseWrapping();
    checkboardGridEnabled = settings->transparencyGrid();
}

void ImageViewer::setExpandImage(bool mode) {
    expandImage = mode;
    updateMinScale();
    applyFitMode();
}

// scale at which current image fills the window
void ImageViewer::updateFitWindowScale() {
    float newMinScaleX = (float) width()*devicePixelRatioF() / mSourceSize.width();
    float newMinScaleY = (float) height()*devicePixelRatioF() / mSourceSize.height();
    if(newMinScaleX < newMinScaleY) {
        fitWindowScale = newMinScaleX;
    } else {
        fitWindowScale = newMinScaleY;
    }
}

bool ImageViewer::sourceImageFits() {
    return mSourceSize.width() < width()*devicePixelRatioF() && mSourceSize.height() < height()*devicePixelRatioF();
}

void ImageViewer::propertySetDrawPos(QPoint newPos) {
    if(newPos != drawingRect.topLeft()) {
        drawingRect.moveTopLeft(newPos);
        update();
    }
}

QPoint ImageViewer::propertyDrawPos() {
    return drawingRect.topLeft();
}

// limit min scale to window size
void ImageViewer::updateMinScale() {
    if(isDisplaying) {
        updateFitWindowScale();
        if(sourceImageFits()) {
            minScale = 1;
        } else {
            minScale = fitWindowScale;
        }
    }
}

// limit maxScale to MAX_SCALE_LIMIT
// then further limit to not exceed MAX_RESOLUTION
// so we dont go full retard on memory consumption
void ImageViewer::updateMaxScale() {
    maxScale = maxScaleLimit;
    float srcRes = (float)mSourceSize.width() *
                          mSourceSize.height() / 1000000;
    float maxRes = (float)maxScale * maxScale *
                          mSourceSize.width() *
                          mSourceSize.height() / 1000000;
    if(maxRes > maxResolutionLimit) {
        maxScale = (float)(sqrt(maxResolutionLimit) / sqrt(srcRes));
    }
}

// Scales drawingRect.
// drawingRect.topLeft() remains unchanged
void ImageViewer::setScale(float scale) {
    if(scale > maxScale) {
        mCurrentScale = maxScale;
    } else if(scale <= minScale + FLT_EPSILON) {
        mCurrentScale = minScale;
        if(imageFitMode == FIT_FREE && mCurrentScale == fitWindowScale) {
            imageFitMode = FIT_WINDOW;
        }
    } else {
        mCurrentScale = scale;
    }
    float w = scale * mSourceSize.width();
    float h = scale * mSourceSize.height();
    drawingRect.setWidth(w);
    drawingRect.setHeight(h);
    emit scaleChanged(mCurrentScale);
}

// ##################################################
// ###################  RESIZE  #####################
// ##################################################

void ImageViewer::requestScaling() {
    if(!isDisplaying)
        return;
    if(image->size() != drawingRect.size() && !animation) {
        emit scalingRequested(drawingRect.size());
    }
}

void ImageViewer::drawTransparencyGrid() {
    if(image && image->hasAlphaChannel()) {
        QPainter painter(image);
        painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
        QColor dark(90,90,90,255);
        QColor light(140,140,140,255);
        int xCount, yCount;
        xCount = image->width() / CHECKBOARD_GRID_SIZE;
        yCount = image->height() / CHECKBOARD_GRID_SIZE;
        QRect square(0, 0, CHECKBOARD_GRID_SIZE, CHECKBOARD_GRID_SIZE);
        bool evenOdd;
        for(int i = 0; i <= yCount; i++) {
            evenOdd = (i % 2);
            for(int j = 0; j <= xCount; j++) {
                if(j % 2 == evenOdd)
                    painter.fillRect(square, light);
                square.translate(CHECKBOARD_GRID_SIZE, 0);
            }
            square.translate(0, CHECKBOARD_GRID_SIZE);
            square.moveLeft(0);
        }
        painter.fillRect(image->rect(), dark);
    }
}

// ##################################################
// ####################  PAINT  #####################
// ##################################################
void ImageViewer::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter painter(this);
    if(animation && smoothAnimatedImages)
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    if(image) {
        image->setDevicePixelRatio(devicePixelRatioF());
        // maybe pre-calculate size so we wont do it in every draw?
        QRectF dpiAdjusted(drawingRect.topLeft()/devicePixelRatioF(), drawingRect.size()/devicePixelRatioF());
        //qDebug() << dpiAdjusted;
        painter.drawPixmap(dpiAdjusted, *image, image->rect());
    }
}

void ImageViewer::mousePressEvent(QMouseEvent *event) {
    QWidget::mousePressEvent(event);
    if(!isDisplaying)
        return;
    showCursor();
    setCursor(QCursor(Qt::ArrowCursor));
    mouseMoveStartPos = event->pos();
    if(event->button() == Qt::LeftButton) {
        setCursor(QCursor(Qt::ClosedHandCursor));
    }
    if(event->button() == Qt::RightButton) {
        setCursor(QCursor(Qt::SizeVerCursor));
        setZoomPoint(event->pos()*devicePixelRatioF());
    }
}

void ImageViewer::mouseMoveEvent(QMouseEvent *event) {
    QWidget::mouseMoveEvent(event);
    if(!isDisplaying)
        return;
    if(event->buttons() & Qt::LeftButton) {
        mouseWrapping?mouseDragWrapping(event):mouseDrag(event);
    } else if(event->buttons() & Qt::RightButton) {
        mouseDragZoom(event);
    } else {
        showCursor();
        hideCursorTimed(true);
    }
}

void ImageViewer::mouseReleaseEvent(QMouseEvent *event) {
    QWidget::mouseReleaseEvent(event);
    if(!isDisplaying) {
        return;
    }
    hideCursorTimed(false);
    setCursor(QCursor(Qt::ArrowCursor));
    if(event->button() == Qt::RightButton && imageFitMode != FIT_WINDOW) {
        //requestScaling();
        //fitDefault();
        //update();
    }
}

// Okular-like cursor drag behavior
// TODO: fix multiscreen
void ImageViewer::mouseDragWrapping(QMouseEvent *event) {
    if( drawingRect.size().width() > width()*devicePixelRatioF() ||
        drawingRect.size().height() > height()*devicePixelRatioF() )
    {
        bool wrapped = false;
        QPoint newPos = mapToGlobal(event->pos()); //global
        QPoint delta = mouseMoveStartPos - event->pos(); // relative
        if(delta.x() && abs(delta.x()) < desktopSize.width() / 2) {
            int left = drawingRect.x() - delta.x();
            int right = left + drawingRect.width();
            if(left <= 0 && right > width()*devicePixelRatioF()) {
                // wrap mouse along the X axis
                if(left+1 <= 0 && right-1 > width()*devicePixelRatioF()) {
                    if(newPos.x() >= desktopSize.width() - 1) {
                        newPos.setX(2);
                        cursor().setPos(newPos);
                        wrapped = true;
                    } else if(newPos.x() <= 0) {
                        newPos.setX(desktopSize.width() - 2);
                        cursor().setPos(newPos);
                        wrapped = true;
                    }
                }
                // move image
                drawingRect.moveLeft(left);
            }
        }
        if(delta.y() && abs(delta.y()) < desktopSize.height() / 2) {
            int top = drawingRect.y() - delta.y();
            int bottom = top + drawingRect.height();
            if(top <= 0 && bottom > height()*devicePixelRatioF()) {
                // wrap mouse along the Y axis
                if(top+1 <= 0 && bottom-1 > height()*devicePixelRatioF()) {
                    if(newPos.y() >= desktopSize.height() - 1) {
                        newPos.setY(2);
                        cursor().setPos(newPos);
                        wrapped = true;
                    } else if(newPos.y() <= 0) {
                        newPos.setY(desktopSize.height() - 2);
                        cursor().setPos(newPos);
                        wrapped = true;
                    }
                }
                // move image
                drawingRect.moveTop(top);
            }
        }
        if(wrapped)
            mouseMoveStartPos = mapFromGlobal(newPos);
        else
            mouseMoveStartPos = event->pos();
        update();
    }
}

// default drag behavior
void ImageViewer::mouseDrag(QMouseEvent *event) {
    if(drawingRect.size().width() > width()*devicePixelRatioF() ||
            drawingRect.size().height() > height()*devicePixelRatioF()) {
        mouseMoveStartPos -= event->pos();
        scroll(mouseMoveStartPos.x()*devicePixelRatioF(), mouseMoveStartPos.y()*devicePixelRatioF(), false);
        mouseMoveStartPos = event->pos();
    }
}

void ImageViewer::mouseDragZoom(QMouseEvent *event) {
    float step = 0.003;
    int currentPos = event->pos().y();
    int moveDistance = mouseMoveStartPos.y() - currentPos;
    float newScale = mCurrentScale * (1.0f + step * moveDistance);
    mouseMoveStartPos = event->pos();
    if(moveDistance < 0 && mCurrentScale <= minScale) {
        return;
    } else if(moveDistance > 0 && newScale > maxScale) { // already at max zoom
        newScale = maxScale;
    } else if(moveDistance < 0 && newScale < minScale - FLT_EPSILON) { // at min zoom
        if(sourceImageFits() && expandImage) {
            fitNormal();
        } else {
            newScale = minScale;
            setFitWindow();
        }
    } else {
        imageFitMode = FIT_FREE;
        scaleAroundZoomPoint(newScale);
        requestScaling();
    }
    update();
}

void ImageViewer::fitWidth() {
    if(isDisplaying) {
        float scale = (float) width()*devicePixelRatioF() / mSourceSize.width();
        if(!expandImage && scale > 1.0) {
            fitNormal();
        } else {
            setScale(scale);
            centerImage();
            if(drawingRect.height() > height()*devicePixelRatioF())
                drawingRect.moveTop(0);
            update();
        }
    } else {
        centerImage();
    }
}

void ImageViewer::fitWindow() {
    if(isDisplaying) {
        bool h = mSourceSize.height() <= height()*devicePixelRatioF();
        bool w = mSourceSize.width() <= width()*devicePixelRatioF();
        // source image fits entirely
        if(h && w && !expandImage) {
            fitNormal();
            return;
        } else { // doesnt fit
            setScale(expandImage?fitWindowScale:minScale);
            centerImage();
            update();
        }
    } else {
        centerImage();
    }
}

void ImageViewer::fitNormal() {
    if(!isDisplaying) {
        return;
    }
    setScale(1.0);
    centerImage();
    if(drawingRect.height() > height()*devicePixelRatioF()) {
        drawingRect.moveTop(0);
    }
    update();
}

void ImageViewer::setFitMode(ImageFitMode newMode) {
    imageFitMode = newMode;
    applyFitMode();
    requestScaling();
}

void ImageViewer::applyFitMode() {
    switch(imageFitMode) {
        case FIT_ORIGINAL:
            fitNormal();
            break;
        case FIT_WIDTH:
            fitWidth();
            break;
        case FIT_WINDOW:
            fitWindow();
            break;
        default: /* FREE etc */
            break;
    }
}

void ImageViewer::setFitOriginal() {
    setFitMode(FIT_ORIGINAL);
}

void ImageViewer::setFitWidth() {
    setFitMode(FIT_WIDTH);
}

void ImageViewer::setFitWindow() {
    setFitMode(FIT_WINDOW);
}

void ImageViewer::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event)
    stopPosAnimation();
    updateMinScale();
    if(imageFitMode == FIT_FREE || imageFitMode == FIT_ORIGINAL) {
        centerImage();
    } else {
        applyFitMode();
    }
    update();
    requestScaling();
}

// center image if it is smaller than parent
// align image's corner to window corner if needed
void ImageViewer::centerImage() {
    if(drawingRect.height() <= height()*devicePixelRatioF()) {
        drawingRect.moveTop((height()*devicePixelRatioF() - drawingRect.height()) / 2);
    } else {
        snapEdgeVertical();
    }
    if(drawingRect.width() <= width()*devicePixelRatioF()) {
        drawingRect.moveLeft((width()*devicePixelRatioF() - drawingRect.width()) / 2);
    } else {
        snapEdgeHorizontal();
    }
}

void ImageViewer::snapEdgeHorizontal() {
    if(drawingRect.x() > 0 && drawingRect.right() > width()*devicePixelRatioF()) {
        drawingRect.moveLeft(0);
    }
    if(width()*devicePixelRatioF() - drawingRect.x() > drawingRect.width()) {
        drawingRect.moveRight(width()*devicePixelRatioF());
    }
}

void ImageViewer::snapEdgeVertical() {
    if(drawingRect.y() > 0 && drawingRect.bottom() > height()*devicePixelRatioF()) {
        drawingRect.moveTop(0);
    }
    if(height()*devicePixelRatioF() - drawingRect.y() > drawingRect.height()) {
        drawingRect.moveBottom(height()*devicePixelRatioF());
    }
}

void ImageViewer::stopPosAnimation() {
    if(posAnimation->state() == QAbstractAnimation::Running)
        posAnimation->stop();
}

// scroll viewport and do update()
void ImageViewer::scroll(int dx, int dy, bool smooth) {
    stopPosAnimation();
    QPoint destTopLeft = drawingRect.topLeft();
    if(drawingRect.size().width() > width()*devicePixelRatioF()) {
        destTopLeft.setX(scrolledX(dx));
    }
    if(drawingRect.size().height() > height()*devicePixelRatioF()) {
        destTopLeft.setY(scrolledY(dy));
    }
    if(smooth) {
        posAnimation->setStartValue(drawingRect.topLeft());
        posAnimation->setEndValue(destTopLeft);
        posAnimation->start(QAbstractAnimation::KeepWhenStopped);
    } else {
        drawingRect.moveTopLeft(destTopLeft);
        update();
    }
}

inline
int ImageViewer::scrolledX(int dx) {
    int newXPos = drawingRect.left();
    if(dx) {
        int left = drawingRect.x() - dx;
        int right = left + drawingRect.width();
        if(left > 0)
            left = 0;
        else if (right <= width()*devicePixelRatioF())
            left = width()*devicePixelRatioF() - drawingRect.width();
        if(left <= 0) {
            newXPos = left;
        }
    }
    return newXPos;
}

inline
int ImageViewer::scrolledY(int dy) {
    int newYPos = drawingRect.top();
    if(dy) {
        int top = drawingRect.y() - dy;
        int bottom = top + drawingRect.height();
        if(top > 0)
            top = 0;
        else if (bottom <= height()*devicePixelRatioF())
            top = height()*devicePixelRatioF() - drawingRect.height();
        if(top <= 0) {
            newYPos = top;
        }
    }
    return newYPos;
}

void ImageViewer::hideCursorTimed(bool restartTimer) {
    if(restartTimer || !cursorTimer->isActive())
        cursorTimer->start(CURSOR_HIDE_TIMEOUT_MS);
}

void ImageViewer::setZoomPoint(QPoint pos) {
    zoomPoint = pos;
    zoomDrawRectPoint.setX((float) (zoomPoint.x() - drawingRect.x())
                                / drawingRect.width());
    zoomDrawRectPoint.setY((float) (zoomPoint.y() - drawingRect.y())
                                / drawingRect.height());
}

// scale image around zoom point,
// so that point's position relative to window remains unchanged
void ImageViewer::scaleAroundZoomPoint(float newScale) {
    setScale(newScale);
    float mappedX = drawingRect.width() * zoomDrawRectPoint.x() + drawingRect.left();
    float mappedY = drawingRect.height() * zoomDrawRectPoint.y() + drawingRect.top();
    int diffX = mappedX - zoomPoint.x();
    int diffY = mappedY - zoomPoint.y();
    drawingRect.moveLeft(drawingRect.left() - diffX);
    drawingRect.moveTop(drawingRect.top() - diffY);
    centerImage();
}

// zoom in around viewport center
void ImageViewer::zoomIn() {
    setZoomPoint(rect().center()*devicePixelRatioF());
    doZoomIn();
}

// zoom out around viewport center
void ImageViewer::zoomOut() {
    setZoomPoint(rect().center()*devicePixelRatioF());
    doZoomOut();
}

// zoom in around cursor
void ImageViewer::zoomInCursor() {
    if(underMouse()) {
        setZoomPoint(mapFromGlobal(cursor().pos())*devicePixelRatioF());
        doZoomIn();
    } else {
        zoomIn();
    }
}

// zoom out around cursor
void ImageViewer::zoomOutCursor() {
    if(underMouse()) {
        setZoomPoint(mapFromGlobal(cursor().pos())*devicePixelRatioF());
        doZoomOut();
    } else {
        zoomOut();
    }
}

void ImageViewer::doZoomIn() {
    if(!isDisplaying) {
        return;
    }
    float newScale = mCurrentScale * 1.1f;
    if(newScale == mCurrentScale) //skip if minScale
        return;
    if(newScale > maxScale)
        newScale = maxScale;
    imageFitMode = FIT_FREE;
    scaleAroundZoomPoint(newScale);
    update();
    requestScaling();
}

void ImageViewer::doZoomOut() {
    if(!isDisplaying) {
        return;
    }
    float newScale = mCurrentScale * 0.9f;
    if(newScale == mCurrentScale) //skip if maxScale
        return;
    if(newScale < minScale - FLT_EPSILON)
        newScale = minScale;
    imageFitMode = FIT_FREE;
    scaleAroundZoomPoint(newScale);
    update();
    requestScaling();
}

ImageFitMode ImageViewer::fitMode() {
    return imageFitMode;
}

QRect ImageViewer::imageRect() {
    return drawingRect;
}

float ImageViewer::currentScale() {
    return mCurrentScale;
}

QSize ImageViewer::sourceSize() {
    return mSourceSize;
}

void ImageViewer::hideCursor() {
    cursorTimer->stop();
    if(this->underMouse()) {
        setCursor(QCursor(Qt::BlankCursor));
    }
}

void ImageViewer::showCursor() {
    cursorTimer->stop();
    setCursor(QCursor(Qt::ArrowCursor));
}
