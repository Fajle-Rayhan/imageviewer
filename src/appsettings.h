#pragma once

#include <QColor>
#include <QKeySequence>
#include <QMap>
#include <QSettings>
#include <QString>

namespace imageviewer {

enum class StartupMode { Fullscreen, Normal, Maximized };
enum class ViewMode { Normal, TrueSize, ScreenWide };
enum class ThemeMode { Dark, Light, System };
enum class PerformanceMode { Auto, Cpu, Gpu };
enum class EditorTool { Pencil, Marker, Text };

struct OverlaySettings {
    bool visible = true;
    int fontSize = 8;
    QColor textColor = QColor("#F0F0F0");
};

struct PanelSettings {
    bool thumbnailsVisible = true;
    bool thumbnailsPinned = false;
    bool thumbnailsAutoHide = false;
    int thumbnailSize = 96;
    int thumbnailBorder = 1;
    bool menuVisible = true;
    bool menuEdgeReveal = true;
};

struct EditorDefaults {
    QColor pencilColor = QColor("#ff9800");
    int pencilSize = 3;
    qreal pencilOpacity = 1.0;

    QColor markerColor = QColor("#ffeb3b");
    int markerSize = 20;
    qreal markerOpacity = 0.35;

    QColor textColor = QColor("#ffffff");
    int textSize = 18;
    bool textBold = false;
    bool textItalic = false;
    bool textUnderline = false;
};

struct AppSettingsData {
    StartupMode startupMode = StartupMode::Fullscreen;
    ViewMode viewMode = ViewMode::TrueSize;
    ThemeMode theme = ThemeMode::Dark;
    QColor canvasBackground = QColor("#303030");
    bool smoothRendering = true;
    bool deleteConfirm = true;
    QString deleteSelect = QStringLiteral("next");
    bool saveOverwriteConfirm = true;
    bool titleOverlayVisible = true;
    PerformanceMode performanceMode = PerformanceMode::Auto;
    PanelSettings panel;
    OverlaySettings overlay;
    EditorDefaults editor;
    QMap<QString, QKeySequence> shortcuts;
};

class AppSettings {
public:
    explicit AppSettings(const QString& organization = QStringLiteral("imageviewer"),
                         const QString& application = QStringLiteral("imageviewer"),
                         const QString& explicitPath = QString());

    void load();
    void save() const;

    AppSettingsData& data() { return m_data; }
    const AppSettingsData& data() const { return m_data; }

    static QMap<QString, QKeySequence> defaultShortcuts();

    static QString startupModeToString(StartupMode mode);
    static StartupMode startupModeFromString(const QString& value);

    static QString viewModeToString(ViewMode mode);
    static ViewMode viewModeFromString(const QString& value);

    static QString themeModeToString(ThemeMode mode);
    static ThemeMode themeModeFromString(const QString& value);

    static QString performanceModeToString(PerformanceMode mode);
    static PerformanceMode performanceModeFromString(const QString& value);

private:
    QString m_explicitPath;
    AppSettingsData m_data;

    QSettings createSettings() const;
};

} // namespace imageviewer
