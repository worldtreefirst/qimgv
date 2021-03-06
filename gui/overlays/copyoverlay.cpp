#include "copyoverlay.h"
#include "ui_copyoverlay.h"

CopyOverlay::CopyOverlay(ContainerWidget *parent) :
    FloatingWidget(parent),
    ui(new Ui::CopyOverlay)
{
    ui->setupUi(this);
    hide();
    setPosition(FloatingWidgetPosition::BOTTOMLEFT);
    ui->headerLabel->setPixmap(QPixmap(":/res/images/copyheader.png"));
    mode = OVERLAY_COPY;
    createShortcuts();
    readSettings();
}

CopyOverlay::~CopyOverlay() {
    delete ui;
}

void CopyOverlay::show() {
    QWidget::show();
    setFocus();
}

void CopyOverlay::hide() {
    QWidget::hide();
    clearFocus();
}


void CopyOverlay::setDialogMode(CopyOverlayMode _mode) {
    mode = _mode;
    if(mode == OVERLAY_COPY)
        ui->headerLabel->setPixmap(QPixmap(":/res/images/copyheader.png"));
    else
        ui->headerLabel->setPixmap(QPixmap(":/res/images/moveheader.png"));
}

CopyOverlayMode CopyOverlay::operationMode() {
    return mode;
}

void CopyOverlay::removePathWidgets() {
    for(int i = 0; i < pathWidgets.count(); i++) {
        QWidget *tmp = pathWidgets.at(i);
        ui->pathSelectorsLayout->removeWidget(tmp);
        delete tmp;
    }
    pathWidgets.clear();
}

void CopyOverlay::createPathWidgets() {
    removePathWidgets();
    int count = paths.length()>maxPathCount?maxPathCount:paths.length();
    for(int i = 0; i < count; i++) {
        PathSelectorWidget *pathWidget = new PathSelectorWidget(this);
        pathWidget->setPath(paths.at(i));
        pathWidget->setButtonText(shortcuts.key(i));
        connect(pathWidget, SIGNAL(selected(QString)),
                this, SLOT(requestFileOperation(QString)));
        pathWidgets.append(pathWidget);
        ui->pathSelectorsLayout->addWidget(pathWidget);
    }
}

void CopyOverlay::createShortcuts() {
    for(int i=0; i<maxPathCount; i++)
        shortcuts.insert(QString::number(i + 1), i);
}

void CopyOverlay::requestFileOperation(QString path) {
    if(mode == OVERLAY_COPY)
        emit copyRequested(path);
    else
        emit moveRequested(path);
}

void CopyOverlay::requestFileOperation(int fieldNumber) {
    if(mode == OVERLAY_COPY)
        emit copyRequested(pathWidgets.at(fieldNumber)->path());
    else
        emit moveRequested(pathWidgets.at(fieldNumber)->path());
}

void CopyOverlay::readSettings() {
    paths = settings->savedPaths();
    if(paths.count() < maxPathCount)
        createDefaultPaths();
    createPathWidgets();
    update();
}

void CopyOverlay::saveSettings() {
    paths.clear();
    for(int i = 0; i< pathWidgets.count(); i++) {
        paths << pathWidgets.at(i)->path();
    }
    settings->setSavedPaths(paths);
}

void CopyOverlay::createDefaultPaths() {
    while(paths.count() < maxPathCount) {
        paths << QDir::homePath();
    }
}

void CopyOverlay::keyPressEvent(QKeyEvent *event) {
    QString key = actionManager->keyForNativeScancode(event->nativeScanCode());
    if(shortcuts.contains(key)) {
        event->accept();
        requestFileOperation(shortcuts[key]);
    } else {
        event->ignore();
    }
}
