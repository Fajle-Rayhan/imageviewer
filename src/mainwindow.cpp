#include "mainwindow.h"

#include "imagecanvasview.h"
#include "settingsdialog.h"
#include "trashservice.h"

#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QProcess>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

namespace imageviewer {

MainWindow::MainWindow(const QString& initialImage, QWidget* parent)
    : QMainWindow(parent), m_settings(), m_trash(new TrashService), m_initialImage(initialImage)
{
    m_settings.load();
    setupUi();
    setupActions();
    applySettings();

    if (!m_initialImage.isEmpty()) {
        if (m_model.loadFor(m_initialImage)) {
            loadImage(m_model.current());
            rebuildThumbnails();
        }
    }

    applyStartupMode();
}

MainWindow::~MainWindow()
{
    delete m_trash;
}

void MainWindow::setupUi()
{
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_canvas = new ImageCanvasView(this);
    connect(m_canvas, &ImageCanvasView::requestNextImage, this, [this]() {
        const QString next = m_model.next();
        if (!next.isEmpty()) {
            loadImage(next);
            setCurrentIndexByPath(next);
        }
    });
    connect(m_canvas, &ImageCanvasView::requestPreviousImage, this, [this]() {
        const QString prev = m_model.previous();
        if (!prev.isEmpty()) {
            loadImage(prev);
            setCurrentIndexByPath(prev);
        }
    });
    connect(m_canvas, &ImageCanvasView::statusMessage, this, &MainWindow::showStatus);

    layout->addWidget(m_canvas, 1);

    m_thumbnails = new QListWidget(this);
    m_thumbnails->setViewMode(QListWidget::IconMode);
    m_thumbnails->setFlow(QListView::LeftToRight);
    m_thumbnails->setResizeMode(QListWidget::Adjust);
    m_thumbnails->setMovement(QListWidget::Static);
    m_thumbnails->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_thumbnails->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_thumbnails->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_thumbnails->setWordWrap(false);
    m_thumbnails->setUniformItemSizes(false);
    m_thumbnails->setMaximumHeight(170);
    connect(m_thumbnails, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        if (!item) {
            return;
        }
        const QString path = item->data(Qt::UserRole).toString();
        if (!path.isEmpty()) {
            loadImage(path);
            setCurrentIndexByPath(path);
        }
    });

    layout->addWidget(m_thumbnails);
    setCentralWidget(central);

    m_overlayLabel = new QLabel(this);
    m_overlayLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_overlayLabel->setStyleSheet(QStringLiteral("QLabel { background: rgba(0,0,0,120); color: #f0f0f0; border-radius: 4px; padding: 2px 6px; }"));
    m_overlayLabel->move(10, 36);
    m_overlayLabel->raise();

    m_toolbar = addToolBar(tr("Main"));
    m_toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    statusBar()->showMessage(tr("Ready"));
}

static QAction* addActionWithFallbackIcon(QToolBar* toolbar, QMenu* menu, const QString& text, const QString& tooltip,
                                           const QIcon& fallbackIcon, const std::function<void()>& fn)
{
    auto* action = new QAction(fallbackIcon, text, toolbar);
    action->setToolTip(tooltip);
    QObject::connect(action, &QAction::triggered, toolbar, fn);
    toolbar->addAction(action);
    menu->addAction(action);
    return action;
}

void MainWindow::setupActions()
{
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));
    QMenu* toolsMenu = menuBar()->addMenu(tr("&Tools"));

    addActionWithFallbackIcon(m_toolbar, fileMenu, tr("Save"), tr("Save image"), style()->standardIcon(QStyle::SP_DialogSaveButton), [this]() { actionSave(); });
    addActionWithFallbackIcon(m_toolbar, fileMenu, tr("Save As"), tr("Save image as"), style()->standardIcon(QStyle::SP_DialogSaveButton), [this]() { actionSaveAs(); });
    addActionWithFallbackIcon(m_toolbar, fileMenu, tr("Delete to Trash"), tr("Move image to Trash"), style()->standardIcon(QStyle::SP_TrashIcon), [this]() { actionDeleteToTrash(); });
    addActionWithFallbackIcon(m_toolbar, fileMenu, tr("Exit"), tr("Exit application"), style()->standardIcon(QStyle::SP_DialogCloseButton), [this]() { close(); });

    addActionWithFallbackIcon(m_toolbar, toolsMenu, tr("Copy"), tr("Copy image to clipboard"), style()->standardIcon(QStyle::SP_FileDialogDetailedView), [this]() { actionCopy(); });
    addActionWithFallbackIcon(m_toolbar, toolsMenu, tr("Mirror"), tr("Mirror image"), style()->standardIcon(QStyle::SP_BrowserReload), [this]() { actionMirror(); });
    addActionWithFallbackIcon(m_toolbar, toolsMenu, tr("Edit with Pencil"), tr("Draw with pencil"), style()->standardIcon(QStyle::SP_DriveFDIcon), [this]() { actionPencil(); });
    addActionWithFallbackIcon(m_toolbar, toolsMenu, tr("Color Marker"), tr("Draw with marker"), style()->standardIcon(QStyle::SP_DriveHDIcon), [this]() { actionMarker(); });
    addActionWithFallbackIcon(m_toolbar, toolsMenu, tr("Text"), tr("Insert styled text"), style()->standardIcon(QStyle::SP_FileDialogContentsView), [this]() { actionText(); });
    addActionWithFallbackIcon(m_toolbar, toolsMenu, tr("Open With"), tr("Open with another application"), style()->standardIcon(QStyle::SP_DirOpenIcon), [this]() { actionOpenWith(); });
    addActionWithFallbackIcon(m_toolbar, toolsMenu, tr("Settings"), tr("Open settings"), style()->standardIcon(QStyle::SP_FileDialogContentsView), [this]() { actionSettings(); });

    QActionGroup* viewGroup = new QActionGroup(this);
    m_viewNormal = viewMenu->addAction(tr("Normal view"));
    m_viewTrue = viewMenu->addAction(tr("True-size view"));
    m_viewScreen = viewMenu->addAction(tr("Screen-wide view"));

    m_viewNormal->setCheckable(true);
    m_viewTrue->setCheckable(true);
    m_viewScreen->setCheckable(true);
    viewGroup->addAction(m_viewNormal);
    viewGroup->addAction(m_viewTrue);
    viewGroup->addAction(m_viewScreen);

    connect(m_viewNormal, &QAction::triggered, this, [this]() {
        m_settings.data().viewMode = ViewMode::Normal;
        applySettings();
    });
    connect(m_viewTrue, &QAction::triggered, this, [this]() {
        m_settings.data().viewMode = ViewMode::TrueSize;
        applySettings();
    });
    connect(m_viewScreen, &QAction::triggered, this, [this]() {
        m_settings.data().viewMode = ViewMode::ScreenWide;
        applySettings();
    });

    m_toggleMenu = new QAction(tr("Toggle Menu"), this);
    connect(m_toggleMenu, &QAction::triggered, this, [this]() {
        menuBar()->setVisible(!menuBar()->isVisible());
    });
    addAction(m_toggleMenu);

    m_toggleThumbs = new QAction(tr("Toggle Thumbnails"), this);
    connect(m_toggleThumbs, &QAction::triggered, this, [this]() {
        m_thumbnails->setVisible(!m_thumbnails->isVisible());
    });
    addAction(m_toggleThumbs);

    auto* undoAction = new QAction(style()->standardIcon(QStyle::SP_ArrowBack), tr("Undo"), this);
    auto* redoAction = new QAction(style()->standardIcon(QStyle::SP_ArrowForward), tr("Redo"), this);
    undoAction->setShortcut(QKeySequence::Undo);
    redoAction->setShortcut(QKeySequence::Redo);
    connect(undoAction, &QAction::triggered, m_canvas, &ImageCanvasView::undo);
    connect(redoAction, &QAction::triggered, m_canvas, &ImageCanvasView::redo);
    toolsMenu->addAction(undoAction);
    toolsMenu->addAction(redoAction);
    addAction(undoAction);
    addAction(redoAction);
}

void MainWindow::applySettings()
{
    const auto& s = m_settings.data();
    m_canvas->setViewMode(s.viewMode);
    m_canvas->setSmoothRendering(s.smoothRendering);
    m_canvas->setCanvasBackground(s.canvasBackground);
    m_canvas->applyEditorDefaults(s.editor);

    menuBar()->setVisible(s.panel.menuVisible);
    m_thumbnails->setVisible(s.panel.thumbnailsVisible);
    m_thumbnails->setIconSize(QSize(s.panel.thumbnailSize, s.panel.thumbnailSize));
    m_thumbnails->setStyleSheet(QStringLiteral("QListWidget::item { border: %1px solid palette(mid); margin: 2px; padding: 2px; } QListWidget::item:selected { border: 2px solid #4FC3F7; }")
                               .arg(s.panel.thumbnailBorder));

    if (s.overlay.visible) {
        m_overlayLabel->setVisible(true);
        m_overlayLabel->setStyleSheet(QStringLiteral("QLabel { background: rgba(0,0,0,120); color: %1; border-radius: 4px; padding: 2px 6px; font-size: %2pt; }")
                                      .arg(s.overlay.textColor.name()).arg(s.overlay.fontSize));
    } else {
        m_overlayLabel->setVisible(false);
    }

    m_viewNormal->setChecked(s.viewMode == ViewMode::Normal);
    m_viewTrue->setChecked(s.viewMode == ViewMode::TrueSize);
    m_viewScreen->setChecked(s.viewMode == ViewMode::ScreenWide);

    m_toggleMenu->setShortcut(s.shortcuts.value(QStringLiteral("toggle_menu"), QKeySequence(QStringLiteral("M"))));
    m_toggleThumbs->setShortcut(s.shortcuts.value(QStringLiteral("toggle_thumbnails"), QKeySequence(QStringLiteral("T"))));

    applyTheme();
    m_settings.save();
}

void MainWindow::applyTheme()
{
    if (m_settings.data().theme == ThemeMode::Light) {
        qApp->setStyleSheet(QString());
        return;
    }

    if (m_settings.data().theme == ThemeMode::System) {
        qApp->setStyleSheet(QString());
        return;
    }

    qApp->setStyleSheet(QStringLiteral(
        "QMainWindow, QWidget { background-color: #202124; color: #f0f0f0; }"
        "QMenuBar, QMenu, QToolBar, QListWidget { background-color: #2b2d30; color: #f0f0f0; }"
        "QToolButton { border: 1px solid #3b3d40; padding: 4px; }"
        "QStatusBar { background-color: #2b2d30; color: #f0f0f0; }"
    ));
}

void MainWindow::applyStartupMode()
{
    switch (m_settings.data().startupMode) {
    case StartupMode::Fullscreen:
        showFullScreen();
        break;
    case StartupMode::Maximized:
        showMaximized();
        break;
    case StartupMode::Normal:
        resize(1280, 820);
        show();
        break;
    }
}

void MainWindow::loadImage(const QString& path)
{
    if (!m_canvas->loadImage(path)) {
        showStatus(tr("Failed to open image: %1").arg(path));
        return;
    }

    QFileInfo info(path);
    setWindowTitle(QStringLiteral("imageviewer - %1").arg(info.fileName()));
    m_overlayLabel->setText(QStringLiteral("%1 · %2x%3")
                            .arg(info.fileName())
                            .arg(m_canvas->scene()->sceneRect().width())
                            .arg(m_canvas->scene()->sceneRect().height()));
    m_overlayLabel->adjustSize();
    m_overlayLabel->move(10, menuBar()->isVisible() ? menuBar()->height() + 8 : 8);
}

void MainWindow::rebuildThumbnails()
{
    m_thumbnails->clear();
    const auto files = m_model.files();
    for (const QString& file : files) {
        QImage img(file);
        if (img.isNull()) {
            continue;
        }

        QPixmap pix = QPixmap::fromImage(img.scaled(m_settings.data().panel.thumbnailSize,
                                                    m_settings.data().panel.thumbnailSize,
                                                    Qt::KeepAspectRatio,
                                                    Qt::SmoothTransformation));
        auto* item = new QListWidgetItem(QIcon(pix), QFileInfo(file).fileName());
        item->setToolTip(file);
        item->setData(Qt::UserRole, file);
        m_thumbnails->addItem(item);
    }
    setCurrentIndexByPath(m_model.current());
}

void MainWindow::setCurrentIndexByPath(const QString& path)
{
    for (int i = 0; i < m_thumbnails->count(); ++i) {
        QListWidgetItem* item = m_thumbnails->item(i);
        if (item->data(Qt::UserRole).toString() == path) {
            m_thumbnails->setCurrentRow(i);
            break;
        }
    }
}

void MainWindow::showStatus(const QString& msg)
{
    statusBar()->showMessage(msg, 2500);
}

void MainWindow::actionCopy()
{
    if (!m_canvas->hasImage()) {
        return;
    }
    auto* clipboard = QApplication::clipboard();
    QImage shot(m_canvas->scene()->sceneRect().size().toSize(), QImage::Format_ARGB32_Premultiplied);
    shot.fill(Qt::transparent);
    QPainter painter(&shot);
    m_canvas->scene()->render(&painter);
    painter.end();
    clipboard->setImage(shot);
    showStatus(tr("Image copied"));
}

void MainWindow::actionMirror()
{
    m_canvas->mirrorCurrent();
    showStatus(tr("Image mirrored"));
}

void MainWindow::actionPencil()
{
    m_canvas->setEditorTool(EditorTool::Pencil);
    showStatus(tr("Pencil tool enabled"));
}

void MainWindow::actionMarker()
{
    m_canvas->setEditorTool(EditorTool::Marker);
    showStatus(tr("Color marker enabled"));
}

void MainWindow::actionText()
{
    m_canvas->setEditorTool(EditorTool::Text);
    showStatus(tr("Text tool enabled"));
}

void MainWindow::actionOpenWith()
{
    const QString current = m_model.current();
    if (current.isEmpty()) {
        return;
    }

    const auto apps = m_openWith.listApplications(current);
    if (apps.isEmpty()) {
        QMessageBox::information(this, tr("Open With"), tr("No compatible application found for this image type."));
        return;
    }

    QStringList names;
    for (const auto& app : apps) {
        names.push_back(app.name);
    }

    bool ok = false;
    const QString selected = QInputDialog::getItem(this, tr("Open With"), tr("Application"), names, 0, false, &ok);
    if (!ok || selected.isEmpty()) {
        return;
    }

    auto it = std::find_if(apps.cbegin(), apps.cend(), [&selected](const OpenWithApp& app) {
        return app.name == selected;
    });

    if (it == apps.cend()) {
        return;
    }

    QString error;
    if (!m_openWith.launch(*it, current, &error)) {
        QMessageBox::warning(this, tr("Open With"), error);
    }
}

void MainWindow::actionDeleteToTrash()
{
    const QString current = m_model.current();
    if (current.isEmpty()) {
        return;
    }

    bool proceed = true;
    if (m_settings.data().deleteConfirm) {
        proceed = QMessageBox::question(this, tr("Delete to Trash"),
                                        tr("Move '%1' to Trash?").arg(QFileInfo(current).fileName())) == QMessageBox::Yes;
    } else {
        showStatus(tr("Delete confirmation disabled; moving to Trash"));
    }

    if (!proceed) {
        return;
    }

    QString error;
    if (!m_trash->moveToTrash(current, &error)) {
        QMessageBox::warning(this, tr("Delete to Trash"), error);
        return;
    }

    const QString next = m_model.removeCurrentOrPath(current, m_settings.data().deleteSelect);
    rebuildThumbnails();
    if (next.isEmpty()) {
        m_canvas->clearEdits();
        showStatus(tr("No images left in directory"));
        return;
    }
    loadImage(next);
}

void MainWindow::actionSave()
{
    const QString current = m_model.current();
    if (current.isEmpty()) {
        return;
    }

    if (m_settings.data().saveOverwriteConfirm) {
        const auto choice = QMessageBox::question(this, tr("Save"), tr("Overwrite current file?"));
        if (choice != QMessageBox::Yes) {
            return;
        }
    }

    if (m_canvas->saveImage(current)) {
        showStatus(tr("Saved %1").arg(QFileInfo(current).fileName()));
        rebuildThumbnails();
    } else {
        QMessageBox::warning(this, tr("Save"), tr("Failed to save file."));
    }
}

void MainWindow::actionSaveAs()
{
    const QString current = m_model.current();
    if (current.isEmpty()) {
        return;
    }

    const QString target = QFileDialog::getSaveFileName(this, tr("Save As"), current);
    if (target.isEmpty()) {
        return;
    }

    if (m_canvas->saveImage(target)) {
        showStatus(tr("Saved %1").arg(QFileInfo(target).fileName()));
        if (m_model.loadFor(target)) {
            rebuildThumbnails();
        }
    } else {
        QMessageBox::warning(this, tr("Save As"), tr("Failed to save file."));
    }
}

void MainWindow::actionSettings()
{
    SettingsDialog dialog(m_settings.data(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    m_settings.data() = dialog.settings();
    applySettings();
}

void MainWindow::mouseMoveEvent(QMouseEvent* event)
{
    QMainWindow::mouseMoveEvent(event);

    if (!m_settings.data().panel.menuEdgeReveal) {
        return;
    }

    if (event->pos().y() <= 6) {
        menuBar()->setVisible(true);
    } else if (!m_settings.data().panel.menuVisible && !menuBar()->geometry().contains(event->pos())) {
        menuBar()->setVisible(false);
    }
}

void MainWindow::leaveEvent(QEvent* event)
{
    QMainWindow::leaveEvent(event);
    if (!m_settings.data().panel.menuVisible && m_settings.data().panel.menuEdgeReveal) {
        menuBar()->setVisible(false);
    }
    if (m_settings.data().panel.thumbnailsAutoHide && !m_settings.data().panel.thumbnailsPinned) {
        m_thumbnails->setVisible(false);
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        if (isFullScreen()) {
            showNormal();
            return;
        }
        close();
        return;
    }

    const auto shortcuts = m_settings.data().shortcuts;
    if (event->matches(QKeySequence::Undo)) {
        m_canvas->undo();
        return;
    }
    if (event->matches(QKeySequence::Redo)) {
        m_canvas->redo();
        return;
    }

    if (QKeySequence(event->modifiers() | event->key()) == shortcuts.value(QStringLiteral("next"))) {
        const QString path = m_model.next();
        if (!path.isEmpty()) {
            loadImage(path);
            setCurrentIndexByPath(path);
        }
        return;
    }
    if (QKeySequence(event->modifiers() | event->key()) == shortcuts.value(QStringLiteral("previous"))) {
        const QString path = m_model.previous();
        if (!path.isEmpty()) {
            loadImage(path);
            setCurrentIndexByPath(path);
        }
        return;
    }

    QMainWindow::keyPressEvent(event);
}

} // namespace imageviewer
