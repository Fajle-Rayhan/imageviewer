#include "imagecanvasview.h"

#include <QInputDialog>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

namespace imageviewer {

ImageCanvasView::ImageCanvasView(QWidget* parent)
    : QGraphicsView(parent)
{
    setScene(&m_scene);
    setFrameShape(QFrame::NoFrame);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setAlignment(Qt::AlignCenter);

    m_baseItem = m_scene.addPixmap(QPixmap());
    m_overlayItem = m_scene.addPixmap(QPixmap());
    m_overlayItem->setZValue(1.0);
    setSmoothRendering(true);
    setCanvasBackground(QColor("#303030"));
}

bool ImageCanvasView::loadImage(const QString& filePath)
{
    QImage image(filePath);
    if (image.isNull()) {
        return false;
    }

    m_currentPath = filePath;
    m_baseImage = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    m_overlayImage = QImage(m_baseImage.size(), QImage::Format_ARGB32_Premultiplied);
    m_overlayImage.fill(Qt::transparent);
    m_stack.clear();

    m_baseItem->setPixmap(QPixmap::fromImage(m_baseImage));
    m_overlayItem->setPixmap(QPixmap::fromImage(m_overlayImage));
    m_scene.setSceneRect(QRectF(QPointF(0, 0), QSizeF(m_baseImage.size())));
    applyViewMode();
    return true;
}

bool ImageCanvasView::saveImage(const QString& filePath)
{
    if (m_baseImage.isNull()) {
        return false;
    }
    QImage composed = m_baseImage;
    QPainter painter(&composed);
    painter.drawImage(QPoint(0, 0), m_overlayImage);
    painter.end();
    if (!composed.save(filePath)) {
        return false;
    }

    if (filePath == m_currentPath) {
        m_baseImage = composed;
        m_overlayImage.fill(Qt::transparent);
        m_stack.clear();
        m_baseItem->setPixmap(QPixmap::fromImage(m_baseImage));
        m_overlayItem->setPixmap(QPixmap::fromImage(m_overlayImage));
    }
    return true;
}

void ImageCanvasView::setViewMode(ViewMode mode)
{
    m_viewMode = mode;
    applyViewMode();
}

void ImageCanvasView::setSmoothRendering(bool enabled)
{
    setRenderHint(QPainter::SmoothPixmapTransform, enabled);
    setRenderHint(QPainter::Antialiasing, enabled);
}

void ImageCanvasView::setCanvasBackground(const QColor& color)
{
    setBackgroundBrush(color);
}

void ImageCanvasView::setEditorTool(EditorTool tool)
{
    m_editorTool = tool;
}

void ImageCanvasView::applyEditorDefaults(const EditorDefaults& defaults)
{
    m_pencilColor = defaults.pencilColor;
    m_pencilSize = defaults.pencilSize;
    m_pencilOpacity = defaults.pencilOpacity;
    m_markerColor = defaults.markerColor;
    m_markerSize = defaults.markerSize;
    m_markerOpacity = defaults.markerOpacity;
    m_textColor = defaults.textColor;
    m_textSize = defaults.textSize;
    m_textBold = defaults.textBold;
    m_textItalic = defaults.textItalic;
    m_textUnderline = defaults.textUnderline;
}

bool ImageCanvasView::hasImage() const
{
    return !m_baseImage.isNull();
}

void ImageCanvasView::undo()
{
    if (!m_stack.canUndo()) {
        return;
    }
    m_stack.undo();
    rebuildOverlay();
}

void ImageCanvasView::redo()
{
    if (!m_stack.canRedo()) {
        return;
    }
    m_stack.redo();
    rebuildOverlay();
}

void ImageCanvasView::clearEdits()
{
    m_stack.clear();
    m_overlayImage.fill(Qt::transparent);
    m_overlayItem->setPixmap(QPixmap::fromImage(m_overlayImage));
}

bool ImageCanvasView::hasPendingEdits() const
{
    return !m_stack.commands().isEmpty();
}

void ImageCanvasView::mirrorCurrent()
{
    if (m_baseImage.isNull()) {
        return;
    }
    m_baseImage = m_baseImage.mirrored(true, false);
    m_baseItem->setPixmap(QPixmap::fromImage(m_baseImage));
}

void ImageCanvasView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    applyViewMode();
}

void ImageCanvasView::wheelEvent(QWheelEvent* event)
{
    if (!hasImage()) {
        event->ignore();
        return;
    }

    if (event->modifiers() & Qt::ControlModifier) {
        const double factor = event->angleDelta().y() > 0 ? 1.15 : (1.0 / 1.15);
        scale(factor, factor);
        event->accept();
        return;
    }

    if (event->angleDelta().y() < 0) {
        emit requestNextImage();
    } else {
        emit requestPreviousImage();
    }
    event->accept();
}

void ImageCanvasView::mousePressEvent(QMouseEvent* event)
{
    if (!hasImage() || event->button() != Qt::LeftButton) {
        QGraphicsView::mousePressEvent(event);
        return;
    }

    const QPointF p = toImagePoint(mapToScene(event->pos()));
    if (p.x() < 0 || p.y() < 0 || p.x() >= m_baseImage.width() || p.y() >= m_baseImage.height()) {
        QGraphicsView::mousePressEvent(event);
        return;
    }

    if (m_editorTool == EditorTool::Text) {
        bool ok = false;
        const QString text = QInputDialog::getText(this, tr("Insert text"), tr("Text"), QLineEdit::Normal, {}, &ok);
        if (ok && !text.isEmpty()) {
            EditorCommand cmd;
            cmd.type = EditorCommandType::Text;
            cmd.text.position = p;
            cmd.text.text = text;
            QFont font;
            font.setPointSize(m_textSize);
            font.setBold(m_textBold);
            font.setItalic(m_textItalic);
            font.setUnderline(m_textUnderline);
            cmd.text.font = font;
            cmd.text.color = m_textColor;
            m_stack.add(cmd);
            rebuildOverlay();
            emit statusMessage(tr("Text added"));
        }
        return;
    }

    m_drawing = true;
    m_currentStroke.points.clear();
    m_currentStroke.points.push_back(p);
    if (m_editorTool == EditorTool::Marker) {
        m_currentStroke.color = m_markerColor;
        m_currentStroke.size = m_markerSize;
        m_currentStroke.opacity = m_markerOpacity;
    } else {
        m_currentStroke.color = m_pencilColor;
        m_currentStroke.size = m_pencilSize;
        m_currentStroke.opacity = m_pencilOpacity;
    }
}

void ImageCanvasView::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_drawing) {
        QGraphicsView::mouseMoveEvent(event);
        return;
    }
    m_currentStroke.points.push_back(toImagePoint(mapToScene(event->pos())));
    rebuildOverlay();

    QPainter painter(&m_overlayImage);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(m_currentStroke.color, m_currentStroke.size, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QColor c = pen.color();
    c.setAlphaF(std::clamp(m_currentStroke.opacity, 0.0, 1.0));
    pen.setColor(c);
    painter.setPen(pen);
    for (int i = 1; i < m_currentStroke.points.size(); ++i) {
        painter.drawLine(m_currentStroke.points[i - 1], m_currentStroke.points[i]);
    }
    painter.end();
    m_overlayItem->setPixmap(QPixmap::fromImage(m_overlayImage));
}

void ImageCanvasView::mouseReleaseEvent(QMouseEvent* event)
{
    if (!m_drawing || event->button() != Qt::LeftButton) {
        QGraphicsView::mouseReleaseEvent(event);
        return;
    }
    m_drawing = false;
    EditorCommand cmd;
    cmd.type = EditorCommandType::Stroke;
    cmd.stroke = m_currentStroke;
    m_stack.add(cmd);
    rebuildOverlay();
    emit statusMessage(tr("Stroke added"));
}

void ImageCanvasView::rebuildOverlay()
{
    if (m_baseImage.isNull()) {
        return;
    }

    m_overlayImage = QImage(m_baseImage.size(), QImage::Format_ARGB32_Premultiplied);
    m_overlayImage.fill(Qt::transparent);

    QPainter painter(&m_overlayImage);
    painter.setRenderHint(QPainter::Antialiasing, true);
    for (const EditorCommand& cmd : m_stack.commands()) {
        if (cmd.type == EditorCommandType::Stroke) {
            if (cmd.stroke.points.size() < 2) {
                continue;
            }
            QPen pen(cmd.stroke.color, cmd.stroke.size, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            QColor c = pen.color();
            c.setAlphaF(std::clamp(cmd.stroke.opacity, 0.0, 1.0));
            pen.setColor(c);
            painter.setPen(pen);
            for (int i = 1; i < cmd.stroke.points.size(); ++i) {
                painter.drawLine(cmd.stroke.points[i - 1], cmd.stroke.points[i]);
            }
        } else {
            painter.setPen(cmd.text.color);
            painter.setFont(cmd.text.font);
            painter.drawText(cmd.text.position, cmd.text.text);
        }
    }
    painter.end();
    m_overlayItem->setPixmap(QPixmap::fromImage(m_overlayImage));
}

void ImageCanvasView::applyViewMode()
{
    if (m_baseImage.isNull()) {
        return;
    }

    resetTransform();
    switch (m_viewMode) {
    case ViewMode::Normal:
        fitInView(m_scene.sceneRect(), Qt::KeepAspectRatio);
        break;
    case ViewMode::TrueSize:
        break;
    case ViewMode::ScreenWide: {
        const qreal sceneW = m_scene.sceneRect().width();
        if (sceneW > 0) {
            const qreal factor = viewport()->width() / sceneW;
            scale(factor, factor);
        }
        break;
    }
    }
}

QPointF ImageCanvasView::toImagePoint(const QPointF& scenePos) const
{
    return scenePos;
}

} // namespace imageviewer
