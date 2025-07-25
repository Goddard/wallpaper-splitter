#pragma once

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QScrollArea>
#include <QVector>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>
#include <QSettings>
#include "core/monitor_detector.h"
#include "core/image_splitter.h"
#include "core/wallpaper_applier.h"
#include "core/monitor_info.h"
#include "imagepreview.h"
#include "imagegallery.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void refreshMonitors();
    void applyWallpapers();
    void onMonitorsChanged();
    void onWallpaperApplied(const WallpaperCore::MonitorInfo& monitor, const QString& path);
    void onWallpaperFailed(const WallpaperCore::MonitorInfo& monitor, const QString& error);
    void onMonitorToggled(int monitorIndex, bool enabled);
    void onImageSelected(const QString& imagePath);
    void onAutoChangeToggled(bool enabled);

private:
    void setupUI();
    void setupSystemTray();
    void updateImagePreview();
    void closeEvent(QCloseEvent* event) override;
    WallpaperCore::MonitorList getEnabledMonitors() const;
    void saveMonitorStates();
    void loadMonitorStates();

    // Core components
    WallpaperCore::MonitorDetector* m_monitorDetector;
    WallpaperCore::ImageSplitter* m_imageSplitter;
    WallpaperCore::WallpaperApplier* m_wallpaperApplier;

    // UI components
    QWidget* m_centralWidget;
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_topLayout;
    QPushButton* m_refreshMonitorsButton;
    QPushButton* m_applyButton;
    QProgressBar* m_progressBar;
    ImagePreview* m_imagePreview;
    ImageGallery* m_imageGallery;

    // Data
    QString m_selectedImagePath;
    QString m_outputDir;
    WallpaperCore::MonitorList m_monitors;
    QVector<bool> m_monitorEnabled; // Track which monitors are enabled
    bool m_autoChangeEnabled; // Track if auto-change is enabled
    
    // System tray
    QSystemTrayIcon* m_systemTray;
    QMenu* m_trayMenu;
}; 