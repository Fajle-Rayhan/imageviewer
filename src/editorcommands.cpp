#include "editorcommands.h"

#include <stdexcept>

namespace imageviewer {

void EditorCommandStack::add(const EditorCommand& cmd)
{
    m_commands.push_back(cmd);
    m_redo.clear();
}

bool EditorCommandStack::canUndo() const
{
    return !m_commands.isEmpty();
}

bool EditorCommandStack::canRedo() const
{
    return !m_redo.isEmpty();
}

EditorCommand EditorCommandStack::undo()
{
    if (m_commands.isEmpty()) {
        throw std::runtime_error("Nothing to undo");
    }
    EditorCommand cmd = m_commands.takeLast();
    m_redo.push_back(cmd);
    return cmd;
}

EditorCommand EditorCommandStack::redo()
{
    if (m_redo.isEmpty()) {
        throw std::runtime_error("Nothing to redo");
    }
    EditorCommand cmd = m_redo.takeLast();
    m_commands.push_back(cmd);
    return cmd;
}

void EditorCommandStack::clear()
{
    m_commands.clear();
    m_redo.clear();
}

} // namespace imageviewer
