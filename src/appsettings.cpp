#include "appsettings.h"

#include <QDir>
#include <QStandardPaths>

namespace imageviewer {

static QString settingsPath(const QString& explicitPath)
{
    if (!explicitPath.isEmpty()) {
        return explicitPath;
    }
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(base);
    return base + QLatin1String("/config.ini");
}

AppSettings::AppSettings(const QString& organization, const QString& application, const QString& explicitPath)
    : m_explicitPath(explicitPath)
{
    Q_UNUSED(organization);
    Q_UNUSED(application);
    m_data.shortcuts = defaultShortcuts();
}

QSettings AppSettings::createSettings() const
{
    return QSettings(settingsPath(m_explicitPath), QSettings::IniFormat);
}

QMap<QString, QKeySequence> AppSettings::defaultShortcuts()
{
    return {
        {QStringLiteral("quit"), QKeySequence(QStringLiteral("Q"))},
        {QStringLiteral("escape"), QKeySequence(QStringLiteral("Esc"))},
        {QStringLiteral("toggle_menu"), QKeySequence(QStringLiteral("M"))},
        {QStringLiteral("toggle_thumbnails"), QKeySequence(QStringLiteral("T"))},
        {QStringLiteral("open_settings"), QKeySequence(QStringLiteral("Ctrl+,"))},
        {QStringLiteral("delete"), QKeySequence(QStringLiteral("Delete"))},
        {QStringLiteral("next"), QKeySequence(QStringLiteral("Right"))},
        {QStringLiteral("previous"), QKeySequence(QStringLiteral("Left"))},
        {QStringLiteral("view_normal"), QKeySequence(QStringLiteral("1"))},
        {QStringLiteral("view_true_size"), QKeySequence(QStringLiteral("0"))},
        {QStringLiteral("view_screen_wide"), QKeySequence(QStringLiteral("2"))},
        {QStringLiteral("tool_pencil"), QKeySequence(QStringLiteral("P"))},
        {QStringLiteral("tool_marker"), QKeySequence(QStringLiteral("H"))},
        {QStringLiteral("tool_text"), QKeySequence(QStringLiteral("X"))},
    };
}

void AppSettings::load()
{
    QSettings s = createSettings();
    m_data.startupMode = startupModeFromString(s.value(QStringLiteral("general/startup_mode"), QStringLiteral("fullscreen")).toString());
    m_data.viewMode = viewModeFromString(s.value(QStringLiteral("general/view_mode"), QStringLiteral("true_size")).toString());
    m_data.theme = themeModeFromString(s.value(QStringLiteral("general/theme"), QStringLiteral("dark")).toString());
    m_data.canvasBackground = s.value(QStringLiteral("general/canvas_background"), QStringLiteral("#303030")).value<QColor>();
    m_data.smoothRendering = s.value(QStringLiteral("general/smooth_rendering"), true).toBool();
    m_data.deleteConfirm = s.value(QStringLiteral("general/delete_confirm"), true).toBool();
    m_data.deleteSelect = s.value(QStringLiteral("general/delete_select"), QStringLiteral("next")).toString();
    m_data.saveOverwriteConfirm = s.value(QStringLiteral("general/save_overwrite_confirm"), true).toBool();
    m_data.titleOverlayVisible = s.value(QStringLiteral("general/title_overlay_visible"), true).toBool();
    m_data.performanceMode = performanceModeFromString(s.value(QStringLiteral("general/performance"), QStringLiteral("auto")).toString());

    m_data.panel.thumbnailsVisible = s.value(QStringLiteral("panel/thumbnails_visible"), true).toBool();
    m_data.panel.thumbnailsPinned = s.value(QStringLiteral("panel/thumbnails_pinned"), false).toBool();
    m_data.panel.thumbnailsAutoHide = s.value(QStringLiteral("panel/thumbnails_auto_hide"), false).toBool();
    m_data.panel.thumbnailSize = s.value(QStringLiteral("panel/thumbnail_size"), 96).toInt();
    m_data.panel.thumbnailBorder = s.value(QStringLiteral("panel/thumbnail_border"), 1).toInt();
    m_data.panel.menuVisible = s.value(QStringLiteral("panel/menu_visible"), true).toBool();
    m_data.panel.menuEdgeReveal = s.value(QStringLiteral("panel/menu_edge_reveal"), true).toBool();

    m_data.overlay.visible = s.value(QStringLiteral("overlay/visible"), true).toBool();
    m_data.overlay.fontSize = s.value(QStringLiteral("overlay/font_size"), 8).toInt();
    m_data.overlay.textColor = s.value(QStringLiteral("overlay/text_color"), QStringLiteral("#F0F0F0")).value<QColor>();

    m_data.editor.pencilColor = s.value(QStringLiteral("editor/pencil_color"), QStringLiteral("#ff9800")).value<QColor>();
    m_data.editor.pencilSize = s.value(QStringLiteral("editor/pencil_size"), 3).toInt();
    m_data.editor.pencilOpacity = s.value(QStringLiteral("editor/pencil_opacity"), 1.0).toDouble();
    m_data.editor.markerColor = s.value(QStringLiteral("editor/marker_color"), QStringLiteral("#ffeb3b")).value<QColor>();
    m_data.editor.markerSize = s.value(QStringLiteral("editor/marker_size"), 20).toInt();
    m_data.editor.markerOpacity = s.value(QStringLiteral("editor/marker_opacity"), 0.35).toDouble();
    m_data.editor.textColor = s.value(QStringLiteral("editor/text_color"), QStringLiteral("#ffffff")).value<QColor>();
    m_data.editor.textSize = s.value(QStringLiteral("editor/text_size"), 18).toInt();
    m_data.editor.textBold = s.value(QStringLiteral("editor/text_bold"), false).toBool();
    m_data.editor.textItalic = s.value(QStringLiteral("editor/text_italic"), false).toBool();
    m_data.editor.textUnderline = s.value(QStringLiteral("editor/text_underline"), false).toBool();

    m_data.shortcuts = defaultShortcuts();
    s.beginGroup(QStringLiteral("shortcuts"));
    const auto keys = s.childKeys();
    for (const QString& key : keys) {
        const QString raw = s.value(key).toString();
        if (!raw.isEmpty()) {
            m_data.shortcuts[key] = QKeySequence(raw);
        }
    }
    s.endGroup();
}

void AppSettings::save() const
{
    QSettings s = createSettings();
    s.setValue(QStringLiteral("general/startup_mode"), startupModeToString(m_data.startupMode));
    s.setValue(QStringLiteral("general/view_mode"), viewModeToString(m_data.viewMode));
    s.setValue(QStringLiteral("general/theme"), themeModeToString(m_data.theme));
    s.setValue(QStringLiteral("general/canvas_background"), m_data.canvasBackground);
    s.setValue(QStringLiteral("general/smooth_rendering"), m_data.smoothRendering);
    s.setValue(QStringLiteral("general/delete_confirm"), m_data.deleteConfirm);
    s.setValue(QStringLiteral("general/delete_select"), m_data.deleteSelect);
    s.setValue(QStringLiteral("general/save_overwrite_confirm"), m_data.saveOverwriteConfirm);
    s.setValue(QStringLiteral("general/title_overlay_visible"), m_data.titleOverlayVisible);
    s.setValue(QStringLiteral("general/performance"), performanceModeToString(m_data.performanceMode));

    s.setValue(QStringLiteral("panel/thumbnails_visible"), m_data.panel.thumbnailsVisible);
    s.setValue(QStringLiteral("panel/thumbnails_pinned"), m_data.panel.thumbnailsPinned);
    s.setValue(QStringLiteral("panel/thumbnails_auto_hide"), m_data.panel.thumbnailsAutoHide);
    s.setValue(QStringLiteral("panel/thumbnail_size"), m_data.panel.thumbnailSize);
    s.setValue(QStringLiteral("panel/thumbnail_border"), m_data.panel.thumbnailBorder);
    s.setValue(QStringLiteral("panel/menu_visible"), m_data.panel.menuVisible);
    s.setValue(QStringLiteral("panel/menu_edge_reveal"), m_data.panel.menuEdgeReveal);

    s.setValue(QStringLiteral("overlay/visible"), m_data.overlay.visible);
    s.setValue(QStringLiteral("overlay/font_size"), m_data.overlay.fontSize);
    s.setValue(QStringLiteral("overlay/text_color"), m_data.overlay.textColor);

    s.setValue(QStringLiteral("editor/pencil_color"), m_data.editor.pencilColor);
    s.setValue(QStringLiteral("editor/pencil_size"), m_data.editor.pencilSize);
    s.setValue(QStringLiteral("editor/pencil_opacity"), m_data.editor.pencilOpacity);
    s.setValue(QStringLiteral("editor/marker_color"), m_data.editor.markerColor);
    s.setValue(QStringLiteral("editor/marker_size"), m_data.editor.markerSize);
    s.setValue(QStringLiteral("editor/marker_opacity"), m_data.editor.markerOpacity);
    s.setValue(QStringLiteral("editor/text_color"), m_data.editor.textColor);
    s.setValue(QStringLiteral("editor/text_size"), m_data.editor.textSize);
    s.setValue(QStringLiteral("editor/text_bold"), m_data.editor.textBold);
    s.setValue(QStringLiteral("editor/text_italic"), m_data.editor.textItalic);
    s.setValue(QStringLiteral("editor/text_underline"), m_data.editor.textUnderline);

    s.beginGroup(QStringLiteral("shortcuts"));
    s.remove(QString());
    for (auto it = m_data.shortcuts.cbegin(); it != m_data.shortcuts.cend(); ++it) {
        s.setValue(it.key(), it.value().toString(QKeySequence::PortableText));
    }
    s.endGroup();
    s.sync();
}

QString AppSettings::startupModeToString(StartupMode mode)
{
    switch (mode) {
    case StartupMode::Fullscreen: return QStringLiteral("fullscreen");
    case StartupMode::Normal: return QStringLiteral("normal");
    case StartupMode::Maximized: return QStringLiteral("maximized");
    }
    return QStringLiteral("fullscreen");
}

StartupMode AppSettings::startupModeFromString(const QString& value)
{
    if (value == QStringLiteral("normal")) return StartupMode::Normal;
    if (value == QStringLiteral("maximized")) return StartupMode::Maximized;
    return StartupMode::Fullscreen;
}

QString AppSettings::viewModeToString(ViewMode mode)
{
    switch (mode) {
    case ViewMode::Normal: return QStringLiteral("normal");
    case ViewMode::TrueSize: return QStringLiteral("true_size");
    case ViewMode::ScreenWide: return QStringLiteral("screen_wide");
    }
    return QStringLiteral("true_size");
}

ViewMode AppSettings::viewModeFromString(const QString& value)
{
    if (value == QStringLiteral("normal")) return ViewMode::Normal;
    if (value == QStringLiteral("screen_wide")) return ViewMode::ScreenWide;
    return ViewMode::TrueSize;
}

QString AppSettings::themeModeToString(ThemeMode mode)
{
    switch (mode) {
    case ThemeMode::Dark: return QStringLiteral("dark");
    case ThemeMode::Light: return QStringLiteral("light");
    case ThemeMode::System: return QStringLiteral("system");
    }
    return QStringLiteral("dark");
}

ThemeMode AppSettings::themeModeFromString(const QString& value)
{
    if (value == QStringLiteral("light")) return ThemeMode::Light;
    if (value == QStringLiteral("system")) return ThemeMode::System;
    return ThemeMode::Dark;
}

QString AppSettings::performanceModeToString(PerformanceMode mode)
{
    switch (mode) {
    case PerformanceMode::Auto: return QStringLiteral("auto");
    case PerformanceMode::Cpu: return QStringLiteral("cpu");
    case PerformanceMode::Gpu: return QStringLiteral("gpu");
    }
    return QStringLiteral("auto");
}

PerformanceMode AppSettings::performanceModeFromString(const QString& value)
{
    if (value == QStringLiteral("cpu")) return PerformanceMode::Cpu;
    if (value == QStringLiteral("gpu")) return PerformanceMode::Gpu;
    return PerformanceMode::Auto;
}

} // namespace imageviewer
