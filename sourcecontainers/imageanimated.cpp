#include "imageanimated.h"
#include <time.h>

// TODO: this class is kinda useless now. redesign?

ImageAnimated::ImageAnimated(QString _path) {
    path = _path;
    loaded = false;
    mSize.setWidth(0);
    mSize.setHeight(0);
    imageInfo = new ImageInfo(path);
    load();
}

ImageAnimated::~ImageAnimated() {
}

void ImageAnimated::load() {
    if(isLoaded()) {
        return;
    }
    if(!imageInfo) {
        imageInfo = new ImageInfo(path);
    }
    QPixmap pixmap(path, imageInfo->extension());
    mSize = pixmap.size();
    loaded = true;
}

bool ImageAnimated::save(QString destPath) {
    QFile file(path);
    if(file.exists()) {
        if(!file.copy(destPath)) {
            qDebug() << "Unable to save file.";
            return false;
        } else {
            return true;
        }
    } else {
        qDebug() << "Unable to save file. Perhaps the source file was deleted?";
        return false;
    }
}

bool ImageAnimated::save() {
    //TODO
    return false;
}

// in case of gif returns current frame
QPixmap *ImageAnimated::getPixmap() {
    QPixmap *pix = new QPixmap(path, imageInfo->extension());
    return pix;
}

const QImage *ImageAnimated::getImage() {
    QImage *img = new QImage();
    const QImage *cPtr = img;
    return cPtr;
}

QMovie *ImageAnimated::getMovie() {
    QMovie *_movie = new QMovie();
    _movie->setFileName(path);
    _movie->setFormat(imageInfo->extension());
    _movie->jumpToFrame(0);
    return _movie;
}

int ImageAnimated::height() {
    return mSize.height();
}

int ImageAnimated::width() {
    return mSize.width();
}

QSize ImageAnimated::size() {
    return mSize;
}
