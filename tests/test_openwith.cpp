#include "../src/openwithservice.h"

#include <QtTest>

using namespace imageviewer;

class OpenWithTest : public QObject {
    Q_OBJECT

private slots:
    void buildsSafeCommand()
    {
        const QStringList cmd = OpenWithService::buildCommand(QStringLiteral("imv %f"), QStringLiteral("/tmp/a.png"));
        QCOMPARE(cmd.size(), 2);
        QCOMPARE(cmd.first(), QStringLiteral("imv"));
        QCOMPARE(cmd.last(), QStringLiteral("/tmp/a.png"));

        const QStringList cmd2 = OpenWithService::buildCommand(QStringLiteral("gimp --new-instance"), QStringLiteral("/tmp/a.png"));
        QCOMPARE(cmd2.first(), QStringLiteral("gimp"));
        QCOMPARE(cmd2.last(), QStringLiteral("/tmp/a.png"));
    }
};

QObject* createOpenWithTest() { return new OpenWithTest; }

#include "test_openwith.moc"
