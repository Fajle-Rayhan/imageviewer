from imageviewer.shortcuts import DEFAULT_SHORTCUTS


def test_required_shortcut_defaults():
    assert DEFAULT_SHORTCUTS["toggle_menu"] == "M"
    assert DEFAULT_SHORTCUTS["toggle_thumbnails"] == "T"
    assert DEFAULT_SHORTCUTS["open_settings"] == "Ctrl+,"
    assert DEFAULT_SHORTCUTS["delete"] == "Delete"
    assert DEFAULT_SHORTCUTS["quit"] == "Q"
    assert DEFAULT_SHORTCUTS["escape"] == "Esc"
