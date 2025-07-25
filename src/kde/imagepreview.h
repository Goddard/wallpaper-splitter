#pragma once

#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QVector>
#include <QCheckBox>
#include "core/monitor_info.h"

class MonitorOverlay;

class ImagePreview : public QWidget {
    Q_OBJECT

public:
    explicit ImagePreview(QWidget* parent = nullptr);

public slots:
    void setImage(const QString& imagePath);
    void setMonitors(const WallpaperCore::MonitorList& monitors, const QVector<bool>& enabledStates = QVector<bool>());
    void updateMonitorOverlays();

signals:
    void monitorToggled(int monitorIndex, bool enabled);

private:
    void resizeEvent(QResizeEvent* event) override;
    void updateOverlayPositions();
    
    QLabel* m_imageLabel;
    QPixmap m_pixmap;
    WallpaperCore::MonitorList m_monitors;
    QVector<MonitorOverlay*> m_overlays;
    QWidget* m_overlayContainer;
}; 