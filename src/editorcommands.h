#pragma once

#include <QColor>
#include <QFont>
#include <QPointF>
#include <QString>
#include <QVector>

namespace imageviewer {

enum class EditorCommandType { Stroke, Text };

struct StrokeCommand {
    QVector<QPointF> points;
    QColor color;
    int size = 1;
    qreal opacity = 1.0;
};

struct TextCommand {
    QPointF position;
    QString text;
    QFont font;
    QColor color;
};

struct EditorCommand {
    EditorCommandType type = EditorCommandType::Stroke;
    StrokeCommand stroke;
    TextCommand text;
};

class EditorCommandStack {
public:
    void add(const EditorCommand& cmd);
    bool canUndo() const;
    bool canRedo() const;
    EditorCommand undo();
    EditorCommand redo();
    const QVector<EditorCommand>& commands() const { return m_commands; }
    void clear();

private:
    QVector<EditorCommand> m_commands;
    QVector<EditorCommand> m_redo;
};

} // namespace imageviewer
