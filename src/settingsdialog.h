#pragma once

#include "appsettings.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QKeySequenceEdit;
class QSpinBox;

namespace imageviewer {

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(AppSettingsData data, QWidget* parent = nullptr);
    AppSettingsData settings() const;

private:
    AppSettingsData m_data;

    QComboBox* m_startupMode = nullptr;
    QComboBox* m_viewMode = nullptr;
    QComboBox* m_themeMode = nullptr;
    QComboBox* m_perfMode = nullptr;
    QCheckBox* m_deleteConfirm = nullptr;
    QCheckBox* m_saveConfirm = nullptr;
    QCheckBox* m_smoothRendering = nullptr;
    QCheckBox* m_thumbsVisible = nullptr;
    QCheckBox* m_thumbsPinned = nullptr;
    QCheckBox* m_thumbsAutoHide = nullptr;
    QSpinBox* m_thumbSize = nullptr;
    QSpinBox* m_thumbBorder = nullptr;
    QCheckBox* m_menuVisible = nullptr;
    QCheckBox* m_menuEdgeReveal = nullptr;
    QCheckBox* m_overlayVisible = nullptr;
    QSpinBox* m_overlayFontSize = nullptr;
    QSpinBox* m_pencilSize = nullptr;
    QSpinBox* m_markerSize = nullptr;
    QSpinBox* m_textSize = nullptr;
    QCheckBox* m_textBold = nullptr;
    QCheckBox* m_textItalic = nullptr;
    QCheckBox* m_textUnderline = nullptr;

    QMap<QString, QKeySequenceEdit*> m_shortcuts;
};

} // namespace imageviewer
