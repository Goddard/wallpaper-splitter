#include "core/monitor_detector.h"
#include <QDebug>
#include <QDir>
#include <QGuiApplication>
#include <QScreen>

namespace WallpaperCore {

MonitorDetector::MonitorDetector(QObject* parent)
    : QObject(parent)
{
}

MonitorInfo MonitorDetector::getPrimaryMonitor()
{
    if (m_monitors.empty()) {
        detectMonitors();
    }
    
    for (const auto& monitor : m_monitors) {
        if (monitor.isPrimary) {
            return monitor;
        }
    }
    
    return m_monitors.empty() ? MonitorInfo() : m_monitors[0];
}

void MonitorDetector::refreshMonitors()
{
    m_monitors = detectMonitors();
    emit monitorsChanged();
}

MonitorList MonitorDetector::detectMonitors()
{
    MonitorList monitors;
    
    QGuiApplication* app = qobject_cast<QGuiApplication*>(QCoreApplication::instance());
    if (!app) {
        qWarning() << "No QGuiApplication instance found";
        return monitors;
    }
    
    QList<QScreen*> screens = app->screens();
    
    for (int i = 0; i < screens.size(); ++i) {
        QScreen* screen = screens[i];
        
        MonitorInfo monitor;
        monitor.name = screen->name();
        
        // Get the physical size and available geometry
        QSize physicalSize = screen->size();
        QRect geometry = screen->geometry();
        QRect availableGeometry = screen->availableGeometry();
        qreal devicePixelRatio = screen->devicePixelRatio();
        
        // Calculate actual resolution accounting for device pixel ratio
        QSize actualSize = physicalSize * devicePixelRatio;
        
        // Use actual size for the geometry
        monitor.geometry = QRect(geometry.x(), geometry.y(), 
                               actualSize.width(), actualSize.height());
        
        monitor.isPrimary = (i == 0) || screen->geometry().x() == 0; // First screen or leftmost is primary
        
        monitors.push_back(monitor);
        
        qDebug() << "Detected monitor:" << monitor.name 
                 << "at" << monitor.geometry 
                 << "primary:" << monitor.isPrimary
                 << "physical size:" << physicalSize
                 << "actual size:" << actualSize
                 << "geometry:" << geometry
                 << "available:" << availableGeometry
                 << "device pixel ratio:" << devicePixelRatio;
    }
    
    m_monitors = monitors;
    return monitors;
}

} // namespace WallpaperCore 