#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog) {
    ui->setupUi(this);
    this->setWindowTitle("Preferences - qimgv " +
                         QCoreApplication::applicationVersion());
    ui->bgColorLabel->setAutoFillBackground(true);
    ui->accentColorLabel->setAutoFillBackground(true);
    shortcutKeys = actionManager->keys();
    ui->versionLabel->setText(QApplication::applicationVersion());
    connect(this, SIGNAL(settingsChanged()),
            settings, SLOT(sendChangeNotification()));
    readSettings();
}

void SettingsDialog::readSettings() {
    bool setting;
    ui->infiniteScrollingCheckBox->setChecked(settings->infiniteScrolling());
    ui->playWebmCheckBox->setChecked(settings->playWebm());
    ui->playMp4CheckBox->setChecked(settings->playMp4());
    ui->playSoundsCheckBox->setChecked(settings->playVideoSounds());
    ui->enablePanelCheckBox->setChecked(settings->panelEnabled());
    ui->panelFullscreenOnlyCheckBox->setChecked(settings->panelFullscreenOnly());
    ui->mouseWrappingCheckBox->setChecked(settings->mouseWrapping());
    ui->squareThumbnailsCheckBox->setChecked(settings->squareThumbnails());
    ui->transparencyGridCheckBox->setChecked(settings->transparencyGrid());
    ui->forceSmoothScrollCheckBox->setChecked(settings->forceSmoothScroll());
    ui->usePreloaderCheckBox->setChecked(settings->usePreloader());
    ui->useThumbnailCacheCheckBox->setChecked(settings->useThumbnailCache());
    ui->smoothUpscalingCheckBox->setChecked(settings->smoothUpscaling());
    ui->expandImageCheckBox->setChecked(settings->expandImage());
    ui->smoothAnimatedImagesCheckBox->setChecked(settings->smoothAnimatedImages());

    ui->mpvLineEdit->setText(settings->mpvBinary());

    // ##### scaling #####
    setting = settings->useFastScale();
    ui->scalingQualityComboBox->setCurrentIndex(setting ? 1 : 0);
    ui->maxZoomSlider->setValue(settings->maximumZoom());
    onMaxZoomSliderChanged(settings->maximumZoom());
    ui->maxZoomResSlider->setValue(settings->maxZoomedResolution());
    onMaxZoomResolutionSliderChanged(settings->maxZoomedResolution());

    // ##### fit mode #####
    int fitMode = settings->imageFitMode();
    ui->fitModeComboBox->setCurrentIndex(fitMode);

    // ##### UI #####
    ui->scalingQualityComboBox->setCurrentIndex(settings->scalingFilter());

    ui->fullscreenCheckBox->setChecked(settings->fullscreenMode());

    ui->panelPositionComboBox->setCurrentIndex(settings->panelPosition());

    //bg color
    QColor bgColor = settings->backgroundColor();
    bgLabelPalette.setColor(QPalette::Window, bgColor);
    ui->bgColorLabel->setPalette(bgLabelPalette);

    //accent color
    QColor accentColor = settings->accentColor();
    accentLabelPalette.setColor(QPalette::Window, accentColor);
    ui->accentColorLabel->setPalette(accentLabelPalette);

    // thumbnail size
    // maybe use slider instead of combobox?
    int size = settings->mainPanelSize();
    switch(size) {
        case SettingsDialog::thumbSizeSmall:
            ui->thumbSizeComboBox->setCurrentIndex(0);
            break;
        case SettingsDialog::thumbSizeMedium:
            ui->thumbSizeComboBox->setCurrentIndex(1);
            break;
        case SettingsDialog::thumbSizeLarge:
            ui->thumbSizeComboBox->setCurrentIndex(2);
            break;
        case SettingsDialog::thumbSizeVeryLarge:
            ui->thumbSizeComboBox->setCurrentIndex(3);
            break;
        default:
            ui->thumbSizeComboBox->addItem("Custom: " + QString::number(size) + "px");
            ui->thumbSizeComboBox->setCurrentIndex(4);
            thumbSizeCustom = size;
            break;
    }
    fillShortcuts();
}

void SettingsDialog::applySettings() {
    // wait for all background stuff to finish
    if(QThreadPool::globalInstance()->activeThreadCount()) {
        QThreadPool::globalInstance()->waitForDone();
    }

    settings->setInfiniteScrolling(ui->infiniteScrollingCheckBox->isChecked());
    settings->setFullscreenMode(ui->fullscreenCheckBox->isChecked());
    settings->setImageFitMode((ImageFitMode)ui->fitModeComboBox->currentIndex());
    settings->setPlayWebm(ui->playWebmCheckBox->isChecked());
    settings->setPlayMp4(ui->playMp4CheckBox->isChecked());
    settings->setPlayVideoSounds(ui->playSoundsCheckBox->isChecked());
    settings->setPanelEnabled(ui->enablePanelCheckBox->isChecked());
    settings->setPanelFullscreenOnly(ui->panelFullscreenOnlyCheckBox->isChecked());
    settings->setMouseWrapping(ui->mouseWrappingCheckBox->isChecked());
    settings->setSquareThumbnails(ui->squareThumbnailsCheckBox->isChecked());
    settings->setTransparencyGrid(ui->transparencyGridCheckBox->isChecked());
    settings->setForceSmoothScroll(ui->forceSmoothScrollCheckBox->isChecked());
    settings->setUsePreloader(ui->usePreloaderCheckBox->isChecked());
    settings->setUseThumbnailCache(ui->useThumbnailCacheCheckBox->isChecked());
    settings->setSmoothUpscaling(ui->smoothUpscalingCheckBox->isChecked());
    settings->setExpandImage(ui->expandImageCheckBox->isChecked());
    settings->setSmoothAnimatedImages(ui->smoothAnimatedImagesCheckBox->isChecked());

    settings->setMpvBinary(ui->mpvLineEdit->text());

    settings->setScalingFilter(ui->scalingQualityComboBox->currentIndex());

    settings->setPanelPosition((PanelHPosition) ui->panelPositionComboBox->currentIndex());

    settings->setBackgroundColor(bgLabelPalette.color(QPalette::Window));
    settings->setAccentColor(accentLabelPalette.color(QPalette::Window));

    int index = ui->thumbSizeComboBox->currentIndex();
    if(index == 0) {
        settings->setMainPanelSize(thumbSizeSmall);
    } else if(index == 1) {
        settings->setMainPanelSize(thumbSizeMedium);
    } else if(index == 2) {
        settings->setMainPanelSize(thumbSizeLarge);
    } else if(index == 3) {
        settings->setMainPanelSize(thumbSizeVeryLarge);
    } else if(index == 4) {
        settings->setMainPanelSize(thumbSizeCustom);
    }

    settings->setMaximumZoom(ui->maxZoomSlider->value());
    settings->setMaxZoomedResolution(ui->maxZoomResSlider->value());

    applyShortcuts();
    settings->saveShortcuts();
    emit settingsChanged();
}

void SettingsDialog::applySettingsAndClose() {
    applySettings();
    this->close();
}

void SettingsDialog::fillShortcuts() {
    ui->shortcutsListWidget->clear();
    const QMap<QString, QString> shortcuts = actionManager->allShortcuts();
    QMapIterator<QString, QString> i(shortcuts);
    while(i.hasNext()) {
        i.next();
        ui->shortcutsListWidget->addItem(i.value() + "=" + i.key());
    }
}

void SettingsDialog::addShortcut() {
    const QStringList actionList = actionManager->actionList(); // move to constructor?
    SettingsShortcutWidget w(actionList, shortcutKeys, this);
    if(w.exec()) {
        ui->shortcutsListWidget->addItem(w.text());
    }
}

void SettingsDialog::removeShortcut() {
    int row = ui->shortcutsListWidget->currentRow();
    if(row >= 0) {
        delete ui->shortcutsListWidget->takeItem(row);
    }
}

void SettingsDialog::applyShortcuts() {
    actionManager->removeAll();
    for(int i = 0; i < ui->shortcutsListWidget->count(); i++) {
        QStringList s = ui->shortcutsListWidget->item(i)->text().split("=");
        actionManager->addShortcut(s[1], s[0]);
    }
}

void SettingsDialog::resetShortcuts() {
    actionManager->resetDefaults();
    fillShortcuts();
}

void SettingsDialog::selectMpvPath() {
    QFileDialog dialog;
    QString file;
    file = dialog.getOpenFileName(this, "Navigate to mpv binary", "", "mpv*");
    if(!file.isEmpty()) {
        ui->mpvLineEdit->setText(file);
    }
}

void SettingsDialog::bgColorDialog() {
    QColorDialog *colorDialog = new QColorDialog(this);
    QColor newColor;
    newColor = colorDialog->getColor(bgLabelPalette.color(QPalette::Window),
                                     this,
                                     "Background color.");
    if(newColor.isValid()) {
        bgLabelPalette.setColor(QPalette::Window, newColor);
        ui->bgColorLabel->setPalette(bgLabelPalette);
    }
    delete colorDialog;
}

void SettingsDialog::accentColorDialog() {
    QColorDialog *colorDialog = new QColorDialog(this);
    QColor newColor;
    newColor = colorDialog->getColor(accentLabelPalette.color(QPalette::Window),
                                     this,
                                     "Accent color.");
    if(newColor.isValid()) {
        accentLabelPalette.setColor(QPalette::Window, newColor);
        ui->accentColorLabel->setPalette(accentLabelPalette);
    }
    delete colorDialog;
}

void SettingsDialog::onMaxZoomSliderChanged(int value) {
    ui->maxZoomLabel->setText(QString::number(value) + "x");
}

void SettingsDialog::onMaxZoomResolutionSliderChanged(int value) {
    ui->maxZoomResLabel->setText(QString::number(value) + " Mpx");
    ui->maxZoomResInfoLabel->setText("<small><i>Max. memory usage: ~" + QString::number(value * 4) + "MB @ 32bpp</i></small>");
}

int SettingsDialog::exec() {
    this->show();
    setMinimumSize(sizeHint() + QSize(20, 0));
    resize(sizeHint() + QSize(20, 0));
    return QDialog::exec();
}

SettingsDialog::~SettingsDialog() {
    delete ui;
}
