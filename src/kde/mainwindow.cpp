#include "mainwindow.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>
#include <QIcon>
#include <QFileDialog>
#include <QFileInfo>
#include <KLocalizedString>
#include <KMessageBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_monitorDetector(nullptr)
    , m_imageSplitter(nullptr)
    , m_wallpaperApplier(nullptr)
    , m_autoChangeEnabled(false)
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
    connect(m_imagePreview, &ImagePreview::monitorToggled,
            this, &MainWindow::onMonitorToggled);
    connect(m_imageGallery, &ImageGallery::imageSelected,
            this, &MainWindow::onImageSelected);
    connect(m_imageGallery, &ImageGallery::autoChangeToggled,
            this, &MainWindow::onAutoChangeToggled);
    
    // Setup system tray
    setupSystemTray();
    
    // Load saved states
    loadMonitorStates();
    loadApplicationState();
    
    // Initial monitor detection
    refreshMonitors();
    
    // Restore auto-change state to image gallery
    if (m_autoChangeEnabled) {
        m_imageGallery->setAutoChangeEnabled(true);
    }
    
    // Load default image to show monitor layout
    updateImagePreview();
}

MainWindow::~MainWindow()
{
    // Save monitor states before destruction
    saveMonitorStates();
    delete m_imageSplitter;
}

void MainWindow::setupUI()
{
    setWindowTitle(i18n("Wallpaper Splitter"));
    setMinimumSize(1000, 700);
    
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
    m_refreshMonitorsButton = new QPushButton(i18n("Refresh Monitors"), this);
    m_applyButton = new QPushButton(i18n("Apply Wallpapers"), this);
    
    m_topLayout->addWidget(m_refreshMonitorsButton);
    m_topLayout->addStretch();
    m_topLayout->addWidget(m_applyButton);
    
    m_mainLayout->addLayout(m_topLayout);
    
    // Progress bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    m_mainLayout->addWidget(m_progressBar);
    
    // Image gallery
    m_imageGallery = new ImageGallery(this);
    m_mainLayout->addWidget(m_imageGallery);
    
    // Image preview with monitor overlays
    m_imagePreview = new ImagePreview(this);
    m_mainLayout->addWidget(m_imagePreview, 1); // Give it more space
    
    // Connect signals
    connect(m_refreshMonitorsButton, &QPushButton::clicked, this, &MainWindow::refreshMonitors);
    connect(m_applyButton, &QPushButton::clicked, this, &MainWindow::applyWallpapers);
    
    // Initial state
    m_applyButton->setEnabled(false);
}



void MainWindow::refreshMonitors()
{
    // Store current monitor enabled state before refreshing
    QVector<bool> oldMonitorEnabled = m_monitorEnabled;
    
    m_monitors = m_monitorDetector->detectMonitors();
    
    // Preserve enabled state for existing monitors, enable new ones by default
    m_monitorEnabled.clear();
    for (int i = 0; i < m_monitors.size(); ++i) {
        // If this monitor index existed before, preserve its state
        if (i < oldMonitorEnabled.size()) {
            m_monitorEnabled.append(oldMonitorEnabled[i]);
        } else {
            // New monitor, enable by default
            m_monitorEnabled.append(true);
        }
    }
    
    // Save the updated monitor states after refresh
    saveMonitorStates();
    
    updateImagePreview();
    m_applyButton->setEnabled(!m_selectedImagePath.isEmpty() && !m_monitors.empty());
}

void MainWindow::applyWallpapers()
{
    if (m_selectedImagePath.isEmpty() || m_monitors.empty()) {
        // Don't show popup for auto-change, just log and return
        qDebug() << "Cannot apply wallpapers: No image selected or no monitors detected";
        return;
    }
    
    WallpaperCore::MonitorList enabledMonitors = getEnabledMonitors();
    if (enabledMonitors.empty()) {
        // Don't show popup for auto-change, just log and return
        qDebug() << "Cannot apply wallpapers: No monitors enabled";
        return;
    }
    
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, enabledMonitors.size());
    m_progressBar->setValue(0);
    m_applyButton->setEnabled(false);
    
    bool success = false;
    
    // Check if we have only one monitor - if so, apply the image directly without splitting
    if (enabledMonitors.size() == 1) {
        qDebug() << "Single monitor detected - applying image directly without splitting";
        
        // For single monitor, we can apply the original image directly
        // Set the wallpaper path to the original image
        enabledMonitors[0].wallpaperPath = m_selectedImagePath;
        
        // Apply wallpaper directly
        success = m_wallpaperApplier->applyWallpapers(enabledMonitors);
    } else {
        // Multiple monitors - split the image as before
        qDebug() << "Multiple monitors detected - splitting image for" << enabledMonitors.size() << "monitors";
        
        // Split the image
        if (!m_imageSplitter->splitImage(m_selectedImagePath, enabledMonitors, m_outputDir)) {
            KMessageBox::error(this, i18n("Failed to split image for monitors."));
            m_progressBar->setVisible(false);
            m_applyButton->setEnabled(true);
            return;
        }
        
        // Set wallpaper paths for each monitor to use individual split images with index-based naming
        for (size_t i = 0; i < enabledMonitors.size(); ++i) {
            enabledMonitors[i].wallpaperPath = m_outputDir + QString("/wallpaper_%1.jpg").arg(i);
        }
        
        // Apply wallpapers
        success = m_wallpaperApplier->applyWallpapers(enabledMonitors);
    }
    
    m_progressBar->setVisible(false);
    m_applyButton->setEnabled(true);
    
    // Log the result to console instead of showing popup
    if (success) {
        qDebug() << "Wallpapers applied successfully!";
    } else {
        qWarning() << "Some wallpapers failed to apply. Check the console for details.";
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

void MainWindow::onMonitorToggled(int monitorIndex, bool enabled)
{
    if (monitorIndex >= 0 && monitorIndex < m_monitorEnabled.size()) {
        m_monitorEnabled[monitorIndex] = enabled;
        m_applyButton->setEnabled(!m_selectedImagePath.isEmpty() && getEnabledMonitors().size() > 0);
        
        // Save the updated monitor states
        saveMonitorStates();
    }
}



void MainWindow::updateImagePreview()
{
    if (!m_selectedImagePath.isEmpty()) {
        m_imagePreview->setImage(m_selectedImagePath);
    } else {
        // Load default image to show monitor layout
        m_imagePreview->setImage(QString());
    }
    
    // Pass the current monitor enabled states
    m_imagePreview->setMonitors(m_monitors, m_monitorEnabled);
}

WallpaperCore::MonitorList MainWindow::getEnabledMonitors() const
{
    WallpaperCore::MonitorList enabledMonitors;
    for (size_t i = 0; i < m_monitors.size(); ++i) {
        if (i < m_monitorEnabled.size() && m_monitorEnabled[static_cast<int>(i)]) {
            enabledMonitors.push_back(m_monitors[i]);
        }
    }
    return enabledMonitors;
}

void MainWindow::setupSystemTray()
{
    // Create system tray icon
    m_systemTray = new QSystemTrayIcon(this);
    
    // Set tray icon - use the same icon as the window
    QIcon trayIcon;
    trayIcon.addFile("/app/share/icons/hicolor/128x128/apps/org.wallpapersplitter.app.png");
    if (trayIcon.isNull()) {
        trayIcon = QIcon::fromTheme("org.wallpapersplitter.app");
    }
    if (trayIcon.isNull()) {
        trayIcon = QIcon::fromTheme("applications-graphics");
    }
    m_systemTray->setIcon(trayIcon);
    
    // Create tray menu
    m_trayMenu = new QMenu();
    
    // Add menu actions
    QAction* showAction = new QAction(i18n("Show"), this);
    QAction* quitAction = new QAction(i18n("Quit"), this);
    
    m_trayMenu->addAction(showAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(quitAction);
    
    // Connect actions
    connect(showAction, &QAction::triggered, this, &QWidget::show);
    connect(showAction, &QAction::triggered, this, &QWidget::raise);
    connect(showAction, &QAction::triggered, this, &QWidget::activateWindow);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    
    // Set tray menu
    m_systemTray->setContextMenu(m_trayMenu);
    
    // Connect tray icon signals
    connect(m_systemTray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            show();
            raise();
            activateWindow();
        }
    });
    
    // Show tray icon
    m_systemTray->show();
    
    // Set tooltip
    m_systemTray->setToolTip(i18n("Wallpaper Splitter"));
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // Save all states before hiding
    saveMonitorStates();
    saveApplicationState();
    
    // Hide the window instead of closing
    hide();
    event->ignore();
}

void MainWindow::onImageSelected(const QString& imagePath)
{
    m_selectedImagePath = imagePath;
    updateImagePreview();
    m_applyButton->setEnabled(!imagePath.isEmpty() && !m_monitors.empty());
    
    // If auto-change is enabled, automatically apply the new wallpaper
    if (m_autoChangeEnabled && !imagePath.isEmpty() && !m_monitors.empty()) {
        applyWallpapers();
    }
}

void MainWindow::onAutoChangeToggled(bool enabled)
{
    m_autoChangeEnabled = enabled;
    
    // Save the auto-change state
    saveApplicationState();
    
    if (enabled) {
        // Auto-change is enabled, apply wallpapers if we have an image and monitors
        if (!m_selectedImagePath.isEmpty() && !m_monitors.empty()) {
            applyWallpapers();
        }
    }
}

void MainWindow::saveMonitorStates()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/wallpaper-splitter/monitors.conf";
    
    // Ensure config directory exists
    QDir configDir = QFileInfo(configPath).absoluteDir();
    if (!configDir.exists()) {
        configDir.mkpath(".");
    }
    
    QSettings settings(configPath, QSettings::IniFormat);
    
    // Save monitor enabled states
    for (int i = 0; i < m_monitorEnabled.size(); ++i) {
        settings.setValue(QString("monitors/enabled_%1").arg(i), m_monitorEnabled[i]);
    }
    
    // Save the number of monitors for validation
    settings.setValue("monitors/count", m_monitorEnabled.size());
}

void MainWindow::loadMonitorStates()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/wallpaper-splitter/monitors.conf";
    QSettings settings(configPath, QSettings::IniFormat);
    
    // Load saved monitor states
    int savedCount = settings.value("monitors/count", 0).toInt();
    
    // Pre-populate with default enabled state
    m_monitorEnabled.clear();
    for (int i = 0; i < savedCount; ++i) {
        bool enabled = settings.value(QString("monitors/enabled_%1").arg(i), true).toBool();
        m_monitorEnabled.append(enabled);
    }
}

void MainWindow::saveApplicationState()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/wallpaper-splitter/application.conf";
    
    // Ensure config directory exists
    QDir configDir = QFileInfo(configPath).absoluteDir();
    if (!configDir.exists()) {
        configDir.mkpath(".");
    }
    
    QSettings settings(configPath, QSettings::IniFormat);
    
    // Save auto-change state
    settings.setValue("autoChange/enabled", m_autoChangeEnabled);
    
    // Save selected image path
    settings.setValue("image/selectedPath", m_selectedImagePath);
}

void MainWindow::loadApplicationState()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/wallpaper-splitter/application.conf";
    QSettings settings(configPath, QSettings::IniFormat);
    
    // Load auto-change state
    m_autoChangeEnabled = settings.value("autoChange/enabled", false).toBool();
    
    // Load selected image path
    m_selectedImagePath = settings.value("image/selectedPath", "").toString();
} 