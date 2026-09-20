from __future__ import annotations

import subprocess
from pathlib import Path

from PySide6.QtCore import QEvent, QPoint, QPointF, QRectF, Qt, QTimer, Signal
from PySide6.QtGui import (QAction, QColor, QImage, QImageReader, QKeySequence,
                           QMouseEvent, QPainter, QPen, QPixmap, QTransform)
from PySide6.QtWidgets import (QAbstractItemView, QApplication, QColorDialog,
                               QComboBox, QDialog, QDockWidget, QFileDialog,
                               QFormLayout, QGraphicsPixmapItem, QGraphicsScene,
                               QGraphicsView, QHBoxLayout, QLabel, QListWidget,
                               QListWidgetItem, QMainWindow, QMessageBox,
                               QPushButton, QSlider, QToolBar, QVBoxLayout,
                               QWidget)

from .directory_model import ImageDirectoryModel
from .editor_commands import EditorCommandStack, Stroke
from .satty import SattyError, SattyLauncher
from .settings import SettingsManager
from .trash import move_to_trash


class MinimapWidget(QWidget):
    panRequested = Signal(float, float)

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.setFixedSize(180, 120)
        self.pixmap: QPixmap | None = None
        self.viewport = QRectF(0, 0, 1, 1)
        self.dragging = False

    def set_data(self, pixmap: QPixmap | None, viewport: QRectF) -> None:
        self.pixmap = pixmap
        self.viewport = viewport
        self.setVisible(pixmap is not None and (viewport.width() < 0.98 or viewport.height() < 0.98))
        self.update()

    def paintEvent(self, _) -> None:
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing, True)
        p.fillRect(self.rect(), QColor(0, 0, 0, 120))
        if not self.pixmap:
            return
        image_rect = self.rect().adjusted(6, 6, -6, -6)
        scaled = self.pixmap.scaled(image_rect.size(), Qt.KeepAspectRatio, Qt.SmoothTransformation)
        top_left = QPoint(
            image_rect.x() + (image_rect.width() - scaled.width()) // 2,
            image_rect.y() + (image_rect.height() - scaled.height()) // 2,
        )
        p.drawPixmap(top_left, scaled)
        vr = QRectF(
            top_left.x() + self.viewport.x() * scaled.width(),
            top_left.y() + self.viewport.y() * scaled.height(),
            max(8, self.viewport.width() * scaled.width()),
            max(8, self.viewport.height() * scaled.height()),
        )
        p.setPen(QPen(QColor("#6aa9ff"), 2))
        p.drawRect(vr)

    def mousePressEvent(self, event: QMouseEvent) -> None:
        self.dragging = True
        self._request(event.position())

    def mouseMoveEvent(self, event: QMouseEvent) -> None:
        if self.dragging:
            self._request(event.position())

    def mouseReleaseEvent(self, _: QMouseEvent) -> None:
        self.dragging = False

    def _request(self, pos: QPointF) -> None:
        x = max(0.0, min(1.0, pos.x() / max(1, self.width())))
        y = max(0.0, min(1.0, pos.y() / max(1, self.height())))
        self.panRequested.emit(x, y)


class ImageCanvas(QGraphicsView):
    viewportChanged = Signal()

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.setScene(QGraphicsScene(self))
        self.setRenderHint(QPainter.SmoothPixmapTransform, True)
        self.setDragMode(QGraphicsView.ScrollHandDrag)
        self._pix_item = QGraphicsPixmapItem()
        self.scene().addItem(self._pix_item)
        self._source_image: QImage | None = None
        self._edit_image: QImage | None = None
        self._zoom = 1.0
        self.editing = False
        self.stroke_size = 4
        self.stroke_color = QColor("#ff3b30")
        self.stroke_opacity = 1.0
        self.eraser = False
        self.command_stack = EditorCommandStack()
        self._active_points: list[tuple[float, float]] = []
        self._set_background("darkgray", QColor("#303030"))

    def wheelEvent(self, event):
        mods = QApplication.keyboardModifiers()
        if mods & Qt.ControlModifier:
            factor = 1.1 if event.angleDelta().y() > 0 else 0.9
            self.zoom_by(factor)
            event.accept()
            return
        super().wheelEvent(event)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        self.viewportChanged.emit()

    def scrollContentsBy(self, dx: int, dy: int) -> None:
        super().scrollContentsBy(dx, dy)
        self.viewportChanged.emit()

    def mousePressEvent(self, event: QMouseEvent) -> None:
        if self.editing and event.button() == Qt.LeftButton and self._source_image is not None:
            img_pt = self.mapToScene(event.position().toPoint())
            self._active_points = [(img_pt.x(), img_pt.y())]
            event.accept()
            return
        super().mousePressEvent(event)

    def mouseMoveEvent(self, event: QMouseEvent) -> None:
        if self.editing and self._active_points:
            img_pt = self.mapToScene(event.position().toPoint())
            self._active_points.append((img_pt.x(), img_pt.y()))
            self._redraw_edit()
            event.accept()
            return
        super().mouseMoveEvent(event)

    def mouseReleaseEvent(self, event: QMouseEvent) -> None:
        if self.editing and self._active_points:
            stroke = Stroke(
                points=self._active_points,
                color=self.stroke_color.name(),
                size=self.stroke_size,
                opacity=self.stroke_opacity,
                erase=self.eraser,
            )
            self.command_stack.add(stroke)
            self._active_points = []
            self._redraw_edit()
            event.accept()
            return
        super().mouseReleaseEvent(event)

    def set_image(self, image: QImage) -> None:
        self._source_image = image
        self._edit_image = image.copy()
        self.command_stack.clear()
        self._active_points = []
        self._pix_item.setPixmap(QPixmap.fromImage(image))
        self.scene().setSceneRect(QRectF(self._pix_item.pixmap().rect()))
        self._zoom = 1.0
        self.resetTransform()
        self.viewportChanged.emit()

    def natural_size(self) -> None:
        self.resetTransform()
        self._zoom = 1.0
        self.viewportChanged.emit()

    def fit_to_window(self) -> None:
        if self._pix_item.pixmap().isNull():
            return
        self.fitInView(self._pix_item, Qt.KeepAspectRatio)
        self._zoom = self.transform().m11()
        self.viewportChanged.emit()

    def zoom_by(self, factor: float) -> None:
        self._zoom *= factor
        self.scale(factor, factor)
        self.viewportChanged.emit()

    def set_smooth(self, enabled: bool, pixelated: bool) -> None:
        mode = Qt.FastTransformation if pixelated else Qt.SmoothTransformation
        if not enabled:
            mode = Qt.FastTransformation
        pix = self._pix_item.pixmap()
        if not pix.isNull() and self._source_image is not None:
            self._pix_item.setPixmap(QPixmap.fromImage(self._source_image).scaled(
                self._source_image.size(), Qt.KeepAspectRatio, mode
            ))

    def _set_background(self, mode: str, custom: QColor) -> None:
        palette = {
            "darkgray": QColor("#303030"),
            "black": QColor("black"),
            "white": QColor("white"),
            "custom": custom,
        }
        color = palette.get(mode, QColor("#303030"))
        self.setBackgroundBrush(color)

    def set_background(self, mode: str, custom: str) -> None:
        self._set_background(mode, QColor(custom))

    def viewport_ratio(self) -> QRectF:
        if self._pix_item.pixmap().isNull():
            return QRectF(0, 0, 1, 1)
        scene_rect = self.sceneRect()
        view_rect = self.mapToScene(self.viewport().rect()).boundingRect().intersected(scene_rect)
        if scene_rect.width() <= 0 or scene_rect.height() <= 0:
            return QRectF(0, 0, 1, 1)
        return QRectF(
            (view_rect.x() - scene_rect.x()) / scene_rect.width(),
            (view_rect.y() - scene_rect.y()) / scene_rect.height(),
            view_rect.width() / scene_rect.width(),
            view_rect.height() / scene_rect.height(),
        )

    def pan_to_ratio(self, x: float, y: float) -> None:
        scene = self.sceneRect()
        vp = self.viewport_ratio()
        cx = scene.x() + (x * scene.width())
        cy = scene.y() + (y * scene.height())
        self.centerOn(cx, cy)
        self.viewportChanged.emit()

    def _redraw_edit(self) -> None:
        if self._source_image is None:
            return
        img = self._source_image.copy()
        painter = QPainter(img)
        for stroke in self.command_stack.strokes:
            self._paint_stroke(painter, stroke)
        if self._active_points:
            preview = Stroke(self._active_points, self.stroke_color.name(), self.stroke_size, self.stroke_opacity, self.eraser)
            self._paint_stroke(painter, preview)
        painter.end()
        self._edit_image = img
        self._pix_item.setPixmap(QPixmap.fromImage(img))

    def _paint_stroke(self, painter: QPainter, stroke: Stroke) -> None:
        if len(stroke.points) < 2:
            return
        color = QColor(stroke.color)
        color.setAlphaF(max(0.0, min(1.0, stroke.opacity)))
        pen = QPen(color, stroke.size, Qt.SolidLine, Qt.RoundCap, Qt.RoundJoin)
        if stroke.erase:
            painter.setCompositionMode(QPainter.CompositionMode_Clear)
        else:
            painter.setCompositionMode(QPainter.CompositionMode_SourceOver)
        painter.setPen(pen)
        prev = QPointF(*stroke.points[0])
        for px, py in stroke.points[1:]:
            current = QPointF(px, py)
            painter.drawLine(prev, current)
            prev = current

    def undo(self) -> None:
        self.command_stack.undo()
        self._redraw_edit()

    def redo(self) -> None:
        self.command_stack.redo()
        self._redraw_edit()

    def save_copy(self, path: Path) -> bool:
        if self._edit_image is None:
            return False
        return self._edit_image.save(str(path))

    def save_overwrite(self, path: Path) -> bool:
        if not self.save_copy(path):
            return False
        self._source_image = self._edit_image.copy() if self._edit_image else self._source_image
        return True

    def crop_visible(self) -> None:
        if self._edit_image is None:
            return
        rect = self.mapToScene(self.viewport().rect()).boundingRect().toAlignedRect()
        rect = rect.intersected(self._edit_image.rect())
        if rect.isEmpty():
            return
        self._source_image = self._edit_image.copy(rect)
        self._edit_image = self._source_image.copy()
        self.command_stack.clear()
        self._pix_item.setPixmap(QPixmap.fromImage(self._source_image))
        self.scene().setSceneRect(QRectF(self._pix_item.pixmap().rect()))
        self.fit_to_window()


class SettingsDialog(QDialog):
    def __init__(self, manager: SettingsManager, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.setWindowTitle("Settings")
        self.manager = manager
        s = manager.settings

        form = QFormLayout(self)
        self.startup = QComboBox()
        self.startup.addItems(["fullscreen", "maximized", "normal"])
        self.startup.setCurrentText(s.startup_mode)

        self.display_mode = QComboBox()
        self.display_mode.addItems(["natural", "fit_to_window", "fit_to_screen"])
        self.display_mode.setCurrentText(s.display_mode)

        self.background = QComboBox()
        self.background.addItems(["darkgray", "black", "white", "custom"])
        self.background.setCurrentText(s.canvas_background)

        self.theme = QComboBox()
        self.theme.addItems(["dark", "light", "system"])
        self.theme.setCurrentText(s.theme)

        self.smooth = QComboBox()
        self.smooth.addItems(["smooth", "pixelated", "disabled"])
        self.smooth.setCurrentText("pixelated" if s.pixelated_rendering else ("smooth" if s.smooth_rendering else "disabled"))

        self.thumb = QSlider(Qt.Horizontal)
        self.thumb.setRange(48, 256)
        self.thumb.setValue(s.panel.thumbnail_size)

        self.color_btn = QPushButton("Choose custom canvas color")
        self.color_btn.clicked.connect(self._pick_color)

        save_btn = QPushButton("Save")
        save_btn.clicked.connect(self.accept)

        form.addRow("Startup", self.startup)
        form.addRow("Display mode", self.display_mode)
        form.addRow("Theme", self.theme)
        form.addRow("Canvas background", self.background)
        form.addRow("Thumbnail size", self.thumb)
        form.addRow("Render", self.smooth)
        form.addRow(self.color_btn)
        form.addRow(save_btn)

    def _pick_color(self) -> None:
        color = QColorDialog.getColor()
        if color.isValid():
            self.manager.settings.custom_canvas_color = color.name()

    def apply(self) -> None:
        s = self.manager.settings
        s.startup_mode = self.startup.currentText()
        s.display_mode = self.display_mode.currentText()
        s.theme = self.theme.currentText()
        s.canvas_background = self.background.currentText()
        s.panel.thumbnail_size = self.thumb.value()
        render_mode = self.smooth.currentText()
        s.smooth_rendering = render_mode == "smooth"
        s.pixelated_rendering = render_mode == "pixelated"


class MainWindow(QMainWindow):
    def __init__(self, initial_path: Path | None = None) -> None:
        super().__init__()
        self.setWindowTitle("imageviewer")
        self.settings_manager = SettingsManager()
        self.settings_manager.load()
        self.model = ImageDirectoryModel()
        self.satty = SattyLauncher(self.settings_manager.settings.satty_command)

        self.canvas = ImageCanvas(self)
        self.setCentralWidget(self.canvas)
        self.overlay = QLabel(self.canvas.viewport())
        self.overlay.setStyleSheet("background: rgba(0,0,0,128); color: #f0f0f0; padding: 2px 6px; border-radius: 6px;")
        self.overlay.move(12, 8)

        self.minimap = MinimapWidget(self.canvas.viewport())
        self.minimap.move(self.canvas.viewport().width() - self.minimap.width() - 10, self.canvas.viewport().height() - self.minimap.height() - 10)
        self.minimap.panRequested.connect(self.canvas.pan_to_ratio)

        self.toolbar = QToolBar("Tools", self)
        self.toolbar.setMovable(False)
        self.toolbar.setIconSize(self.toolbar.iconSize())
        self.addToolBar(Qt.TopToolBarArea, self.toolbar)

        self.thumbnails = QListWidget()
        self.thumbnails.setViewMode(QListWidget.IconMode)
        self.thumbnails.setFlow(QListWidget.LeftToRight)
        self.thumbnails.setMovement(QListWidget.Static)
        self.thumbnails.setResizeMode(QListWidget.Adjust)
        self.thumbnails.setHorizontalScrollMode(QAbstractItemView.ScrollPerPixel)
        self.thumbnails.itemActivated.connect(self._thumbnail_open)
        self.thumbnails.itemClicked.connect(self._thumbnail_open)

        self.thumb_dock = QDockWidget("Thumbnails", self)
        self.thumb_dock.setWidget(self.thumbnails)
        self.thumb_dock.setFeatures(QDockWidget.NoDockWidgetFeatures)
        self.addDockWidget(Qt.TopDockWidgetArea, self.thumb_dock)

        self._top_panels_visible = True
        self._install_actions()
        self._apply_settings()
        self.canvas.viewportChanged.connect(self._update_overlay)
        self.canvas.viewportChanged.connect(self._update_minimap)
        self.installEventFilter(self)

        if initial_path and initial_path.exists():
            self.open_path(initial_path)
        if self.settings_manager.settings.startup_mode == "fullscreen":
            self.showFullScreen()
        elif self.settings_manager.settings.startup_mode == "maximized":
            self.showMaximized()

    def _install_actions(self) -> None:
        shortcuts = self.settings_manager.settings.shortcuts

        self._add_action("Open", self.open_dialog)
        self._add_action("Copy", self.copy_image)
        self._add_action("Mirror", self.mirror)
        self._add_action("Rotate", self.rotate_right)
        self._add_action("Crop", self.canvas.crop_visible)
        self._add_action("Zoom +", lambda: self.canvas.zoom_by(1.1), shortcuts.get("zoom_in"))
        self._add_action("Zoom -", lambda: self.canvas.zoom_by(0.9), shortcuts.get("zoom_out"))
        self._add_action("Open with", self.open_with)
        self._add_action("Edit with Satty", self.edit_with_satty, shortcuts.get("satty"))
        self._add_action("Pencil", self.toggle_editor, shortcuts.get("pencil"))
        self._add_action("Undo", self.canvas.undo)
        self._add_action("Redo", self.canvas.redo)
        self._add_action("Eraser", self.toggle_eraser)
        self._add_action("Brush+", lambda: self.set_brush_size(self.canvas.stroke_size + 2))
        self._add_action("Brush-", lambda: self.set_brush_size(max(1, self.canvas.stroke_size - 2)))
        self._add_action("Opacity+", lambda: self.set_brush_opacity(min(1.0, self.canvas.stroke_opacity + 0.1)))
        self._add_action("Opacity-", lambda: self.set_brush_opacity(max(0.1, self.canvas.stroke_opacity - 0.1)))
        self._add_action("Color", self.pick_brush_color)
        self._add_action("Save Copy", self.save_copy)
        self._add_action("Save As", self.save_as)
        self._add_action("Metadata", self.show_metadata)
        self._add_action("Trash", self.delete_current, shortcuts.get("delete"))
        self._add_action("Settings", self.open_settings, shortcuts.get("open_settings"))
        self._add_action("Exit", self.close, shortcuts.get("quit"))

        self._bind_shortcut("Next", shortcuts.get("next_image"), self.next_image)
        self._bind_shortcut("Prev", shortcuts.get("prev_image"), self.prev_image)
        self._bind_shortcut("Natural", shortcuts.get("natural_size"), self.canvas.natural_size)
        self._bind_shortcut("Fit", shortcuts.get("fit_to_window"), self.canvas.fit_to_window)
        self._bind_shortcut("Menu", shortcuts.get("toggle_menu"), self.toggle_menu)
        self._bind_shortcut("Thumb", shortcuts.get("toggle_thumbnails"), self.toggle_thumbnails)
        self._bind_shortcut("Fullscreen", shortcuts.get("toggle_fullscreen"), self.toggle_fullscreen)

    def _bind_shortcut(self, text: str, key: str | None, callback) -> None:
        if not key:
            return
        action = QAction(text, self)
        action.setShortcut(QKeySequence(key))
        action.triggered.connect(callback)
        self.addAction(action)

    def _add_action(self, text: str, callback, shortcut: str | None = None) -> None:
        action = QAction(text, self)
        action.setToolTip(text)
        action.triggered.connect(callback)
        if shortcut:
            action.setShortcut(QKeySequence(shortcut))
            self.addAction(action)
        self.toolbar.addAction(action)

    def _apply_settings(self) -> None:
        s = self.settings_manager.settings
        self.canvas.set_background(s.canvas_background, s.custom_canvas_color)
        self.canvas.set_smooth(s.smooth_rendering, s.pixelated_rendering)
        self.thumbnails.setIconSize(QPixmap(1, 1).size().scaled(s.panel.thumbnail_size, s.panel.thumbnail_size, Qt.KeepAspectRatio))
        self.overlay.setVisible(s.overlay.visible)
        self.overlay.setStyleSheet(
            f"background: rgba(0,0,0,{int(max(0.0,min(1.0,s.overlay.opacity))*255)});"
            f" color: {s.overlay.text_color}; padding: 2px 6px; border-radius: 6px; font-size: {s.overlay.font_size}pt;"
        )

    def open_dialog(self) -> None:
        path, _ = QFileDialog.getOpenFileName(self, "Open image")
        if path:
            self.open_path(Path(path))

    def open_path(self, path: Path) -> None:
        state = self.model.load_for(path)
        self._load_current_image()
        self._rebuild_thumbnails(state.index)

    def _thumbnail_open(self, item: QListWidgetItem) -> None:
        path = Path(item.data(Qt.UserRole))
        if path in self.model.state.files:
            self.model.state.index = self.model.state.files.index(path)
            self._load_current_image()

    def _load_current_image(self) -> None:
        path = self.model.state.current
        if not path:
            self.canvas.set_image(QImage())
            return
        if not path.exists():
            self.model.refresh()
            return
        reader = QImageReader(str(path))
        reader.setAutoTransform(True)
        image = reader.read()
        if image.isNull():
            QMessageBox.warning(self, "Open failed", f"Could not open {path.name}")
            return
        self.canvas.set_image(image)
        mode = self.settings_manager.settings.display_mode
        if mode == "fit_to_window" or mode == "fit_to_screen":
            QTimer.singleShot(0, self.canvas.fit_to_window)
        self._update_overlay()
        self._update_minimap()
        self._preload_adjacent()

    def _preload_adjacent(self) -> None:
        files = self.model.state.files
        if not files:
            return
        i = self.model.state.index
        for idx in {max(0, i - 1), min(len(files) - 1, i + 1)}:
            if idx == i:
                continue
            QTimer.singleShot(0, lambda p=files[idx]: QImageReader(str(p)).read())

    def _rebuild_thumbnails(self, selected: int) -> None:
        self.thumbnails.clear()
        for idx, path in enumerate(self.model.state.files):
            item = QListWidgetItem(path.name)
            item.setData(Qt.UserRole, str(path))
            pix = QPixmap(str(path))
            if not pix.isNull():
                item.setIcon(pix.scaled(self.settings_manager.settings.panel.thumbnail_size,
                                        self.settings_manager.settings.panel.thumbnail_size,
                                        Qt.KeepAspectRatio,
                                        Qt.SmoothTransformation))
            self.thumbnails.addItem(item)
            if idx == selected:
                self.thumbnails.setCurrentItem(item)

    def wheelEvent(self, event):
        if QApplication.keyboardModifiers() & Qt.ControlModifier:
            super().wheelEvent(event)
            return
        if event.angleDelta().y() < 0:
            self.next_image()
        else:
            self.prev_image()

    def next_image(self) -> None:
        self.model.next()
        self._load_current_image()
        self._select_thumb()

    def prev_image(self) -> None:
        self.model.prev()
        self._load_current_image()
        self._select_thumb()

    def _select_thumb(self) -> None:
        idx = self.model.state.index
        if 0 <= idx < self.thumbnails.count():
            self.thumbnails.setCurrentRow(idx)

    def copy_image(self) -> None:
        current = self.model.state.current
        if not current:
            return
        QApplication.clipboard().setText(str(current))

    def mirror(self) -> None:
        if self.canvas._source_image is None:
            return
        mirrored = self.canvas._source_image.mirrored(True, False)
        self.canvas.set_image(mirrored)

    def rotate_right(self) -> None:
        if self.canvas._source_image is None:
            return
        self.canvas.set_image(self.canvas._source_image.transformed(QTransform().rotate(90)))

    def open_with(self) -> None:
        current = self.model.state.current
        if not current:
            return
        subprocess.Popen(["xdg-open", str(current)])

    def edit_with_satty(self) -> None:
        current = self.model.state.current
        if not current:
            return
        try:
            save_dir = current.parent if self.settings_manager.settings.satty_use_original_dir else None
            self.satty.launch(current, save_dir=save_dir)
        except SattyError as exc:
            QMessageBox.information(self, "Satty", str(exc))

    def toggle_editor(self) -> None:
        self.canvas.editing = not self.canvas.editing
        QMessageBox.information(
            self,
            "Editor",
            "Editor enabled. Use left mouse to draw, Undo/Redo, Eraser, Brush, Opacity, Color, Crop, Save Copy or Save As.",
        )

    def set_brush_size(self, size: int) -> None:
        self.canvas.stroke_size = size

    def set_brush_opacity(self, value: float) -> None:
        self.canvas.stroke_opacity = value

    def pick_brush_color(self) -> None:
        color = QColorDialog.getColor(self.canvas.stroke_color, self)
        if color.isValid():
            self.canvas.stroke_color = color
            self.canvas.eraser = False

    def toggle_eraser(self) -> None:
        self.canvas.eraser = not self.canvas.eraser

    def save_copy(self) -> None:
        current = self.model.state.current
        if not current:
            return
        target, _ = QFileDialog.getSaveFileName(self, "Save copy", str(current.with_name(f"{current.stem}_copy{current.suffix}")))
        if target:
            self.canvas.save_copy(Path(target))

    def save_as(self) -> None:
        current = self.model.state.current
        if not current:
            return
        target, _ = QFileDialog.getSaveFileName(self, "Save As", str(current))
        if not target:
            return
        if self.canvas.save_overwrite(Path(target)):
            self.open_path(Path(target))

    def show_metadata(self) -> None:
        current = self.model.state.current
        if not current or self.canvas._source_image is None:
            return
        size_mb = current.stat().st_size / (1024 * 1024)
        img = self.canvas._source_image
        QMessageBox.information(
            self,
            "Metadata",
            f"{current.name}\n{img.width()}x{img.height()}\n{size_mb:.2f} MB\n{self.model.state.index + 1}/{len(self.model.state.files)}",
        )

    def delete_current(self) -> None:
        current = self.model.state.current
        if not current:
            return
        if self.settings_manager.settings.delete_confirm:
            answer = QMessageBox.question(self, "Move to trash", f"Move {current.name} to trash?")
            if answer != QMessageBox.Yes:
                return
        if not move_to_trash(current):
            QMessageBox.warning(self, "Trash", "Failed to move file to trash")
            return
        next_item = self.model.remove(current, self.settings_manager.settings.delete_select)
        self._rebuild_thumbnails(self.model.state.index)
        if next_item:
            self._load_current_image()
        else:
            self.canvas.set_image(QImage())
            self.overlay.setText("No images")

    def open_settings(self) -> None:
        dialog = SettingsDialog(self.settings_manager, self)
        if dialog.exec() == QDialog.Accepted:
            dialog.apply()
            self.settings_manager.save()
            self._apply_settings()

    def toggle_menu(self) -> None:
        self.toolbar.setVisible(not self.toolbar.isVisible())

    def toggle_thumbnails(self) -> None:
        self.thumb_dock.setVisible(not self.thumb_dock.isVisible())

    def toggle_fullscreen(self) -> None:
        if self.isFullScreen():
            self.showNormal()
        else:
            self.showFullScreen()

    def _update_overlay(self) -> None:
        current = self.model.state.current
        image = self.canvas._source_image
        if not current or image is None:
            self.overlay.setText("No image")
            return
        size = current.stat().st_size if current.exists() else 0
        zoom = self.canvas.transform().m11() * 100
        self.overlay.setText(
            f"{current.name} · {image.width()}x{image.height()} · {size / (1024 * 1024):.2f} MB · {zoom:.0f}% · {self.model.state.index + 1}/{len(self.model.state.files)}"
        )
        self.overlay.adjustSize()
        self.minimap.move(self.canvas.viewport().width() - self.minimap.width() - 10,
                          self.canvas.viewport().height() - self.minimap.height() - 10)

    def _update_minimap(self) -> None:
        self.minimap.set_data(self.canvas._pix_item.pixmap(), self.canvas.viewport_ratio())

    def eventFilter(self, obj, event):
        if obj is self and event.type() == QEvent.MouseMove:
            y = self.mapFromGlobal(event.globalPosition().toPoint()).y()
            s = self.settings_manager.settings.panel
            reveal = s.edge_reveal and y <= 24
            hide = s.auto_hide and not s.pinned and y > (self.toolbar.height() + self.thumb_dock.height() + 20)
            if reveal:
                self.toolbar.show()
                self.thumb_dock.show()
            elif hide:
                self.toolbar.hide()
                self.thumb_dock.hide()
        return super().eventFilter(obj, event)

    def keyPressEvent(self, event):
        esc = self.settings_manager.settings.shortcuts.get("escape", "Esc")
        if event.matches(QKeySequence.Cancel) and esc.lower() == "esc":
            if self.isFullScreen() and self.settings_manager.settings.exit_behavior.startswith("esc_exits_fullscreen"):
                self.showNormal()
                if self.settings_manager.settings.exit_behavior.endswith("quit"):
                    self.close()
                return
        super().keyPressEvent(event)
