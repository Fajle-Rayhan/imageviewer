#pragma once

#include <QDir>
#include <QStringList>

namespace imageviewer {

class ImageDirectoryModel {
public:
    bool loadFor(const QString& imagePath);
    QString current() const;
    QString next();
    QString previous();
    QString removeCurrentOrPath(const QString& path, const QString& prefer);
    QStringList files() const { return m_files; }
    int currentIndex() const { return m_index; }

    static QStringList supportedExtensions();

private:
    QStringList m_files;
    int m_index = -1;
};

} // namespace imageviewer
