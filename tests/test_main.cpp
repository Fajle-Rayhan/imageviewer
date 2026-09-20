#include <QCoreApplication>
#include <QtTest>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    int status = 0;

    extern QObject* createSettingsTest();
    extern QObject* createModelTest();
    extern QObject* createEditorTest();
    extern QObject* createOpenWithTest();
    extern QObject* createViewModeTest();

    QObject* tests[] = {
        createSettingsTest(),
        createModelTest(),
        createEditorTest(),
        createOpenWithTest(),
        createViewModeTest(),
    };

    for (QObject* test : tests) {
        status |= QTest::qExec(test, argc, argv);
        delete test;
    }
    return status;
}
