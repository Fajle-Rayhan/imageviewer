#include "settingsdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QKeySequenceEdit>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

namespace imageviewer {

SettingsDialog::SettingsDialog(AppSettingsData data, QWidget* parent)
    : QDialog(parent), m_data(std::move(data))
{
    setWindowTitle(tr("Settings"));
    resize(720, 560);

    auto* root = new QVBoxLayout(this);
    auto* tabs = new QTabWidget(this);
    root->addWidget(tabs);

    auto* general = new QWidget(this);
    auto* generalForm = new QFormLayout(general);

    m_startupMode = new QComboBox(this);
    m_startupMode->addItem(tr("Fullscreen"), static_cast<int>(StartupMode::Fullscreen));
    m_startupMode->addItem(tr("Normal"), static_cast<int>(StartupMode::Normal));
    m_startupMode->addItem(tr("Maximized"), static_cast<int>(StartupMode::Maximized));
    m_startupMode->setCurrentIndex(m_startupMode->findData(static_cast<int>(m_data.startupMode)));

    m_viewMode = new QComboBox(this);
    m_viewMode->addItem(tr("Normal"), static_cast<int>(ViewMode::Normal));
    m_viewMode->addItem(tr("True size"), static_cast<int>(ViewMode::TrueSize));
    m_viewMode->addItem(tr("Screen wide"), static_cast<int>(ViewMode::ScreenWide));
    m_viewMode->setCurrentIndex(m_viewMode->findData(static_cast<int>(m_data.viewMode)));

    m_themeMode = new QComboBox(this);
    m_themeMode->addItem(tr("Dark"), static_cast<int>(ThemeMode::Dark));
    m_themeMode->addItem(tr("Light"), static_cast<int>(ThemeMode::Light));
    m_themeMode->addItem(tr("System"), static_cast<int>(ThemeMode::System));
    m_themeMode->setCurrentIndex(m_themeMode->findData(static_cast<int>(m_data.theme)));

    m_perfMode = new QComboBox(this);
    m_perfMode->addItem(tr("Auto"), static_cast<int>(PerformanceMode::Auto));
    m_perfMode->addItem(tr("CPU"), static_cast<int>(PerformanceMode::Cpu));
    m_perfMode->addItem(tr("GPU"), static_cast<int>(PerformanceMode::Gpu));
    m_perfMode->setCurrentIndex(m_perfMode->findData(static_cast<int>(m_data.performanceMode)));

    m_deleteConfirm = new QCheckBox(tr("Confirm before delete"), this);
    m_deleteConfirm->setChecked(m_data.deleteConfirm);
    m_saveConfirm = new QCheckBox(tr("Confirm overwrite on save"), this);
    m_saveConfirm->setChecked(m_data.saveOverwriteConfirm);
    m_smoothRendering = new QCheckBox(tr("Smooth rendering"), this);
    m_smoothRendering->setChecked(m_data.smoothRendering);

    generalForm->addRow(tr("Startup mode"), m_startupMode);
    generalForm->addRow(tr("View mode"), m_viewMode);
    generalForm->addRow(tr("Theme"), m_themeMode);
    generalForm->addRow(tr("Performance"), m_perfMode);
    generalForm->addRow(m_deleteConfirm);
    generalForm->addRow(m_saveConfirm);
    generalForm->addRow(m_smoothRendering);
    tabs->addTab(general, tr("General"));

    auto* panels = new QWidget(this);
    auto* panelForm = new QFormLayout(panels);
    m_thumbsVisible = new QCheckBox(tr("Show thumbnail panel"), this);
    m_thumbsVisible->setChecked(m_data.panel.thumbnailsVisible);
    m_thumbsPinned = new QCheckBox(tr("Pinned thumbnails"), this);
    m_thumbsPinned->setChecked(m_data.panel.thumbnailsPinned);
    m_thumbsAutoHide = new QCheckBox(tr("Auto-hide thumbnails"), this);
    m_thumbsAutoHide->setChecked(m_data.panel.thumbnailsAutoHide);
    m_thumbSize = new QSpinBox(this);
    m_thumbSize->setRange(48, 512);
    m_thumbSize->setValue(m_data.panel.thumbnailSize);
    m_thumbBorder = new QSpinBox(this);
    m_thumbBorder->setRange(0, 8);
    m_thumbBorder->setValue(m_data.panel.thumbnailBorder);
    m_menuVisible = new QCheckBox(tr("Show menu bar"), this);
    m_menuVisible->setChecked(m_data.panel.menuVisible);
    m_menuEdgeReveal = new QCheckBox(tr("Menu edge reveal"), this);
    m_menuEdgeReveal->setChecked(m_data.panel.menuEdgeReveal);

    panelForm->addRow(m_thumbsVisible);
    panelForm->addRow(m_thumbsPinned);
    panelForm->addRow(m_thumbsAutoHide);
    panelForm->addRow(tr("Thumbnail size"), m_thumbSize);
    panelForm->addRow(tr("Thumbnail border"), m_thumbBorder);
    panelForm->addRow(m_menuVisible);
    panelForm->addRow(m_menuEdgeReveal);
    tabs->addTab(panels, tr("Panels"));

    auto* editor = new QWidget(this);
    auto* editorForm = new QFormLayout(editor);
    m_pencilSize = new QSpinBox(this);
    m_pencilSize->setRange(1, 64);
    m_pencilSize->setValue(m_data.editor.pencilSize);
    m_markerSize = new QSpinBox(this);
    m_markerSize->setRange(1, 128);
    m_markerSize->setValue(m_data.editor.markerSize);
    m_textSize = new QSpinBox(this);
    m_textSize->setRange(6, 120);
    m_textSize->setValue(m_data.editor.textSize);
    m_textBold = new QCheckBox(tr("Text bold"), this);
    m_textBold->setChecked(m_data.editor.textBold);
    m_textItalic = new QCheckBox(tr("Text italic"), this);
    m_textItalic->setChecked(m_data.editor.textItalic);
    m_textUnderline = new QCheckBox(tr("Text underline"), this);
    m_textUnderline->setChecked(m_data.editor.textUnderline);
    m_overlayVisible = new QCheckBox(tr("Show title/info overlay"), this);
    m_overlayVisible->setChecked(m_data.overlay.visible);
    m_overlayFontSize = new QSpinBox(this);
    m_overlayFontSize->setRange(6, 24);
    m_overlayFontSize->setValue(m_data.overlay.fontSize);

    editorForm->addRow(tr("Pencil size"), m_pencilSize);
    editorForm->addRow(tr("Marker size"), m_markerSize);
    editorForm->addRow(tr("Text size"), m_textSize);
    editorForm->addRow(m_textBold);
    editorForm->addRow(m_textItalic);
    editorForm->addRow(m_textUnderline);
    editorForm->addRow(m_overlayVisible);
    editorForm->addRow(tr("Overlay font size"), m_overlayFontSize);
    tabs->addTab(editor, tr("Editor/Overlay"));

    auto* shortcuts = new QWidget(this);
    auto* shortcutForm = new QFormLayout(shortcuts);
    const QStringList shortcutKeys = {
        QStringLiteral("toggle_menu"), QStringLiteral("toggle_thumbnails"), QStringLiteral("open_settings"),
        QStringLiteral("delete"), QStringLiteral("quit"), QStringLiteral("next"), QStringLiteral("previous"),
        QStringLiteral("view_normal"), QStringLiteral("view_true_size"), QStringLiteral("view_screen_wide"),
        QStringLiteral("tool_pencil"), QStringLiteral("tool_marker"), QStringLiteral("tool_text")
    };

    for (const QString& key : shortcutKeys) {
        auto* edit = new QKeySequenceEdit(m_data.shortcuts.value(key), this);
        m_shortcuts.insert(key, edit);
        shortcutForm->addRow(key, edit);
    }
    tabs->addTab(shortcuts, tr("Shortcuts"));

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);
}

AppSettingsData SettingsDialog::settings() const
{
    AppSettingsData s = m_data;
    s.startupMode = static_cast<StartupMode>(m_startupMode->currentData().toInt());
    s.viewMode = static_cast<ViewMode>(m_viewMode->currentData().toInt());
    s.theme = static_cast<ThemeMode>(m_themeMode->currentData().toInt());
    s.performanceMode = static_cast<PerformanceMode>(m_perfMode->currentData().toInt());
    s.deleteConfirm = m_deleteConfirm->isChecked();
    s.saveOverwriteConfirm = m_saveConfirm->isChecked();
    s.smoothRendering = m_smoothRendering->isChecked();

    s.panel.thumbnailsVisible = m_thumbsVisible->isChecked();
    s.panel.thumbnailsPinned = m_thumbsPinned->isChecked();
    s.panel.thumbnailsAutoHide = m_thumbsAutoHide->isChecked();
    s.panel.thumbnailSize = m_thumbSize->value();
    s.panel.thumbnailBorder = m_thumbBorder->value();
    s.panel.menuVisible = m_menuVisible->isChecked();
    s.panel.menuEdgeReveal = m_menuEdgeReveal->isChecked();

    s.editor.pencilSize = m_pencilSize->value();
    s.editor.markerSize = m_markerSize->value();
    s.editor.textSize = m_textSize->value();
    s.editor.textBold = m_textBold->isChecked();
    s.editor.textItalic = m_textItalic->isChecked();
    s.editor.textUnderline = m_textUnderline->isChecked();

    s.overlay.visible = m_overlayVisible->isChecked();
    s.overlay.fontSize = m_overlayFontSize->value();

    for (auto it = m_shortcuts.cbegin(); it != m_shortcuts.cend(); ++it) {
        s.shortcuts[it.key()] = it.value()->keySequence();
    }

    return s;
}

} // namespace imageviewer
