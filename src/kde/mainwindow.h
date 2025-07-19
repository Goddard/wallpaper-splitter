#pragma once

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QScrollArea>
#include <QVector>
#include "core/monitor_detector.h"
#include "core/image_splitter.h"
#include "core/wallpaper_applier.h"
#include "core/monitor_info.h"
#include "imagepreview.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void selectImage();
    void refreshMonitors();
    void applyWallpapers();
    void onMonitorsChanged();
    void onWallpaperApplied(const WallpaperCore::MonitorInfo& monitor, const QString& path);
    void onWallpaperFailed(const WallpaperCore::MonitorInfo& monitor, const QString& error);
    void onMonitorToggled(int monitorIndex, bool enabled);

private:
    void setupUI();
    void updateImagePreview();
    WallpaperCore::MonitorList getEnabledMonitors() const;

    // Core components
    WallpaperCore::MonitorDetector* m_monitorDetector;
    WallpaperCore::ImageSplitter* m_imageSplitter;
    WallpaperCore::WallpaperApplier* m_wallpaperApplier;

    // UI components
    QWidget* m_centralWidget;
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_topLayout;
    QPushButton* m_selectImageButton;
    QLabel* m_imagePathLabel;
    QPushButton* m_refreshMonitorsButton;
    QPushButton* m_applyButton;
    QProgressBar* m_progressBar;
    ImagePreview* m_imagePreview;

    // Data
    QString m_selectedImagePath;
    QString m_outputDir;
    WallpaperCore::MonitorList m_monitors;
    QVector<bool> m_monitorEnabled; // Track which monitors are enabled
}; 