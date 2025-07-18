#include "core/image_splitter.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <algorithm>

namespace WallpaperCore {

bool ImageSplitter::splitImage(const QString& inputPath, 
                              const MonitorList& monitors,
                              const QString& outputDir)
{
    if (monitors.empty()) {
        qWarning() << "No monitors provided for image splitting";
        return false;
    }
    
    if (!validateImage(inputPath, monitors)) {
        return false;
    }
    
    QDir dir(outputDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // Sort monitors by x position (left to right)
    MonitorList sortedMonitors = monitors;
    std::sort(sortedMonitors.begin(), sortedMonitors.end(), 
              [](const MonitorInfo& a, const MonitorInfo& b) {
                  return a.geometry.x() < b.geometry.x();
              });
    
    qDebug() << "Monitors sorted by x position:";
    for (int i = 0; i < sortedMonitors.size(); ++i) {
        qDebug() << "  " << i << ":" << sortedMonitors[i].name 
                 << "at x=" << sortedMonitors[i].geometry.x();
    }
    
    // Determine which prefix to use (alternate between a_ and b_)
    QString prefix = "a_";
    QFileInfo testFileA(dir.filePath("a_wallpaper_0.jpg"));
    QFileInfo testFileB(dir.filePath("b_wallpaper_0.jpg"));
    
    if (testFileA.exists() && testFileB.exists()) {
        // Both exist, use the opposite of the newer one
        if (testFileA.lastModified() > testFileB.lastModified()) {
            prefix = "b_";  // a_ is newer, so use b_
            qDebug() << "Found existing a_ files (newer), switching to b_ prefix";
        } else {
            prefix = "a_";  // b_ is newer, so use a_
            qDebug() << "Found existing b_ files (newer), switching to a_ prefix";
        }
    } else if (testFileA.exists()) {
        prefix = "b_";
        qDebug() << "Found existing a_ files, switching to b_ prefix";
    } else if (testFileB.exists()) {
        prefix = "a_";
        qDebug() << "Found existing b_ files, switching to a_ prefix";
    } else {
        // No files exist, start with a_
        prefix = "a_";
        qDebug() << "No existing files found, using a_ prefix";
    }
    
    // Create individual split images for each monitor
    bool allSuccess = true;
    
    for (int i = 0; i < sortedMonitors.size(); ++i) {
        const auto& monitor = sortedMonitors[i];
        // Use alternating prefix naming: a_wallpaper_0.jpg or b_wallpaper_0.jpg
        QString outputPath = dir.filePath(QString("%1wallpaper_%2.jpg").arg(prefix).arg(i));
        
        if (!splitImageForMonitor(inputPath, monitor, outputPath, i)) {
            qWarning() << "Failed to split image for monitor:" << monitor.name;
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

bool ImageSplitter::splitImageForMonitor(const QString& inputPath,
                                        const MonitorInfo& monitor,
                                        const QString& outputPath,
                                        int monitorIndex)
{
    // Load image using Qt
    QImage image(inputPath);
    if (image.isNull()) {
        qWarning() << "Failed to load image:" << inputPath;
        return false;
    }
    
    QSize imageSize = image.size();
    
    // Use the passed monitorIndex directly (0, 1, 2) for simple horizontal splitting
    // Calculate crop rectangle for simple horizontal splitting (like the reference script)
    int sectionWidth = imageSize.width() / 3; // 33.33% of image width
    int cropX = monitorIndex * sectionWidth;
    int cropY = 0;
    int cropWidth = sectionWidth;
    int cropHeight = imageSize.height(); // 100% of image height
    
    // Crop the image for this monitor
    QImage cropped = image.copy(QRect(cropX, cropY, cropWidth, cropHeight));
    
    // Resize to monitor resolution if needed
    if (cropWidth != monitor.geometry.width() || cropHeight != monitor.geometry.height()) {
        cropped = cropped.scaled(monitor.geometry.width(), 
                                monitor.geometry.height(),
                                Qt::IgnoreAspectRatio,
                                Qt::SmoothTransformation);
    }
    
    // Save the cropped image
    if (!cropped.save(outputPath, "JPEG", 95)) { // 95% quality
        qWarning() << "Failed to save image:" << outputPath;
        return false;
    }
    
    qDebug() << "Split image for monitor" << monitor.name 
             << "(index" << monitorIndex << ") saved to" << outputPath;
    
    return true;
}

QSize ImageSplitter::getOptimalImageSize(const MonitorList& monitors)
{
    if (monitors.empty()) {
        return QSize();
    }
    
    // Calculate the total virtual desktop size
    int minX = 0, minY = 0, maxX = 0, maxY = 0;
    
    for (const auto& monitor : monitors) {
        minX = qMin(minX, monitor.geometry.x());
        minY = qMin(minY, monitor.geometry.y());
        maxX = qMax(maxX, monitor.geometry.x() + monitor.geometry.width());
        maxY = qMax(maxY, monitor.geometry.y() + monitor.geometry.height());
    }
    
    return QSize(maxX - minX, maxY - minY);
}

bool ImageSplitter::validateImage(const QString& imagePath, 
                                 const MonitorList& monitors)
{
    QFileInfo fileInfo(imagePath);
    if (!fileInfo.exists() || !fileInfo.isReadable()) {
        qWarning() << "Image file does not exist or is not readable:" << imagePath;
        return false;
    }
    
    // Load image using Qt
    QImage image(imagePath);
    if (image.isNull()) {
        qWarning() << "Failed to load image:" << imagePath;
        return false;
    }
    
    QSize imageSize = image.size();
    QSize optimalSize = getOptimalImageSize(monitors);
    
    if (imageSize.width() < optimalSize.width() || 
        imageSize.height() < optimalSize.height()) {
        qWarning() << "Image size" << imageSize 
                   << "is smaller than optimal size" << optimalSize;
        return false;
    }
    
    return true;
}

QRect ImageSplitter::calculateCropRect(const QSize& imageSize, 
                                      const MonitorInfo& monitor,
                                      const MonitorList& allMonitors)
{
    QSize optimalSize = getOptimalImageSize(allMonitors);
    
    // Calculate the scale factor between image and virtual desktop
    double scaleX = static_cast<double>(imageSize.width()) / optimalSize.width();
    double scaleY = static_cast<double>(imageSize.height()) / optimalSize.height();
    
    // Calculate the crop rectangle in image coordinates
    int cropX = static_cast<int>(monitor.geometry.x() * scaleX);
    int cropY = static_cast<int>(monitor.geometry.y() * scaleY);
    int cropWidth = static_cast<int>(monitor.geometry.width() * scaleX);
    int cropHeight = static_cast<int>(monitor.geometry.height() * scaleY);
    
    return QRect(cropX, cropY, cropWidth, cropHeight);
}

int ImageSplitter::getMonitorIndex(const MonitorInfo& monitor)
{
    // This function is deprecated - monitor index should be passed directly
    // Keep for backward compatibility but it's not used in the new approach
    qWarning() << "getMonitorIndex is deprecated - use direct index passing instead";
    return 0;
}

} // namespace WallpaperCore 