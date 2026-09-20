#pragma once

#include "appsettings.h"
#include "editorcommands.h"

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>

namespace imageviewer {

class ImageCanvasView : public QGraphicsView {
    Q_OBJECT

public:
    explicit ImageCanvasView(QWidget* parent = nullptr);

    bool loadImage(const QString& filePath);
    bool saveImage(const QString& filePath);
    void setViewMode(ViewMode mode);
    ViewMode viewMode() const { return m_viewMode; }
    void setSmoothRendering(bool enabled);
    void setCanvasBackground(const QColor& color);

    void setEditorTool(EditorTool tool);
    EditorTool editorTool() const { return m_editorTool; }
    void applyEditorDefaults(const EditorDefaults& defaults);

    bool hasImage() const;
    QString currentImagePath() const { return m_currentPath; }

    void undo();
    void redo();
    void clearEdits();
    bool hasPendingEdits() const;
    void mirrorCurrent();

signals:
    void requestNextImage();
    void requestPreviousImage();
    void statusMessage(const QString& message);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void rebuildOverlay();
    void applyViewMode();
    QPointF toImagePoint(const QPointF& scenePos) const;

    QGraphicsScene m_scene;
    QGraphicsPixmapItem* m_baseItem = nullptr;
    QGraphicsPixmapItem* m_overlayItem = nullptr;

    QImage m_baseImage;
    QImage m_overlayImage;
    QString m_currentPath;

    ViewMode m_viewMode = ViewMode::TrueSize;
    EditorTool m_editorTool = EditorTool::Pencil;

    QColor m_pencilColor = QColor("#ff9800");
    int m_pencilSize = 3;
    qreal m_pencilOpacity = 1.0;

    QColor m_markerColor = QColor("#ffeb3b");
    int m_markerSize = 20;
    qreal m_markerOpacity = 0.35;

    QColor m_textColor = QColor("#ffffff");
    int m_textSize = 18;
    bool m_textBold = false;
    bool m_textItalic = false;
    bool m_textUnderline = false;

    bool m_drawing = false;
    StrokeCommand m_currentStroke;

    EditorCommandStack m_stack;
};

} // namespace imageviewer
