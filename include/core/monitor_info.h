#pragma once

#include <QString>
#include <QRect>
#include <vector>

namespace WallpaperCore {

struct MonitorInfo {
    QString name;
    QRect geometry;
    bool isPrimary;
    QString wallpaperPath;
    
    MonitorInfo() : isPrimary(false) {}
    MonitorInfo(const QString& n, const QRect& g, bool primary = false)
        : name(n), geometry(g), isPrimary(primary) {}
};

using MonitorList = std::vector<MonitorInfo>;

} // namespace WallpaperCore 