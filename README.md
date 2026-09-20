# imageviewer

A Linux-first image viewer for Arch Linux + Hyprland, built with Python 3 and Qt (PySide6).

## Features

- Open files from CLI (`imageviewer /path/image.png`) and file manager (`.desktop` entry).
- Directory discovery + previous/next navigation (wheel, arrows, shortcuts).
- Formats via Qt image plugins: PNG/JPEG/WebP/GIF/BMP/SVG/TIFF and other Qt-supported formats.
- Configurable startup mode (`fullscreen` default, `maximized`, `normal`).
- Configurable shortcuts (defaults include `Q`, `Esc`, `M`, `T`, `Ctrl+,`, `Delete`).
- Display modes: natural size (default), fit-to-window, fit-to-screen.
- Ctrl+wheel zoom; wheel image navigation by default.
- Drag panning through Qt graphics view hand-drag behavior.
- Smooth rendering default with pixelated/disabled option.
- Canvas backgrounds: dark gray (default), black, white, custom color.
- Info overlay with filename, dimensions, size, zoom, and index.
- Minimap when zoomed with draggable viewport indicator.
- Icon-first toolbar/menu actions (copy, mirror, rotate, crop, zoom, open with, satty, pencil, metadata, trash, settings, exit).
- Thumbnail strip with active selection and quick switching.
- Auto-hide/pinned edge reveal behavior for toolbar and thumbnail strip.
- Delete-to-trash via `gio trash` fallback to Freedesktop Trash spec layout.
- Satty integration with original directory save target preference.
- In-canvas editor overlay: pencil/eraser (toggle erase flag), size/color/opacity, undo/redo stack core, crop-visible, save copy/overwrite hooks.
- JSON settings persistence in `~/.config/imageviewer/config.json`.

## Build / run (Arch Linux)

```bash
sudo pacman -S python python-pyside6 python-pip
python -m pip install -e .
imageviewer /path/to/image.png
```

## Development

```bash
python -m pip install -e .[dev]
pytest
```

## Configuration

Saved at:

```text
~/.config/imageviewer/config.json
```

Key settings include startup mode, display mode, rendering mode, background, overlay style, panel behavior, delete behavior, Satty command, and all shortcuts.

## Keyboard defaults

- `Q`: exit
- `Esc`: exit fullscreen (configurable behavior)
- `M`: toggle toolbar
- `T`: toggle thumbnails
- `Ctrl+,`: settings
- `Delete`: move image to trash
- `Right/Left`: next/previous image
- `Ctrl+Wheel`: zoom
- `Wheel`: previous/next image
- `0`: natural size
- `1`: fit to window

## Satty setup

Install Satty and keep command as `satty`, or change `satty_command` in settings JSON.

```bash
sudo pacman -S satty
```

If Satty is missing, the app shows a friendly error message.

## MIME association

Install desktop + mime files:

```bash
install -Dm644 packaging/imageviewer.desktop ~/.local/share/applications/imageviewer.desktop
install -Dm644 packaging/imageviewer-mime.xml ~/.local/share/mime/packages/imageviewer-mime.xml
update-desktop-database ~/.local/share/applications
update-mime-database ~/.local/share/mime
xdg-mime default imageviewer.desktop image/png image/jpeg image/webp image/gif image/bmp image/svg+xml image/tiff
```

## Packaging

`packaging/arch/PKGBUILD` is provided. Create a reproducible source archive first:

```bash
git archive --format=tar.gz --output=imageviewer-0.1.0.tar.gz --prefix=imageviewer-0.1.0/ HEAD
```

Then build via `makepkg -si`.

## Hyprland / Wayland notes

Qt works on Wayland and X11. For native Wayland:

```bash
QT_QPA_PLATFORM=wayland imageviewer /path/to/image.png
```

## Testing coverage in this repository

Included focused tests cover:

- settings persistence
- directory navigation and post-delete selection
- shortcut defaults
- editor command stack undo/redo core

## Limitations / deferred items

- Thumbnail generation is currently synchronous per item while list updates are batched quickly.
- In-canvas editor currently focuses on freehand draw/erase + crop/undo/redo; shape/text tools are not yet complete.
- Toolbar reveal is edge/auto-hide tuned for responsiveness; advanced animation profiles are basic.
- Satty argument compatibility may vary across Satty versions.
