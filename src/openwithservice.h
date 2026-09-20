#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

namespace imageviewer {

struct OpenWithApp {
    QString name;
    QString exec;
    QString icon;
};

class OpenWithService {
public:
    QVector<OpenWithApp> listApplications(const QString& filePath) const;
    bool launch(const OpenWithApp& app, const QString& filePath, QString* error = nullptr) const;

    static QStringList buildCommand(const QString& execTemplate, const QString& filePath);

private:
    static bool desktopFileSupportsMime(const QString& desktopFilePath, const QString& mimeType, OpenWithApp* out);
};

} // namespace imageviewer
