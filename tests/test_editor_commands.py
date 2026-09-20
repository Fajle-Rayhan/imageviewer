from imageviewer.editor_commands import EditorCommandStack, Stroke


def test_editor_command_stack_undo_redo():
    stack = EditorCommandStack()
    s1 = Stroke(points=[(0, 0), (1, 1)], color="#fff", size=2, opacity=1.0)
    s2 = Stroke(points=[(1, 1), (2, 2)], color="#000", size=4, opacity=0.8)

    stack.add(s1)
    stack.add(s2)
    assert len(stack.strokes) == 2

    removed = stack.undo()
    assert removed == s2
    assert len(stack.strokes) == 1

    restored = stack.redo()
    assert restored == s2
    assert len(stack.strokes) == 2
