#pragma once

#include "monitor_info.h"
#include <QString>
#include <QSize>
#include <QRect>
#include <vector>

namespace WallpaperCore {

class ImageSplitter {
public:
    ImageSplitter() = default;
    virtual ~ImageSplitter() = default;

    // Split image for multiple monitors
    virtual bool splitImage(const QString& inputPath, 
                           const MonitorList& monitors,
                           const QString& outputDir);
    
    // Split image for specific monitor
    virtual bool splitImageForMonitor(const QString& inputPath,
                                     const MonitorInfo& monitor,
                                     const QString& outputPath,
                                     int monitorIndex);
    
    // Get optimal image size for monitor layout
    virtual QSize getOptimalImageSize(const MonitorList& monitors);
    
    // Validate if image can be split for given monitors
    virtual bool validateImage(const QString& imagePath, 
                              const MonitorList& monitors);

protected:
    // Helper method to calculate crop rectangle for monitor
    QRect calculateCropRect(const QSize& imageSize, 
                           const MonitorInfo& monitor,
                           const MonitorList& allMonitors);
    
    // Helper method to get monitor index for simple horizontal splitting
    int getMonitorIndex(const MonitorInfo& monitor);
};

} // namespace WallpaperCore 