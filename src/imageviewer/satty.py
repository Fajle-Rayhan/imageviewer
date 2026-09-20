from __future__ import annotations

import shutil
import subprocess
from pathlib import Path


class SattyError(RuntimeError):
    pass


class SattyLauncher:
    def __init__(self, command: str = "satty") -> None:
        self.command = command

    def available(self) -> bool:
        return shutil.which(self.command) is not None

    def launch(self, image_path: Path, save_dir: Path | None = None) -> subprocess.Popen[bytes]:
        if not self.available():
            raise SattyError("Satty command not found. Install satty and set the command in settings.")
        cmd = [self.command, "--filename", str(image_path)]
        if save_dir:
            cmd += ["--output-filename", str(save_dir / image_path.name)]
        return subprocess.Popen(cmd)
