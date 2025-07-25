#include "core/monitor_detector.h"
#include <QDebug>
#include <QDir>
#include <QGuiApplication>
#include <QScreen>
#include <QProcess>

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
    
    // Use KDE's native monitor detection via dbus
    QProcess process;
    process.start("qdbus", QStringList() << "org.kde.plasmashell" << "/PlasmaShell" 
                 << "org.kde.PlasmaShell.evaluateScript" 
                 << "JSON.stringify(desktops().filter(d => d.screen != -1).map(d => ({screen: d.screen, geom: screenGeometry(d.screen)})))");
    
    if (process.waitForFinished(5000) && process.exitCode() == 0) {
        QString output = process.readAllStandardOutput();
        // Parse and use KDE's monitor info
        qDebug() << "KDE monitors:" << output;
    }
    
    // Fallback to Qt detection
    QGuiApplication* app = qobject_cast<QGuiApplication*>(QCoreApplication::instance());
    if (!app) {
        return monitors;
    }
    
    QList<QScreen*> screens = app->screens();
    
    for (int i = 0; i < screens.size(); ++i) {
        QScreen* screen = screens[i];
        
        MonitorInfo monitor;
        monitor.name = screen->name();
        
        QRect geometry = screen->geometry();
        QSize physicalSize = screen->size();
        qreal devicePixelRatio = screen->devicePixelRatio();
        QSize actualSize = physicalSize * devicePixelRatio;
        
        monitor.geometry = geometry;
        monitor.actualResolution = actualSize;
        monitor.isPrimary = (i == 0) || geometry.x() == 0;
        
        monitors.push_back(monitor);
        
        qDebug() << "Monitor:" << monitor.name << "at" << monitor.geometry;
    }
    
    m_monitors = monitors;
    return monitors;
}

} // namespace WallpaperCore 