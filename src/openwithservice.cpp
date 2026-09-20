#include "openwithservice.h"

#include <QDir>
#include <QFile>
#include <QMimeDatabase>
#include <QProcess>
#include <QSet>
#include <QStandardPaths>
#include <QStringTokenizer>

namespace imageviewer {

bool OpenWithService::desktopFileSupportsMime(const QString& desktopFilePath, const QString& mimeType, OpenWithApp* out)
{
    QFile file(desktopFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QString name;
    QString exec;
    QString icon;
    QStringList mimeTypes;
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.startsWith(QLatin1String("Name="))) {
            name = line.mid(5);
        } else if (line.startsWith(QLatin1String("Exec="))) {
            exec = line.mid(5);
        } else if (line.startsWith(QLatin1String("Icon="))) {
            icon = line.mid(5);
        } else if (line.startsWith(QLatin1String("MimeType="))) {
            mimeTypes = line.mid(9).split(QLatin1Char(';'), Qt::SkipEmptyParts);
        }
    }

    if (name.isEmpty() || exec.isEmpty() || !mimeTypes.contains(mimeType)) {
        return false;
    }

    if (out) {
        out->name = name;
        out->exec = exec;
        out->icon = icon;
    }
    return true;
}

QVector<OpenWithApp> OpenWithService::listApplications(const QString& filePath) const
{
    QVector<OpenWithApp> apps;
    QMimeDatabase db;
    const QString mime = db.mimeTypeForFile(filePath, QMimeDatabase::MatchContent).name();
    if (mime.isEmpty()) {
        return apps;
    }

    QSet<QString> seenNames;
    const QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    for (const QString& dirPath : dirs) {
        QDir dir(dirPath);
        const QStringList desktopFiles = dir.entryList({QStringLiteral("*.desktop")}, QDir::Files);
        for (const QString& desktopFile : desktopFiles) {
            OpenWithApp app;
            const QString path = dir.absoluteFilePath(desktopFile);
            if (!desktopFileSupportsMime(path, mime, &app)) {
                continue;
            }
            if (seenNames.contains(app.name)) {
                continue;
            }
            seenNames.insert(app.name);
            apps.push_back(app);
        }
    }
    std::sort(apps.begin(), apps.end(), [](const OpenWithApp& a, const OpenWithApp& b) {
        return a.name.toLower() < b.name.toLower();
    });
    return apps;
}

QStringList OpenWithService::buildCommand(const QString& execTemplate, const QString& filePath)
{
    QString cleaned = execTemplate;
    cleaned.remove(QStringLiteral("%u"));
    cleaned.remove(QStringLiteral("%U"));
    cleaned.remove(QStringLiteral("%f"));
    cleaned.remove(QStringLiteral("%F"));
    cleaned.remove(QStringLiteral("%i"));
    cleaned.remove(QStringLiteral("%c"));
    cleaned.remove(QStringLiteral("%k"));
    cleaned = cleaned.simplified();

    QStringList split = QProcess::splitCommand(cleaned);
    if (split.isEmpty()) {
        return {};
    }
    split.push_back(filePath);
    return split;
}

bool OpenWithService::launch(const OpenWithApp& app, const QString& filePath, QString* error) const
{
    const QStringList cmd = buildCommand(app.exec, filePath);
    if (cmd.isEmpty()) {
        if (error) {
            *error = QStringLiteral("Invalid desktop command for selected application.");
        }
        return false;
    }

    const QString program = cmd.first();
    const QStringList args = cmd.mid(1);
    const bool ok = QProcess::startDetached(program, args);
    if (!ok && error) {
        *error = QStringLiteral("Failed to launch %1").arg(app.name);
    }
    return ok;
}

} // namespace imageviewer
