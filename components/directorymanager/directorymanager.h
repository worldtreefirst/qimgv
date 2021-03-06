#ifndef DIRECTORYMANAGER_H
#define DIRECTORYMANAGER_H

#include <QObject>
#include <QMimeDatabase>
#include <QDir>
#include <QCollator>
#include <algorithm>
#include <vector>
#include <QElapsedTimer>
#include <QUrl>
#include <QFile>
#include <QString>
#include <QSize>
#include <QDebug>
#include <QFileInfo>
#include <QMessageBox>
#include <QImageReader>
#include "settings.h"
#include "sourcecontainers/imageinfo.h"

class DirectoryManager : public QObject
{
    Q_OBJECT
public:
    DirectoryManager();
    // ignored if the same dir is already opened
    void setDirectory(QString);
    // returns index in file list
    // -1 if not found
    int indexOf(QString fileName) const;
    QStringList fileList() const;
    QString currentDirectoryPath() const;
    QString filePathAt(int index) const;
    bool removeAt(int index);
    int fileCount() const;
    bool existsInCurrentDir(QString fileName) const;
    bool isImage(QString filePath) const;
    bool hasImages() const;
    bool contains(QString fileName) const;
    bool checkRange(int index) const;
    bool copyTo(QString destDirectory, int index);
    QString fileNameAt(int index) const;
    bool isDirectory(QString path) const;

private slots:
    void fileChanged(const QString file);
    void directoryContentsChanged(QString dirPath);

private:
    QDir currentDir;
    QStringList mFileNameList;
    QStringList mimeFilters, extensionFilters;

    void readSettings();
//    WatcherWindows watcher;
    QMimeDatabase mimeDb;
    bool quickFormatDetection;

    void generateFileList();
    void generateFileListQuick();
    void generateFileListDeep();

    void onFileRemovedExternal(QString);

    void sortFileList();
    void onFileChangedExternal(QString fileName);
signals:
    void directoryChanged(const QString &path);
    void directorySortingChanged();
    void fileRemovedAt(int);
    void fileChangedAt(int);
    void fileAddedAt(int);
};

#endif // DIRECTORYMANAGER_H
