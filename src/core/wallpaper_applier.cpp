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
    
    // Determine which prefix to use (use the most recently created files)
    QString outputDir;
    if (!sortedMonitors.empty() && !sortedMonitors[0].wallpaperPath.isEmpty()) {
        QFileInfo fileInfo(sortedMonitors[0].wallpaperPath);
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
    
    // Build the JavaScript script to set wallpapers
    QString script = R"(
function getDesktops() {
    return desktops()
        .filter(d => d.screen != -1)
        .sort((a, b) => screenGeometry(a.screen).left - screenGeometry(b.screen).left);
}

function setWallpaper(desktop, path) {
    desktop.wallpaperPlugin = "org.kde.image"
    desktop.currentConfigGroup = Array("Wallpaper", "org.kde.image", "General")
    desktop.writeConfig("Image", path)
    // Force a reload to trigger wallpaperChanged signal
    desktop.reloadConfig()
}

const imageList = [)";
    
    // Add image paths to the script in the correct order (left to right)
    // Use alternating prefix naming: a_wallpaper_0.jpg or b_wallpaper_0.jpg
    QStringList imagePaths;
    for (int i = 0; i < sortedMonitors.size(); ++i) {
        const auto& monitor = sortedMonitors[i];
        // Get the output directory from the first monitor's wallpaper path
        QString outputDir;
        if (!monitor.wallpaperPath.isEmpty()) {
            QFileInfo fileInfo(monitor.wallpaperPath);
            outputDir = fileInfo.absolutePath();
        } else {
            // Fallback: use a default directory
            outputDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/wallpaper-splitter";
        }
        
        QString imagePath = QString("%1/%2wallpaper_%3.jpg").arg(outputDir).arg(prefix).arg(i);
        imagePaths.append(QString("'file://%1'").arg(imagePath));
    }
    
    script += imagePaths.join(",") + R"(];

// Apply wallpapers in order: leftmost (index 0) gets image 0, middle (index 1) gets image 1, rightmost (index 2) gets image 2
getDesktops().forEach((desktop, i) => {
    if (i < imageList.length) {
        setWallpaper(desktop, imageList[i]);
    }
});
)";
    
    qDebug() << "Executing DBus script to set wallpapers...";
    qDebug() << "Using prefix:" << prefix;
    qDebug() << "Monitors in order (left to right):";
    for (int i = 0; i < sortedMonitors.size(); ++i) {
        qDebug() << "  " << i << ":" << sortedMonitors[i].name << "at x=" << sortedMonitors[i].geometry.x();
    }
    qDebug() << "Image paths in order:" << imagePaths;
    
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