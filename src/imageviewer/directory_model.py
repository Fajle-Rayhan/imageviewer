from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

SUPPORTED_SUFFIXES = {
    ".png",
    ".jpg",
    ".jpeg",
    ".webp",
    ".gif",
    ".bmp",
    ".svg",
    ".tif",
    ".tiff",
    ".ico",
    ".ppm",
}


@dataclass(slots=True)
class DirectoryState:
    files: list[Path]
    index: int = 0

    @property
    def current(self) -> Path | None:
        if not self.files:
            return None
        return self.files[self.index]


class ImageDirectoryModel:
    def __init__(self) -> None:
        self.state = DirectoryState([])

    def load_for(self, image_path: Path) -> DirectoryState:
        image_path = image_path.resolve()
        entries = [
            p
            for p in sorted(image_path.parent.iterdir(), key=lambda p: p.name.casefold())
            if p.is_file() and p.suffix.lower() in SUPPORTED_SUFFIXES
        ]
        try:
            idx = entries.index(image_path)
        except ValueError:
            entries.insert(0, image_path)
            idx = 0
        self.state = DirectoryState(entries, idx)
        return self.state

    def next(self) -> Path | None:
        if not self.state.files:
            return None
        self.state.index = (self.state.index + 1) % len(self.state.files)
        return self.state.current

    def prev(self) -> Path | None:
        if not self.state.files:
            return None
        self.state.index = (self.state.index - 1) % len(self.state.files)
        return self.state.current

    def remove(self, path: Path, prefer: str = "next") -> Path | None:
        if path not in self.state.files:
            return self.state.current
        idx = self.state.files.index(path)
        self.state.files.pop(idx)
        if not self.state.files:
            self.state.index = 0
            return None
        if prefer == "prev":
            self.state.index = max(0, idx - 1)
        else:
            self.state.index = min(idx, len(self.state.files) - 1)
        return self.state.current

    def refresh(self) -> DirectoryState:
        current = self.state.current
        if not current:
            return self.state
        return self.load_for(current)
