#pragma once

#include "monitor_info.h"
#include <QString>
#include <QObject>

namespace WallpaperCore {

class WallpaperApplier : public QObject {
    Q_OBJECT

public:
    explicit WallpaperApplier(QObject* parent = nullptr);
    virtual ~WallpaperApplier() = default;

    // Apply wallpaper to specific monitor
    virtual bool applyWallpaper(const MonitorInfo& monitor, 
                               const QString& wallpaperPath);
    
    // Apply wallpapers to all monitors
    virtual bool applyWallpapers(const MonitorList& monitors);
    
    // Get current wallpaper for monitor
    virtual QString getCurrentWallpaper(const MonitorInfo& monitor);
    
    // Check if desktop environment is supported
    virtual bool isSupported();
    
    // Get desktop environment name
    virtual QString getDesktopEnvironment();

signals:
    void wallpaperApplied(const MonitorInfo& monitor, const QString& path);
    void wallpaperFailed(const MonitorInfo& monitor, const QString& error);
};

} // namespace WallpaperCore 