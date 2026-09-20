#include "../src/appsettings.h"

#include <QtTest>

using namespace imageviewer;

class ViewModeTest : public QObject {
    Q_OBJECT

private slots:
    void parsesViewModes()
    {
        QCOMPARE(AppSettings::viewModeFromString(QStringLiteral("normal")), ViewMode::Normal);
        QCOMPARE(AppSettings::viewModeFromString(QStringLiteral("true_size")), ViewMode::TrueSize);
        QCOMPARE(AppSettings::viewModeFromString(QStringLiteral("screen_wide")), ViewMode::ScreenWide);
        QCOMPARE(AppSettings::viewModeToString(ViewMode::ScreenWide), QStringLiteral("screen_wide"));
    }
};

QObject* createViewModeTest() { return new ViewModeTest; }

#include "test_viewmode.moc"
