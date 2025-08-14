#include "imagegallery.h"
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QPixmap>
#include <QApplication>
#include <QStyle>
#include <QDesktopServices>
#include <QUrl>
#include <KLocalizedString>
#include <QCryptographicHash>
#include <QFile>

const QString ImageGallery::CONFIG_FILE = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/wallpaper-splitter/gallery.conf";
const QString ImageGallery::THUMBNAIL_DIR = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/wallpaper-splitter/thumbnails";

// ImageGalleryItem implementation
ImageGalleryItem::ImageGalleryItem(const QString& imagePath, QWidget* parent)
    : QWidget(parent)
    , m_imagePath(imagePath)
    , m_selected(false)
{
    // Create a layout for the row with proper spacing
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(12);
    
    // Create a container widget for the image background
    QWidget* imageContainer = new QWidget(this);
    imageContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    imageContainer->setMinimumHeight(180);
    
    // Get the thumbnail path for display
    QString displayPath = imagePath;
    if (parent) {
        ImageGallery* gallery = qobject_cast<ImageGallery*>(parent->parent());
        if (gallery) {
            displayPath = gallery->generateThumbnail(imagePath);
        }
    }
    
    // Set the image as the background of the container with better styling
    QPixmap pixmap(displayPath);
    if (!pixmap.isNull()) {
        imageContainer->setStyleSheet(QString("QWidget { background-image: url(%1); background-position: center; background-repeat: no-repeat; border: 2px solid #ddd; border-radius: 8px; }").arg(displayPath));
    } else {
        imageContainer->setStyleSheet("QWidget { background-color: #f8f8f8; border: 2px solid #ddd; border-radius: 8px; }");
    }
    
    // Remove button - no custom styling, use native appearance
    m_removeButton = new QPushButton(this);
    m_removeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    m_removeButton->setFixedSize(28, 28);
    m_removeButton->setToolTip(i18n("Remove from gallery"));
    // No custom styling - uses native KDE button appearance
    
    // Add widgets to layout - image container takes most space, button takes minimum
    layout->addWidget(imageContainer, 1);
    layout->addWidget(m_removeButton, 0);
    
    connect(m_removeButton, &QPushButton::clicked, this, &ImageGalleryItem::removeRequested);
    
    // Set a good height for viewing images with spacing
    setFixedHeight(220);
}



void ImageGalleryItem::setSelected(bool selected)
{
    m_selected = selected;
    updateStyle();
}

void ImageGalleryItem::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit selected();
    }
    QWidget::mousePressEvent(event);
}

void ImageGalleryItem::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    QAction* removeAction = menu.addAction(i18n("Remove from gallery"));
    QAction* openAction = menu.addAction(i18n("Open in file manager"));
    
    QAction* selectedAction = menu.exec(event->globalPos());
    
    if (selectedAction == removeAction) {
        emit removeRequested();
    } else if (selectedAction == openAction) {
        QFileInfo fileInfo(m_imagePath);
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath()));
    }
}

void ImageGalleryItem::updateStyle()
{
    // Use KDE's native theming - no custom styling needed
    // The selection will be handled by the QListWidget's native selection mechanism
}

// ImageGallery implementation
ImageGallery::ImageGallery(QWidget* parent)
    : QWidget(parent)
    , m_autoChangeEnabled(false)
    , m_currentIndex(0)
{
    setupUI();
    loadImages();
    
    m_changeTimer = new QTimer(this);
    connect(m_changeTimer, &QTimer::timeout, this, &ImageGallery::onTimerTimeout);
}

void ImageGallery::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(8);
    
    // Controls layout
    m_controlsLayout = new QHBoxLayout();
    
    // Auto-change controls
    m_intervalLabel = new QLabel(i18n("Change every: 30 minutes"), this);
    // Set fixed width to accommodate the longest possible text: "Change every: 23 hour(s) 59 minute(s)"
    m_intervalLabel->setFixedWidth(200);
    m_intervalSlider = new QSlider(Qt::Horizontal, this);
    m_intervalSlider->setRange(1, 1440); // 1 minute to 24 hours
    m_intervalSlider->setValue(30); // Default 30 minutes
    m_intervalSlider->setToolTip(i18n("Set wallpaper change interval"));
    
    m_autoChangeButton = new QPushButton(i18n("Start Auto-Change"), this);
    m_autoChangeButton->setCheckable(true);
    m_autoChangeButton->setToolTip(i18n("Automatically cycle through wallpapers"));
    
    // Navigation controls
    m_previousButton = new QPushButton(i18n("← Previous"), this);
    m_nextButton = new QPushButton(i18n("Next →"), this);
    m_addButton = new QPushButton(i18n("+ Add Image"), this);
    
    m_controlsLayout->addWidget(m_intervalLabel);
    m_controlsLayout->addWidget(m_intervalSlider);
    m_controlsLayout->addWidget(m_autoChangeButton);
    m_controlsLayout->addStretch();
    m_controlsLayout->addWidget(m_previousButton);
    m_controlsLayout->addWidget(m_nextButton);
    m_controlsLayout->addWidget(m_addButton);
    
    m_mainLayout->addLayout(m_controlsLayout);
    
    // Image list
    m_imageList = new QListWidget(this);
    m_imageList->setSpacing(8); // More spacing between items
    m_imageList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_imageList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_imageList->setMinimumHeight(200);
    m_imageList->setStyleSheet("QListWidget { background-color: transparent; border: none; } QListWidget::item { background-color: transparent; border: none; }");
    
    m_mainLayout->addWidget(m_imageList);
    
    // Connect signals
    connect(m_intervalSlider, &QSlider::valueChanged, this, &ImageGallery::onIntervalChanged);
    connect(m_autoChangeButton, &QPushButton::toggled, this, [this](bool checked) {
        if (checked) {
            startAutoChange();
        } else {
            stopAutoChange();
        }
    });
    connect(m_previousButton, &QPushButton::clicked, this, &ImageGallery::previousImage);
    connect(m_nextButton, &QPushButton::clicked, this, &ImageGallery::nextImage);
    connect(m_addButton, &QPushButton::clicked, this, &ImageGallery::addImage);
    connect(m_imageList, &QListWidget::itemClicked, this, &ImageGallery::onItemSelected);
    
    updateTimerLabel();
}

QString ImageGallery::getCurrentImage() const
{
    return m_currentImage;
}

void ImageGallery::setCurrentImage(const QString& imagePath)
{
    m_currentImage = imagePath;
    m_currentIndex = m_imagePaths.indexOf(imagePath);
    
    // Update selection in list
    for (int i = 0; i < m_imageList->count(); ++i) {
        QListWidgetItem* item = m_imageList->item(i);
        ImageGalleryItem* widget = qobject_cast<ImageGalleryItem*>(m_imageList->itemWidget(item));
        if (widget) {
            widget->setSelected(widget->getImagePath() == imagePath);
        }
    }
    
    // Save the current image index
    saveImages();
    
    emit imageSelected(imagePath);
}

QStringList ImageGallery::getAllImages() const
{
    return m_imagePaths;
}

bool ImageGallery::hasImages() const
{
    return !m_imagePaths.isEmpty();
}

void ImageGallery::addImage()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this,
        i18n("Select Images"),
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        i18n("Image Files (*.png *.jpg *.jpeg *.bmp *.gif *.webp)"));
    
    if (fileNames.isEmpty()) {
        return;
    }
    
    for (const QString& fileName : fileNames) {
        if (!m_imagePaths.contains(fileName)) {
            m_imagePaths.append(fileName);
            
            // Generate thumbnail for the new image
            generateThumbnail(fileName);
            
            // Add to list widget
            QListWidgetItem* item = new QListWidgetItem(m_imageList);
            ImageGalleryItem* widget = new ImageGalleryItem(fileName, m_imageList);
            item->setSizeHint(widget->sizeHint());
            m_imageList->setItemWidget(item, widget);
            
            connect(widget, &ImageGalleryItem::removeRequested, this, &ImageGallery::onRemoveRequested);
            connect(widget, &ImageGalleryItem::selected, this, [this, fileName]() {
                setCurrentImage(fileName);
            });
        }
    }
    
    // Set first image as current if none selected
    if (m_currentImage.isEmpty() && !m_imagePaths.isEmpty()) {
        setCurrentImage(m_imagePaths.first());
    }
    
    saveImages();
}

void ImageGallery::removeImage(const QString& imagePath)
{
    int index = m_imagePaths.indexOf(imagePath);
    if (index >= 0) {
        m_imagePaths.removeAt(index);
        
        // Remove thumbnail file if it exists
        QString thumbnailPath = getThumbnailPath(imagePath);
        QFile::remove(thumbnailPath);
        
        // Remove from list widget
        for (int i = 0; i < m_imageList->count(); ++i) {
            QListWidgetItem* item = m_imageList->item(i);
            ImageGalleryItem* widget = qobject_cast<ImageGalleryItem*>(m_imageList->itemWidget(item));
            if (widget && widget->getImagePath() == imagePath) {
                delete m_imageList->takeItem(i);
                break;
            }
        }
        
        // Update current image if it was removed
        if (m_currentImage == imagePath) {
            if (!m_imagePaths.isEmpty()) {
                setCurrentImage(m_imagePaths.first());
            } else {
                m_currentImage.clear();
                m_currentIndex = -1;
                emit imageSelected(QString());
            }
        }
        
        saveImages();
    }
}

void ImageGallery::startAutoChange()
{
    if (m_imagePaths.isEmpty()) {
        m_autoChangeButton->setChecked(false);
        return;
    }
    
    m_autoChangeEnabled = true;
    m_changeTimer->start(m_intervalSlider->value() * 60 * 1000); // Convert minutes to milliseconds
    m_autoChangeButton->setText(i18n("Stop Auto-Change"));
    
    // Save the auto-change state
    saveImages();
    
    emit autoChangeToggled(true);
}

void ImageGallery::stopAutoChange()
{
    m_autoChangeEnabled = false;
    m_changeTimer->stop();
    m_autoChangeButton->setText(i18n("Start Auto-Change"));
    
    // Save the auto-change state
    saveImages();
    
    emit autoChangeToggled(false);
}

void ImageGallery::nextImage()
{
    if (m_imagePaths.isEmpty()) {
        return;
    }
    
    m_currentIndex = (m_currentIndex + 1) % m_imagePaths.size();
    setCurrentImage(m_imagePaths[m_currentIndex]);
    
    // Save the current image index
    saveImages();
}

void ImageGallery::previousImage()
{
    if (m_imagePaths.isEmpty()) {
        return;
    }
    
    m_currentIndex = (m_currentIndex - 1 + m_imagePaths.size()) % m_imagePaths.size();
    setCurrentImage(m_imagePaths[m_currentIndex]);
    
    // Save the current image index
    saveImages();
}

void ImageGallery::onTimerTimeout()
{
    nextImage();
}

void ImageGallery::onIntervalChanged(int value)
{
    updateTimerLabel();
    
    // Save the interval value
    saveImages();
    
    if (m_autoChangeEnabled) {
        m_changeTimer->start(value * 60 * 1000);
    }
}

void ImageGallery::onItemSelected(QListWidgetItem* item)
{
    ImageGalleryItem* widget = qobject_cast<ImageGalleryItem*>(m_imageList->itemWidget(item));
    if (widget) {
        setCurrentImage(widget->getImagePath());
    }
}

void ImageGallery::onRemoveRequested()
{
    ImageGalleryItem* widget = qobject_cast<ImageGalleryItem*>(sender());
    if (widget) {
        removeImage(widget->getImagePath());
    }
}

void ImageGallery::updateTimerLabel()
{
    int minutes = m_intervalSlider->value();
    if (minutes < 60) {
        m_intervalLabel->setText(i18n("Change every: %1 minute(s)", minutes));
    } else {
        int hours = minutes / 60;
        int remainingMinutes = minutes % 60;
        if (remainingMinutes == 0) {
            m_intervalLabel->setText(i18n("Change every: %1 hour(s)", hours));
        } else {
            m_intervalLabel->setText(i18n("Change every: %1 hour(s) %2 minute(s)", hours, remainingMinutes));
        }
    }
}

void ImageGallery::loadImages()
{
    QSettings settings(CONFIG_FILE, QSettings::IniFormat);
    m_imagePaths = settings.value("gallery/images").toStringList();
    
    // Load saved interval value
    int savedInterval = settings.value("gallery/interval", 30).toInt();
    m_intervalSlider->setValue(savedInterval);
    
    // Load saved auto-change state
    m_autoChangeEnabled = settings.value("gallery/autoChangeEnabled", false).toBool();
    m_autoChangeButton->setChecked(m_autoChangeEnabled);
    
    // Load saved current image index
    m_currentIndex = settings.value("gallery/currentIndex", 0).toInt();
    
    // Clean up any orphaned thumbnails
    cleanupOrphanedThumbnails();
    
    for (const QString& imagePath : m_imagePaths) {
        if (QFile::exists(imagePath)) {
            // Generate thumbnail for existing images
            generateThumbnail(imagePath);
            
            QListWidgetItem* item = new QListWidgetItem(m_imageList);
            ImageGalleryItem* widget = new ImageGalleryItem(imagePath, m_imageList);
            item->setSizeHint(widget->sizeHint());
            m_imageList->setItemWidget(item, widget);
            
            connect(widget, &ImageGalleryItem::removeRequested, this, &ImageGallery::onRemoveRequested);
            connect(widget, &ImageGalleryItem::selected, this, [this, imagePath]() {
                setCurrentImage(imagePath);
            });
        }
    }
    
    if (!m_imagePaths.isEmpty()) {
        // Restore the saved current image index, or use the first image if invalid
        if (m_currentIndex >= 0 && m_currentIndex < m_imagePaths.size()) {
            setCurrentImage(m_imagePaths[m_currentIndex]);
        } else {
            setCurrentImage(m_imagePaths.first());
            m_currentIndex = 0;
        }
    }
}

void ImageGallery::saveImages()
{
    QDir configDir = QFileInfo(CONFIG_FILE).absoluteDir();
    if (!configDir.exists()) {
        configDir.mkpath(".");
    }
    
    QSettings settings(CONFIG_FILE, QSettings::IniFormat);
    settings.setValue("gallery/images", m_imagePaths);
    
    // Save interval value
    settings.setValue("gallery/interval", m_intervalSlider->value());
    
    // Save auto-change state
    settings.setValue("gallery/autoChangeEnabled", m_autoChangeEnabled);
    
    // Save current image index
    settings.setValue("gallery/currentIndex", m_currentIndex);
}

void ImageGallery::ensureThumbnailDirectory()
{
    QDir thumbnailDir(THUMBNAIL_DIR);
    if (!thumbnailDir.exists()) {
        thumbnailDir.mkpath(".");
    }
}

QString ImageGallery::getThumbnailPath(const QString& imagePath)
{
    // Create a unique filename based on the image path hash
    QByteArray hash = QCryptographicHash::hash(imagePath.toUtf8(), QCryptographicHash::Md5);
    QString hashString = hash.toHex();
    
    // Get the original file extension
    QFileInfo fileInfo(imagePath);
    QString extension = fileInfo.suffix().toLower();
    
    // Use PNG for thumbnails to ensure quality and transparency support
    return THUMBNAIL_DIR + "/" + hashString + ".png";
}

QString ImageGallery::generateThumbnail(const QString& imagePath)
{
    ensureThumbnailDirectory();
    
    QString thumbnailPath = getThumbnailPath(imagePath);
    
    // Check if thumbnail already exists and is newer than the original file
    QFileInfo thumbnailInfo(thumbnailPath);
    QFileInfo originalInfo(imagePath);
    
    if (thumbnailInfo.exists() && thumbnailInfo.lastModified() >= originalInfo.lastModified()) {
        return thumbnailPath;
    }
    
    // Load the original image
    QPixmap originalPixmap(imagePath);
    if (originalPixmap.isNull()) {
        return imagePath; // Return original path if loading fails
    }
    
    // Scale down the image while maintaining aspect ratio
    QPixmap thumbnail = originalPixmap.scaled(THUMBNAIL_SIZE, THUMBNAIL_SIZE, 
                                             Qt::KeepAspectRatio, 
                                             Qt::SmoothTransformation);
    
    // Save the thumbnail
    if (thumbnail.save(thumbnailPath, "PNG")) {
        return thumbnailPath;
    } else {
        return imagePath; // Return original path if saving fails
    }
}

void ImageGallery::cleanupOrphanedThumbnails()
{
    ensureThumbnailDirectory();
    
    QDir thumbnailDir(THUMBNAIL_DIR);
    QStringList thumbnailFiles = thumbnailDir.entryList(QStringList() << "*.png", QDir::Files);
    
    for (const QString& thumbnailFile : thumbnailFiles) {
        QString thumbnailPath = THUMBNAIL_DIR + "/" + thumbnailFile;
        
        // Check if this thumbnail corresponds to any existing image
        bool found = false;
        for (const QString& imagePath : m_imagePaths) {
            if (getThumbnailPath(imagePath) == thumbnailPath) {
                found = true;
                break;
            }
        }
        
        // If no corresponding image found, remove the orphaned thumbnail
        if (!found) {
            QFile::remove(thumbnailPath);
        }
    }
}

void ImageGallery::setAutoChangeEnabled(bool enabled)
{
    m_autoChangeEnabled = enabled;
    m_autoChangeButton->setChecked(enabled);
    
    if (enabled) {
        startAutoChange();
    } else {
        stopAutoChange();
    }
} 