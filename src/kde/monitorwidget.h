#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include "core/monitor_info.h"

class MonitorWidget : public QWidget {
    Q_OBJECT

public:
    explicit MonitorWidget(const WallpaperCore::MonitorInfo& monitor, QWidget* parent = nullptr);

    WallpaperCore::MonitorInfo getMonitorInfo() const;
    bool isSelected() const;

private:
    WallpaperCore::MonitorInfo m_monitor;
    QCheckBox* m_enabledCheckBox;
    QLabel* m_nameLabel;
    QLabel* m_resolutionLabel;
    QLabel* m_positionLabel;
}; 