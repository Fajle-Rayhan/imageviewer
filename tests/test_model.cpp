#include "../src/imagedirectorymodel.h"

#include <QDir>
#include <QFile>
#include <QtTest>

using namespace imageviewer;

class ModelTest : public QObject {
    Q_OBJECT

private slots:
    void navigationAndDeleteSelection()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString a = dir.path() + QStringLiteral("/a.png");
        const QString b = dir.path() + QStringLiteral("/b.jpg");
        const QString c = dir.path() + QStringLiteral("/c.webp");
        QFile(a).open(QIODevice::WriteOnly);
        QFile(b).open(QIODevice::WriteOnly);
        QFile(c).open(QIODevice::WriteOnly);

        ImageDirectoryModel model;
        QVERIFY(model.loadFor(b));
        QCOMPARE(QFileInfo(model.current()).fileName(), QStringLiteral("b.jpg"));
        QCOMPARE(QFileInfo(model.next()).fileName(), QStringLiteral("c.webp"));
        QCOMPARE(QFileInfo(model.previous()).fileName(), QStringLiteral("b.jpg"));

        QCOMPARE(QFileInfo(model.removeCurrentOrPath(b, QStringLiteral("next"))).fileName(), QStringLiteral("c.webp"));
        QCOMPARE(QFileInfo(model.removeCurrentOrPath(c, QStringLiteral("prev"))).fileName(), QStringLiteral("a.png"));
    }
};

QObject* createModelTest() { return new ModelTest; }

#include "test_model.moc"
