from pathlib import Path

from imageviewer.directory_model import ImageDirectoryModel


def test_directory_navigation_and_delete_selection(tmp_path: Path):
    names = ["a.png", "b.jpg", "c.webp"]
    for name in names:
        (tmp_path / name).write_bytes(b"x")

    model = ImageDirectoryModel()
    state = model.load_for(tmp_path / "b.jpg")
    assert state.current.name == "b.jpg"

    assert model.next().name == "c.webp"
    assert model.prev().name == "b.jpg"

    nxt = model.remove(tmp_path / "b.jpg", prefer="next")
    assert nxt.name == "c.webp"

    prv = model.remove(tmp_path / "c.webp", prefer="prev")
    assert prv.name == "a.png"
