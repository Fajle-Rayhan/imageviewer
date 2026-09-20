from __future__ import annotations

import sys
from pathlib import Path

from PySide6.QtWidgets import QApplication

from .main_window import MainWindow


def main() -> int:
    app = QApplication(sys.argv)
    initial = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else None
    win = MainWindow(initial)
    win.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
