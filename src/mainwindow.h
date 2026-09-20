#pragma once

#include "appsettings.h"
#include "imagedirectorymodel.h"
#include "openwithservice.h"

#include <QMainWindow>

class QAction;
class QLabel;
class QListWidget;
class QMenu;
class QToolBar;

namespace imageviewer {

class ImageCanvasView;
class TrashService;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(const QString& initialImage = QString(), QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setupUi();
    void setupActions();
    void applySettings();
    void applyTheme();
    void applyStartupMode();
    void loadImage(const QString& path);
    void rebuildThumbnails();
    void setCurrentIndexByPath(const QString& path);
    void showStatus(const QString& msg);

    void actionCopy();
    void actionMirror();
    void actionPencil();
    void actionMarker();
    void actionText();
    void actionOpenWith();
    void actionDeleteToTrash();
    void actionSave();
    void actionSaveAs();
    void actionSettings();

    AppSettings m_settings;
    ImageDirectoryModel m_model;
    OpenWithService m_openWith;
    TrashService* m_trash = nullptr;

    ImageCanvasView* m_canvas = nullptr;
    QListWidget* m_thumbnails = nullptr;
    QLabel* m_overlayLabel = nullptr;

    QAction* m_viewNormal = nullptr;
    QAction* m_viewTrue = nullptr;
    QAction* m_viewScreen = nullptr;

    QAction* m_toggleMenu = nullptr;
    QAction* m_toggleThumbs = nullptr;

    QToolBar* m_toolbar = nullptr;
    QString m_initialImage;
};

} // namespace imageviewer
