#include "trashservice.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

namespace imageviewer {

bool TrashService::moveToTrash(const QString& path, QString* error) const
{
    if (!QFileInfo::exists(path)) {
        if (error) {
            *error = QStringLiteral("File does not exist");
        }
        return false;
    }

    int exitCode = -1;
    if (QProcess::execute(QStringLiteral("gio"), {QStringLiteral("trash"), path}) == 0) {
        return true;
    }

    const QString home = QDir::homePath();
    const QString trashBase = home + QStringLiteral("/.local/share/Trash");
    const QString filesDir = trashBase + QStringLiteral("/files");
    const QString infoDir = trashBase + QStringLiteral("/info");
    QDir().mkpath(filesDir);
    QDir().mkpath(infoDir);

    QFileInfo info(path);
    QString target = filesDir + QLatin1Char('/') + info.fileName();
    int suffix = 1;
    while (QFileInfo::exists(target)) {
        target = filesDir + QLatin1Char('/') + info.completeBaseName() + QStringLiteral("_%1").arg(suffix++);
        if (!info.suffix().isEmpty()) {
            target += QLatin1Char('.') + info.suffix();
        }
    }

    if (!QFile::rename(path, target)) {
        if (error) {
            *error = QStringLiteral("Failed to move to trash");
        }
        return false;
    }

    QFile infoFile(infoDir + QLatin1Char('/') + QFileInfo(target).fileName() + QStringLiteral(".trashinfo"));
    if (infoFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        const QByteArray content = QByteArrayLiteral("[Trash Info]\nPath=")
            + QFile::encodeName(info.absoluteFilePath())
            + QByteArrayLiteral("\nDeletionDate=")
            + QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toUtf8()
            + QByteArrayLiteral("\n");
        infoFile.write(content);
    }

    Q_UNUSED(exitCode);
    return true;
}

} // namespace imageviewer
