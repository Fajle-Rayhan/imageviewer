from __future__ import annotations

import json
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Any

from .shortcuts import DEFAULT_SHORTCUTS


@dataclass(slots=True)
class OverlaySettings:
    visible: bool = True
    font_size: int = 8
    opacity: float = 0.6
    text_color: str = "#f0f0f0"
    position: str = "top-left"


@dataclass(slots=True)
class PanelSettings:
    auto_hide: bool = True
    pinned: bool = False
    edge_reveal: bool = True
    drag_reveal: bool = True
    animation_ms: int = 80
    thumbnail_size: int = 96


@dataclass(slots=True)
class Settings:
    startup_mode: str = "fullscreen"
    exit_behavior: str = "esc_exits_fullscreen_then_quit"
    display_mode: str = "natural"
    smooth_rendering: bool = True
    pixelated_rendering: bool = False
    rendering_backend: str = "gpu_preferred"
    canvas_background: str = "darkgray"
    custom_canvas_color: str = "#303030"
    theme: str = "dark"
    delete_confirm: bool = True
    delete_select: str = "next"
    satty_command: str = "satty"
    satty_use_original_dir: bool = True
    editor_default_tool: str = "pencil"
    shortcuts: dict[str, str] = field(default_factory=lambda: dict(DEFAULT_SHORTCUTS))
    overlay: OverlaySettings = field(default_factory=OverlaySettings)
    panel: PanelSettings = field(default_factory=PanelSettings)


class SettingsManager:
    def __init__(self, app_name: str = "imageviewer", config_dir: Path | None = None) -> None:
        base = config_dir or Path.home() / ".config"
        self.config_path = base / app_name / "config.json"
        self._settings = Settings()

    @property
    def settings(self) -> Settings:
        return self._settings

    def load(self) -> Settings:
        if not self.config_path.exists():
            return self._settings
        data = json.loads(self.config_path.read_text(encoding="utf-8"))
        self._settings = self._from_dict(data)
        return self._settings

    def save(self) -> None:
        self.config_path.parent.mkdir(parents=True, exist_ok=True)
        self.config_path.write_text(json.dumps(self.to_dict(), indent=2), encoding="utf-8")

    def to_dict(self) -> dict[str, Any]:
        data = asdict(self._settings)
        data["shortcuts"] = dict(DEFAULT_SHORTCUTS) | data.get("shortcuts", {})
        return data

    def _from_dict(self, data: dict[str, Any]) -> Settings:
        overlay = OverlaySettings(**(data.get("overlay") or {}))
        panel = PanelSettings(**(data.get("panel") or {}))
        shortcuts = dict(DEFAULT_SHORTCUTS) | dict(data.get("shortcuts") or {})
        raw = {k: v for k, v in data.items() if k not in {"overlay", "panel", "shortcuts"}}
        return Settings(**raw, overlay=overlay, panel=panel, shortcuts=shortcuts)
