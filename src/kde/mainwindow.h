#pragma once

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QProgressBar>
#include <QMessageBox>
#include <QScrollArea>
#include <QWidget>
#include <QThread>
#include <QSignalMapper>
#include "core/monitor_detector.h"
#include "core/image_splitter.h"
#include "core/wallpaper_applier.h"
#include "monitorwidget.h"
#include "imagepreview.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void selectImage();
    void applyWallpapers();
    void refreshMonitors();
    void onMonitorsChanged();
    void onWallpaperApplied(const WallpaperCore::MonitorInfo& monitor, const QString& path);
    void onWallpaperFailed(const WallpaperCore::MonitorInfo& monitor, const QString& error);

private:
    void setupUI();
    void updateMonitorWidgets();
    void updateImagePreview();
    
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
    QScrollArea* m_monitorScrollArea;
    QWidget* m_monitorContainer;
    QVBoxLayout* m_monitorLayout;
    ImagePreview* m_imagePreview;
    
    // Data
    QString m_selectedImagePath;
    WallpaperCore::MonitorList m_monitors;
    QString m_outputDir;
}; 