from __future__ import annotations

import datetime as _dt
import os
import shutil
import subprocess
from pathlib import Path
from urllib.parse import quote


def _gio_trash(path: Path) -> bool:
    try:
        subprocess.run(["gio", "trash", str(path)], check=True, capture_output=True)
        return True
    except (FileNotFoundError, subprocess.CalledProcessError):
        return False


def _freedesktop_trash(path: Path) -> bool:
    home = Path.home()
    trash_files = home / ".local/share/Trash/files"
    trash_info = home / ".local/share/Trash/info"
    trash_files.mkdir(parents=True, exist_ok=True)
    trash_info.mkdir(parents=True, exist_ok=True)

    target = trash_files / path.name
    counter = 1
    while target.exists():
        target = trash_files / f"{path.stem}_{counter}{path.suffix}"
        counter += 1

    shutil.move(str(path), str(target))
    info_name = f"{target.name}.trashinfo"
    deletion_date = _dt.datetime.now().strftime("%Y-%m-%dT%H:%M:%S")
    payload = f"[Trash Info]\nPath={quote(str(path))}\nDeletionDate={deletion_date}\n"
    (trash_info / info_name).write_text(payload, encoding="utf-8")
    return True


def move_to_trash(path: Path) -> bool:
    path = path.resolve()
    if not path.exists():
        return False
    if os.name != "posix":
        return False
    return _gio_trash(path) or _freedesktop_trash(path)
