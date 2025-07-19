#include "imagepreview.h"
#include "monitoroverlay.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QFileInfo>
#include <QResizeEvent>
#include <KLocalizedString>
#include <QDebug> // Added for debug logging
#include <QCoreApplication> // Added for application directory
#include <QFile> // Added for file existence check

ImagePreview::ImagePreview(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0); // Remove margins
    
    // Create a container for the image and overlays
    m_overlayContainer = new QWidget(this);
    m_overlayContainer->setMinimumSize(400, 300);
    m_overlayContainer->setStyleSheet("QWidget { border: 1px solid gray; background-color: black; }");
    
    // Create image label inside the container
    m_imageLabel = new QLabel(m_overlayContainer);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setText(i18n("No image selected"));
    m_imageLabel->setStyleSheet("QLabel { background-color: transparent; color: white; }");
    
    layout->addWidget(m_overlayContainer);
}

void ImagePreview::setImage(const QString& imagePath)
{
    if (imagePath.isEmpty()) {
        // Load default image when no image is selected
        QString defaultImagePath = QCoreApplication::applicationDirPath() + "/default-image.jpg";
        
        // If we're in a Flatpak, try the app directory
        if (defaultImagePath.startsWith("/app/")) {
            defaultImagePath = "/app/default-image.jpg";
        } else {
            // Try relative to the executable
            defaultImagePath = QCoreApplication::applicationDirPath() + "/default-image.jpg";
        }
        
        // Fallback to project root if not found
        if (!QFile::exists(defaultImagePath)) {
            defaultImagePath = QCoreApplication::applicationDirPath() + "/../default-image.jpg";
        }
        
        if (QFile::exists(defaultImagePath)) {
            m_pixmap.load(defaultImagePath);
            if (!m_pixmap.isNull()) {
                qDebug() << "Loaded default image from:" << defaultImagePath;
                updateOverlayPositions();
                return;
            }
        }
        
        // If default image fails, show placeholder text
        m_imageLabel->setText(i18n("No image selected"));
        m_pixmap = QPixmap();
        return;
    }
    
    QFileInfo fileInfo(imagePath);
    if (!fileInfo.exists()) {
        m_imageLabel->setText(i18n("Image file not found"));
        m_pixmap = QPixmap();
        return;
    }
    
    m_pixmap.load(imagePath);
    if (m_pixmap.isNull()) {
        m_imageLabel->setText(i18n("Failed to load image"));
        return;
    }
    
    updateOverlayPositions();
}

void ImagePreview::setMonitors(const WallpaperCore::MonitorList& monitors)
{
    m_monitors = monitors;
    
    // Clear existing overlays
    for (auto overlay : m_overlays) {
        delete overlay;
    }
    m_overlays.clear();
    
    // Create new overlays
    for (size_t i = 0; i < monitors.size(); ++i) {
        MonitorOverlay* overlay = new MonitorOverlay(monitors[i], static_cast<int>(i), m_overlayContainer);
        connect(overlay, &MonitorOverlay::toggled, this, &ImagePreview::monitorToggled);
        m_overlays.append(overlay);
    }
    
    updateOverlayPositions();
}

void ImagePreview::updateMonitorOverlays()
{
    updateOverlayPositions();
}

void ImagePreview::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateOverlayPositions();
}

void ImagePreview::updateOverlayPositions()
{
    if (m_pixmap.isNull() || m_monitors.empty()) {
        return;
    }
    
    // Calculate the virtual desktop bounds using logical geometry (divide by device pixel ratio)
    QRect virtualDesktop;
    for (const auto& monitor : m_monitors) {
        // Convert actual geometry to logical geometry (divide by device pixel ratio)
        // The actual geometry is already in logical coordinates, we don't need to divide
        QRect logicalGeometry = monitor.geometry;
        virtualDesktop = virtualDesktop.united(logicalGeometry);
        qDebug() << "Monitor:" << monitor.name << "actual geometry:" << monitor.geometry << "logical:" << logicalGeometry;
    }
    qDebug() << "Virtual desktop bounds (logical):" << virtualDesktop;
    
    // Get the container size
    QSize containerSize = m_overlayContainer->size();
    qDebug() << "Container size:" << containerSize;
    
    // Calculate scale factors to fit the virtual desktop in the container
    double scaleX = static_cast<double>(containerSize.width()) / virtualDesktop.width();
    double scaleY = static_cast<double>(containerSize.height()) / virtualDesktop.height();
    double scale = qMin(scaleX, scaleY) * 0.9; // Leave some margin
    qDebug() << "Scale factors - X:" << scaleX << "Y:" << scaleY << "Final:" << scale;
    
    // Calculate offset to center the layout
    int offsetX = (containerSize.width() - virtualDesktop.width() * scale) / 2;
    int offsetY = (containerSize.height() - virtualDesktop.height() * scale) / 2;
    qDebug() << "Offsets - X:" << offsetX << "Y:" << offsetY;
    
    // Position the image label to cover the entire virtual desktop area
    m_imageLabel->setGeometry(offsetX, offsetY, 
                             virtualDesktop.width() * scale, 
                             virtualDesktop.height() * scale);
    
    // Scale the image to fit the virtual desktop area (not the container)
    QPixmap scaledPixmap = m_pixmap.scaled(m_imageLabel->size(), 
                                          Qt::KeepAspectRatio, 
                                          Qt::SmoothTransformation);
    m_imageLabel->setPixmap(scaledPixmap);
    
    // Calculate the actual image area within the label (accounting for aspect ratio)
    QSize scaledImageSize = scaledPixmap.size();
    QSize labelSize = m_imageLabel->size();
    qDebug() << "Image sizes - Original:" << m_pixmap.size() << "Scaled:" << scaledImageSize << "Label:" << labelSize;
    
    // Calculate image offset within the label (centered)
    int imageOffsetX = (labelSize.width() - scaledImageSize.width()) / 2;
    int imageOffsetY = (labelSize.height() - scaledImageSize.height()) / 2;
    qDebug() << "Image offsets - X:" << imageOffsetX << "Y:" << imageOffsetY;
    
    // Position overlays relative to the image, not the virtual desktop
    for (int i = 0; i < m_overlays.size() && i < m_monitors.size(); ++i) {
        MonitorOverlay* overlay = m_overlays[i];
        const auto& monitor = m_monitors[i];
        
        // Use the geometry directly (it should already be in logical coordinates)
        QRect logicalGeometry = monitor.geometry;
        
        // Calculate the monitor's position relative to the virtual desktop
        double relativeX = static_cast<double>(logicalGeometry.x()) / virtualDesktop.width();
        double relativeY = static_cast<double>(logicalGeometry.y()) / virtualDesktop.height();
        double relativeWidth = static_cast<double>(logicalGeometry.width()) / virtualDesktop.width();
        double relativeHeight = static_cast<double>(logicalGeometry.height()) / virtualDesktop.height();
        
        // Apply these relative positions to the scaled image area
        int x = offsetX + imageOffsetX + (relativeX * scaledImageSize.width());
        int y = offsetY + imageOffsetY + (relativeY * scaledImageSize.height());
        int width = relativeWidth * scaledImageSize.width();
        int height = relativeHeight * scaledImageSize.height();
        
        qDebug() << "Monitor" << i << "(" << monitor.name << ") - Logical:" << logicalGeometry
                 << "Relative:" << relativeX << relativeY << relativeWidth << relativeHeight
                 << "Final:" << x << y << width << height;
        
        overlay->setGeometry(QRect(x, y, width, height));
        overlay->show();
    }
} 