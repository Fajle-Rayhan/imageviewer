#include "../src/editorcommands.h"

#include <QtTest>

using namespace imageviewer;

class EditorTest : public QObject {
    Q_OBJECT

private slots:
    void undoRedo()
    {
        EditorCommandStack stack;

        EditorCommand c1;
        c1.type = EditorCommandType::Stroke;
        c1.stroke.points = {QPointF(0, 0), QPointF(1, 1)};
        c1.stroke.color = QColor("#ffffff");
        c1.stroke.size = 2;
        c1.stroke.opacity = 1.0;

        EditorCommand c2 = c1;
        c2.stroke.color = QColor("#000000");
        c2.stroke.size = 4;

        stack.add(c1);
        stack.add(c2);
        QCOMPARE(stack.commands().size(), 2);

        EditorCommand removed = stack.undo();
        QCOMPARE(removed.stroke.size, 4);
        QCOMPARE(stack.commands().size(), 1);

        EditorCommand restored = stack.redo();
        QCOMPARE(restored.stroke.size, 4);
        QCOMPARE(stack.commands().size(), 2);
    }
};

QObject* createEditorTest() { return new EditorTest; }

#include "test_editor.moc"
