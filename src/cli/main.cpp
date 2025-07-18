#include <QGuiApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>
#include "core/monitor_detector.h"
#include "core/image_splitter.h"
#include "core/wallpaper_applier.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("wallpaper-splitter-cli");
    app.setApplicationVersion("1.0.0");
    
    QCommandLineParser parser;
    parser.setApplicationDescription("Split wallpapers for multi-monitor setups");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption imageOption(QStringList() << "i" << "image",
        "Input image file to split", "file");
    parser.addOption(imageOption);
    
    QCommandLineOption outputOption(QStringList() << "o" << "output",
        "Output directory for split images", "directory");
    parser.addOption(outputOption);
    
    QCommandLineOption applyOption(QStringList() << "a" << "apply",
        "Apply wallpapers after splitting");
    parser.addOption(applyOption);
    
    QCommandLineOption listOption(QStringList() << "l" << "list",
        "List detected monitors");
    parser.addOption(listOption);
    
    parser.process(app);
    
    // Initialize core components
    WallpaperCore::MonitorDetector detector;
    WallpaperCore::ImageSplitter splitter;
    WallpaperCore::WallpaperApplier applier;
    
    // List monitors if requested
    if (parser.isSet(listOption)) {
        WallpaperCore::MonitorList monitors = detector.detectMonitors();
        qInfo() << "Detected monitors:";
        for (const auto& monitor : monitors) {
            qInfo() << "  " << monitor.name 
                    << "(" << monitor.geometry.width() << "x" << monitor.geometry.height()
                    << " at" << monitor.geometry.x() << "," << monitor.geometry.y() << ")"
                    << (monitor.isPrimary ? " [Primary]" : "");
        }
        return 0;
    }
    
    // Check required options
    if (!parser.isSet(imageOption)) {
        qCritical() << "Error: Input image file is required. Use -i option.";
        parser.showHelp(1);
    }
    
    QString imagePath = parser.value(imageOption);
    QString outputDir = parser.value(outputOption);
    
    if (outputDir.isEmpty()) {
        // Use writable location in Flatpak or next to executable as default
        QString appDir = QCoreApplication::applicationDirPath();
        if (appDir.startsWith("/app/")) {
            // We're in a Flatpak, use user's home directory
            outputDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.wallpaper-splitter";
        } else {
            // We're not in a Flatpak, use directory next to executable
            outputDir = appDir + "/wallpaper-splitter";
        }
    }
    
    // Detect monitors
    WallpaperCore::MonitorList monitors = detector.detectMonitors();
    if (monitors.empty()) {
        qCritical() << "Error: No monitors detected.";
        return 1;
    }
    
    qInfo() << "Detected" << monitors.size() << "monitor(s)";
    
    // Split image
    qInfo() << "Splitting image:" << imagePath;
    if (!splitter.splitImage(imagePath, monitors, outputDir)) {
        qCritical() << "Error: Failed to split image.";
        return 1;
    }
    
    qInfo() << "Image split successfully. Output directory:" << outputDir;
    
    // Apply wallpapers if requested
    if (parser.isSet(applyOption)) {
        qInfo() << "Applying wallpapers...";
        
        // Update monitor wallpaper paths to use individual split images with index-based naming
        for (int i = 0; i < monitors.size(); ++i) {
            monitors[i].wallpaperPath = outputDir + QString("/wallpaper_%1.jpg").arg(i);
        }
        
        if (!applier.applyWallpapers(monitors)) {
            qWarning() << "Warning: Some wallpapers failed to apply.";
            return 1;
        }
        
        qInfo() << "Wallpapers applied successfully.";
    }
    
    return 0;
} 