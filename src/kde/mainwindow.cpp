#include "mainwindow.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>
#include <QIcon>
#include <KLocalizedString>
#include <KMessageBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_monitorDetector(nullptr)
    , m_imageSplitter(nullptr)
    , m_wallpaperApplier(nullptr)
{
    // Initialize core components
    m_monitorDetector = new WallpaperCore::MonitorDetector(this);
    m_imageSplitter = new WallpaperCore::ImageSplitter();
    m_wallpaperApplier = new WallpaperCore::WallpaperApplier(this);
    
    // Setup output directory - use writable location in Flatpak or next to executable
    QString appDir = QCoreApplication::applicationDirPath();
    if (appDir.startsWith("/app/")) {
        // We're in a Flatpak, use user's home directory
        m_outputDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.wallpaper-splitter";
    } else {
        // We're not in a Flatpak, use directory next to executable
        m_outputDir = appDir + "/wallpaper-splitter";
    }
    
    setupUI();
    
    // Connect signals
    connect(m_monitorDetector, &WallpaperCore::MonitorDetector::monitorsChanged,
            this, &MainWindow::onMonitorsChanged);
    connect(m_wallpaperApplier, &WallpaperCore::WallpaperApplier::wallpaperApplied,
            this, &MainWindow::onWallpaperApplied);
    connect(m_wallpaperApplier, &WallpaperCore::WallpaperApplier::wallpaperFailed,
            this, &MainWindow::onWallpaperFailed);
    
    // Initial monitor detection
    refreshMonitors();
}

MainWindow::~MainWindow()
{
    delete m_imageSplitter;
}

void MainWindow::setupUI()
{
    setWindowTitle(i18n("Wallpaper Splitter"));
    setMinimumSize(800, 600);
    
    // Set window class to match desktop file
    setWindowIconText("org.wallpapersplitter.app");
    
    // Set application icon - try multiple methods for Wayland compatibility
    QIcon appIcon;
    appIcon.addFile("/app/share/icons/hicolor/128x128/apps/org.wallpapersplitter.app.png");
    if (!appIcon.isNull()) {
        setWindowIcon(appIcon);
    } else {
        // Fallback to theme icon
        setWindowIcon(QIcon::fromTheme("org.wallpapersplitter.app"));
    }
    
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    
    // Top controls
    m_topLayout = new QHBoxLayout();
    m_selectImageButton = new QPushButton(i18n("Select Image"), this);
    m_imagePathLabel = new QLabel(i18n("No image selected"), this);
    m_refreshMonitorsButton = new QPushButton(i18n("Refresh Monitors"), this);
    m_applyButton = new QPushButton(i18n("Apply Wallpapers"), this);
    
    m_topLayout->addWidget(m_selectImageButton);
    m_topLayout->addWidget(m_imagePathLabel, 1);
    m_topLayout->addWidget(m_refreshMonitorsButton);
    m_topLayout->addWidget(m_applyButton);
    
    m_mainLayout->addLayout(m_topLayout);
    
    // Progress bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    m_mainLayout->addWidget(m_progressBar);
    
    // Image preview
    m_imagePreview = new ImagePreview(this);
    m_mainLayout->addWidget(m_imagePreview);
    
    // Monitor widgets
    m_monitorScrollArea = new QScrollArea(this);
    m_monitorContainer = new QWidget(this);
    m_monitorLayout = new QVBoxLayout(m_monitorContainer);
    m_monitorScrollArea->setWidget(m_monitorContainer);
    m_monitorScrollArea->setWidgetResizable(true);
    m_mainLayout->addWidget(m_monitorScrollArea);
    
    // Connect signals
    connect(m_selectImageButton, &QPushButton::clicked, this, &MainWindow::selectImage);
    connect(m_refreshMonitorsButton, &QPushButton::clicked, this, &MainWindow::refreshMonitors);
    connect(m_applyButton, &QPushButton::clicked, this, &MainWindow::applyWallpapers);
    
    // Initial state
    m_applyButton->setEnabled(false);
}

void MainWindow::selectImage()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        i18n("Select Wallpaper Image"),
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        i18n("Image Files (*.png *.jpg *.jpeg *.bmp *.gif)"));
    
    if (!fileName.isEmpty()) {
        m_selectedImagePath = fileName;
        m_imagePathLabel->setText(QFileInfo(fileName).fileName());
        updateImagePreview();
        m_applyButton->setEnabled(!m_monitors.empty());
    }
}

void MainWindow::refreshMonitors()
{
    m_monitors = m_monitorDetector->detectMonitors();
    updateMonitorWidgets();
    m_applyButton->setEnabled(!m_selectedImagePath.isEmpty() && !m_monitors.empty());
}

void MainWindow::applyWallpapers()
{
    if (m_selectedImagePath.isEmpty() || m_monitors.empty()) {
        KMessageBox::information(this, i18n("Please select an image and ensure monitors are detected."));
        return;
    }
    
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, m_monitors.size());
    m_progressBar->setValue(0);
    m_applyButton->setEnabled(false);
    
    // Split the image
    if (!m_imageSplitter->splitImage(m_selectedImagePath, m_monitors, m_outputDir)) {
        KMessageBox::error(this, i18n("Failed to split image for monitors."));
        m_progressBar->setVisible(false);
        m_applyButton->setEnabled(true);
        return;
    }
    
    // Set wallpaper paths for each monitor to use individual split images with index-based naming
    for (int i = 0; i < m_monitors.size(); ++i) {
        m_monitors[i].wallpaperPath = m_outputDir + QString("/wallpaper_%1.jpg").arg(i);
    }
    
    // Apply wallpapers
    bool success = m_wallpaperApplier->applyWallpapers(m_monitors);
    
    m_progressBar->setVisible(false);
    m_applyButton->setEnabled(true);
    
    if (success) {
        KMessageBox::information(this, i18n("Wallpapers applied successfully!"));
    } else {
        KMessageBox::information(this, i18n("Some wallpapers failed to apply. Check the console for details."));
    }
}

void MainWindow::onMonitorsChanged()
{
    refreshMonitors();
}

void MainWindow::onWallpaperApplied(const WallpaperCore::MonitorInfo& monitor, const QString& path)
{
    m_progressBar->setValue(m_progressBar->value() + 1);
    qDebug() << "Wallpaper applied to monitor" << monitor.name << ":" << path;
}

void MainWindow::onWallpaperFailed(const WallpaperCore::MonitorInfo& monitor, const QString& error)
{
    m_progressBar->setValue(m_progressBar->value() + 1);
    qWarning() << "Failed to apply wallpaper to monitor" << monitor.name << ":" << error;
}

void MainWindow::updateMonitorWidgets()
{
    // Clear existing widgets
    QLayoutItem* item;
    while ((item = m_monitorLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    
    // Add monitor widgets
    for (const auto& monitor : m_monitors) {
        MonitorWidget* widget = new MonitorWidget(monitor, this);
        m_monitorLayout->addWidget(widget);
    }
}

void MainWindow::updateImagePreview()
{
    if (!m_selectedImagePath.isEmpty()) {
        m_imagePreview->setImage(m_selectedImagePath);
    }
} 