#include "../src/appsettings.h"

#include <QDir>
#include <QtTest>

using namespace imageviewer;

class SettingsTest : public QObject {
    Q_OBJECT

private slots:
    void persistsValues()
    {
        const QString path = QDir::temp().filePath(QStringLiteral("imageviewer-settings-test.ini"));
        QFile::remove(path);

        AppSettings s(QString(), QString(), path);
        s.data().startupMode = StartupMode::Normal;
        s.data().shortcuts[QStringLiteral("toggle_menu")] = QKeySequence(QStringLiteral("Ctrl+M"));
        s.data().panel.thumbnailSize = 140;
        s.save();

        AppSettings loaded(QString(), QString(), path);
        loaded.load();

        QCOMPARE(loaded.data().startupMode, StartupMode::Normal);
        QCOMPARE(loaded.data().shortcuts.value(QStringLiteral("toggle_menu")).toString(), QStringLiteral("Ctrl+M"));
        QCOMPARE(loaded.data().panel.thumbnailSize, 140);
        QFile::remove(path);
    }
};

QObject* createSettingsTest() { return new SettingsTest; }

#include "test_settings.moc"
