#pragma once

#include <QWidget>
#include <QCheckBox>
#include <QLabel>
#include <QPainter>
#include "core/monitor_info.h"

class MonitorOverlay : public QWidget {
    Q_OBJECT

public:
    explicit MonitorOverlay(const WallpaperCore::MonitorInfo& monitor, int index, bool enabled = true, QWidget* parent = nullptr);

    void setGeometry(const QRect& geometry);
    bool isEnabled() const;
    int getIndex() const { return m_index; }
    void setSingleMonitorMode(bool singleMode);

signals:
    void toggled(int index, bool enabled);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    WallpaperCore::MonitorInfo m_monitor;
    int m_index;
    QCheckBox* m_checkBox;
    QLabel* m_infoLabel;
    bool m_hovered;
    
    void updateStyle();
}; 