#pragma once

#include <QString>
#include <QRect>
#include <vector>

namespace WallpaperCore {

struct MonitorInfo {
    QString name;
    QRect geometry;
    QSize actualResolution;  // Store the actual resolution (e.g., 3840x2160 for 4K)
    bool isPrimary;
    QString wallpaperPath;
    
    MonitorInfo() : isPrimary(false) {}
    MonitorInfo(const QString& n, const QRect& g, const QSize& actual, bool primary = false)
        : name(n), geometry(g), actualResolution(actual), isPrimary(primary) {}
};

using MonitorList = std::vector<MonitorInfo>;

} // namespace WallpaperCore 