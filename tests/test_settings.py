from pathlib import Path

from imageviewer.settings import SettingsManager


def test_settings_persistence(tmp_path: Path):
    manager = SettingsManager(config_dir=tmp_path)
    manager.settings.startup_mode = "normal"
    manager.settings.shortcuts["toggle_menu"] = "Ctrl+M"
    manager.save()

    loaded = SettingsManager(config_dir=tmp_path)
    loaded.load()

    assert loaded.settings.startup_mode == "normal"
    assert loaded.settings.shortcuts["toggle_menu"] == "Ctrl+M"
