from __future__ import annotations

from dataclasses import dataclass


@dataclass(slots=True)
class Stroke:
    points: list[tuple[float, float]]
    color: str
    size: int
    opacity: float
    erase: bool = False


class EditorCommandStack:
    def __init__(self) -> None:
        self._strokes: list[Stroke] = []
        self._redo: list[Stroke] = []

    @property
    def strokes(self) -> list[Stroke]:
        return list(self._strokes)

    def add(self, stroke: Stroke) -> None:
        self._strokes.append(stroke)
        self._redo.clear()

    def undo(self) -> Stroke | None:
        if not self._strokes:
            return None
        stroke = self._strokes.pop()
        self._redo.append(stroke)
        return stroke

    def redo(self) -> Stroke | None:
        if not self._redo:
            return None
        stroke = self._redo.pop()
        self._strokes.append(stroke)
        return stroke

    def clear(self) -> None:
        self._strokes.clear()
        self._redo.clear()
