#include "imagedirectorymodel.h"

#include <QFileInfo>

namespace imageviewer {

QStringList ImageDirectoryModel::supportedExtensions()
{
    return {
        QStringLiteral("*.png"), QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"), QStringLiteral("*.webp"),
        QStringLiteral("*.gif"), QStringLiteral("*.bmp"), QStringLiteral("*.svg"), QStringLiteral("*.tif"),
        QStringLiteral("*.tiff"), QStringLiteral("*.ico"), QStringLiteral("*.ppm")
    };
}

bool ImageDirectoryModel::loadFor(const QString& imagePath)
{
    QFileInfo info(imagePath);
    if (!info.exists()) {
        return false;
    }
    QDir dir(info.absolutePath());
    m_files = dir.entryList(supportedExtensions(), QDir::Files, QDir::Name | QDir::IgnoreCase);
    for (QString& file : m_files) {
        file = dir.absoluteFilePath(file);
    }

    m_index = m_files.indexOf(info.absoluteFilePath());
    if (m_index < 0) {
        m_files.prepend(info.absoluteFilePath());
        m_index = 0;
    }
    return true;
}

QString ImageDirectoryModel::current() const
{
    if (m_index < 0 || m_index >= m_files.size()) {
        return {};
    }
    return m_files.at(m_index);
}

QString ImageDirectoryModel::next()
{
    if (m_files.isEmpty()) {
        return {};
    }
    m_index = (m_index + 1) % m_files.size();
    return current();
}

QString ImageDirectoryModel::previous()
{
    if (m_files.isEmpty()) {
        return {};
    }
    m_index = (m_index - 1 + m_files.size()) % m_files.size();
    return current();
}

QString ImageDirectoryModel::removeCurrentOrPath(const QString& path, const QString& prefer)
{
    const int idx = m_files.indexOf(path);
    if (idx < 0) {
        return current();
    }
    m_files.removeAt(idx);
    if (m_files.isEmpty()) {
        m_index = -1;
        return {};
    }

    if (prefer == QStringLiteral("prev")) {
        m_index = qMax(0, idx - 1);
    } else {
        m_index = qMin(idx, m_files.size() - 1);
    }
    return current();
}

} // namespace imageviewer
