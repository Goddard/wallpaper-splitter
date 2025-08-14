#include "core/wallpaper_applier.h"
#include <QDebug>
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>
#include <algorithm>

namespace WallpaperCore {

WallpaperApplier::WallpaperApplier(QObject* parent)
    : QObject(parent)
{
}

bool WallpaperApplier::applyWallpaper(const MonitorInfo& monitor, 
                                     const QString& wallpaperPath)
{
    if (!isSupported()) {
        qWarning() << "Desktop environment not supported:" << getDesktopEnvironment();
        return false;
    }
    
    QFileInfo fileInfo(wallpaperPath);
    if (!fileInfo.exists()) {
        qWarning() << "Wallpaper file does not exist:" << wallpaperPath;
        emit wallpaperFailed(monitor, "Wallpaper file does not exist");
        return false;
    }
    
    // For KDE Plasma, we use the plasma-apply-wallpaperimage command
    // This applies to the entire desktop (all monitors)
    QProcess process;
    QStringList arguments;
    arguments << wallpaperPath;
    
    process.start("plasma-apply-wallpaperimage", arguments);
    
    if (!process.waitForFinished(10000)) { // 10 second timeout
        qWarning() << "Timeout applying wallpaper to monitor:" << monitor.name;
        emit wallpaperFailed(monitor, "Timeout applying wallpaper");
        return false;
    }
    
    if (process.exitCode() != 0) {
        QString error = QString::fromUtf8(process.readAllStandardError());
        qWarning() << "Failed to apply wallpaper to monitor" << monitor.name 
                   << ":" << error;
        emit wallpaperFailed(monitor, error);
        return false;
    }
    
    qDebug() << "Successfully applied wallpaper to monitor:" << monitor.name;
    emit wallpaperApplied(monitor, wallpaperPath);
    return true;
}

bool WallpaperApplier::applyWallpapers(const MonitorList& monitors)
{
    // For KDE Plasma, we need to set different wallpaper images for each monitor
    // using the DBus interface with JavaScript scripting
    if (monitors.empty()) {
        return false;
    }
    
    // Sort monitors by x position (left to right) to match the order we split the image
    MonitorList sortedMonitors = monitors;
    std::sort(sortedMonitors.begin(), sortedMonitors.end(), 
              [](const MonitorInfo& a, const MonitorInfo& b) {
                  return a.geometry.x() < b.geometry.x();
              });
    
    // Filter out disabled monitors (those with empty wallpaperPath)
    MonitorList enabledMonitors;
    for (const auto& monitor : sortedMonitors) {
        if (!monitor.wallpaperPath.isEmpty()) {
            enabledMonitors.push_back(monitor);
        }
    }
    
    if (enabledMonitors.empty()) {
        qDebug() << "No enabled monitors found";
        return false;
    }
    
    // Check if this is a single monitor setup (original image path, not split)
    bool isSingleMonitor = false;
    if (enabledMonitors.size() == 1) {
        QFileInfo wallpaperFile(enabledMonitors[0].wallpaperPath);
        // Check if the wallpaper path is the original image (not a split image)
        if (!wallpaperFile.fileName().startsWith("wallpaper_") && 
            !wallpaperFile.fileName().startsWith("a_wallpaper_") && 
            !wallpaperFile.fileName().startsWith("b_wallpaper_")) {
            isSingleMonitor = true;
            qDebug() << "Single monitor mode detected - applying original image directly";
        }
    }
    
    // For single monitor, use a simplified DBus script
    if (isSingleMonitor) {
        qDebug() << "Using simplified DBus script for single monitor";
        
        QString imagePath = QString("file://%1").arg(enabledMonitors[0].wallpaperPath);
        
        // Create a simple script for single monitor
        QString script = QString(R"(
const ds = desktops();
for (let i = 0; i < ds.length; i++) {
    const desktop = ds[i];
    desktop.wallpaperPlugin = 'org.kde.image';
    desktop.currentConfigGroup = Array('Wallpaper', 'org.kde.image', 'General');
    desktop.writeConfig('Image', '%1');
    desktop.reloadConfig();
    print('Applied wallpaper to desktop ' + i + ' (screen ' + desktop.screen + '): %1');
}
)").arg(imagePath);
        
        qDebug() << "Executing single monitor DBus script...";
        qDebug() << "Image path:" << imagePath;
        
        // Execute the script via DBus
        QProcess process;
        process.start("qdbus", QStringList() << "org.kde.plasmashell" << "/PlasmaShell" 
                     << "org.kde.PlasmaShell.evaluateScript" << script);
        
        if (!process.waitForFinished(10000)) { // 10 second timeout
            qWarning() << "Timeout executing single monitor DBus script";
            return false;
        }
        
        if (process.exitCode() != 0) {
            QString error = QString::fromUtf8(process.readAllStandardError());
            qWarning() << "Failed to execute single monitor DBus script:" << error;
            return false;
        }
        
        qDebug() << "Successfully executed single monitor DBus script";
        qDebug() << "Script output:" << QString::fromUtf8(process.readAllStandardOutput());
        
        return true;
    }
    
    // Determine which prefix to use (use the most recently created files)
    QString outputDir;
    if (!enabledMonitors.empty() && !enabledMonitors[0].wallpaperPath.isEmpty()) {
        QFileInfo fileInfo(enabledMonitors[0].wallpaperPath);
        outputDir = fileInfo.absolutePath();
    } else {
        outputDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/wallpaper-splitter";
    }
    
    // Check which prefix has the most recently created files
    QString prefix = "a_";
    QFileInfo testFileA(QString("%1/a_wallpaper_0.jpg").arg(outputDir));
    QFileInfo testFileB(QString("%1/b_wallpaper_0.jpg").arg(outputDir));
    
    if (testFileA.exists() && testFileB.exists()) {
        // Both exist, use the newer one
        if (testFileA.lastModified() > testFileB.lastModified()) {
            prefix = "a_";
            qDebug() << "Using a_ prefix (newer files)";
        } else {
            prefix = "b_";
            qDebug() << "Using b_ prefix (newer files)";
        }
    } else if (testFileA.exists()) {
        prefix = "a_";
        qDebug() << "Found a_ files, using a_ prefix";
    } else if (testFileB.exists()) {
        prefix = "b_";
        qDebug() << "Found b_ files, using b_ prefix";
    } else {
        // Fallback to a_ if no files exist
        prefix = "a_";
        qDebug() << "No prefix files found, using a_ prefix";
    }
    
    // Build a simpler JavaScript script that works reliably
    QString script = QString(R"(
const ds = desktops();
const enabledMonitors = [
)");
    
    // Add monitor geometry mappings
    for (int i = 0; i < enabledMonitors.size(); ++i) {
        const auto& monitor = enabledMonitors[i];
        QString key = QString("'%1x%2+%3+%4'")
            .arg(monitor.geometry.width())
            .arg(monitor.geometry.height())
            .arg(monitor.geometry.x())
            .arg(monitor.geometry.y());
        QString imagePath = QString("'file://%1/%2wallpaper_%3.jpg'")
            .arg(outputDir).arg(prefix).arg(i);
        
        script += QString("  { key: %1, image: %2, index: %3 },\n")
            .arg(key).arg(imagePath).arg(i);
    }
    
    script += R"(];

// Apply wallpapers based on geometry matching
for (let i = 0; i < ds.length; i++) {
    const desktop = ds[i];
    const geom = screenGeometry(desktop.screen);
    const key = geom.width + 'x' + geom.height + '+' + geom.left + '+' + geom.top;
    
    for (let j = 0; j < enabledMonitors.length; j++) {
        const monitor = enabledMonitors[j];
        if (monitor.key === key) {
            desktop.wallpaperPlugin = 'org.kde.image';
            desktop.currentConfigGroup = Array('Wallpaper', 'org.kde.image', 'General');
            desktop.writeConfig('Image', monitor.image);
            desktop.reloadConfig();
            print('Applied wallpaper to desktop ' + i + ' (screen ' + desktop.screen + '): ' + monitor.image);
            break;
        }
    }
}
)";
    
    qDebug() << "Executing DBus script to set wallpapers...";
    qDebug() << "Using prefix:" << prefix;
    qDebug() << "Enabled monitors in order (left to right):";
    for (int i = 0; i < enabledMonitors.size(); ++i) {
        qDebug() << "  " << i << ":" << enabledMonitors[i].name 
                 << "at x=" << enabledMonitors[i].geometry.x()
                 << "y=" << enabledMonitors[i].geometry.y()
                 << "size=" << enabledMonitors[i].geometry.width() << "x" << enabledMonitors[i].geometry.height();
    }
    
    // Execute the script via DBus
    QProcess process;
    process.start("qdbus", QStringList() << "org.kde.plasmashell" << "/PlasmaShell" 
                 << "org.kde.PlasmaShell.evaluateScript" << script);
    
    if (!process.waitForFinished(10000)) { // 10 second timeout
        qWarning() << "Timeout executing DBus script";
        return false;
    }
    
    if (process.exitCode() != 0) {
        QString error = QString::fromUtf8(process.readAllStandardError());
        qWarning() << "Failed to execute DBus script:" << error;
        return false;
    }
    
    qDebug() << "Successfully executed DBus script to set wallpapers";
    qDebug() << "Script output:" << QString::fromUtf8(process.readAllStandardOutput());
    
    return true;
}

QString WallpaperApplier::getCurrentWallpaper(const MonitorInfo& monitor)
{
    // For KDE Plasma, we can read the wallpaper from the config
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) 
                        + "/plasma-org.kde.plasma.desktop-appletsrc";
    
    QSettings settings(configPath, QSettings::IniFormat);
    QString wallpaperPath = settings.value("Containments/1/Applets/1/Configuration/Image", "").toString();
    
    return wallpaperPath;
}

bool WallpaperApplier::isSupported()
{
    QString desktop = getDesktopEnvironment();
    return desktop == "KDE" || desktop == "plasma";
}

QString WallpaperApplier::getDesktopEnvironment()
{
    QString desktop = qgetenv("XDG_CURRENT_DESKTOP");
    if (desktop.isEmpty()) {
        desktop = qgetenv("DESKTOP_SESSION");
    }
    
    if (desktop.contains("plasma", Qt::CaseInsensitive) || 
        desktop.contains("kde", Qt::CaseInsensitive)) {
        return "KDE";
    }
    
    return desktop;
}

} // namespace WallpaperCore
