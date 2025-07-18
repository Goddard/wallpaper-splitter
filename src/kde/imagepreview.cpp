#include "imagepreview.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QFileInfo>
#include <KLocalizedString>

ImagePreview::ImagePreview(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    QLabel* titleLabel = new QLabel(i18n("Image Preview"), this);
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);
    
    m_imageLabel = new QLabel(this);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setMinimumSize(300, 200);
    m_imageLabel->setStyleSheet("QLabel { border: 1px solid gray; background-color: #f0f0f0; }");
    m_imageLabel->setText(i18n("No image selected"));
    layout->addWidget(m_imageLabel);
}

void ImagePreview::setImage(const QString& imagePath)
{
    if (imagePath.isEmpty()) {
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
    
    // Scale the pixmap to fit in the preview area while maintaining aspect ratio
    QSize labelSize = m_imageLabel->size();
    QPixmap scaledPixmap = m_pixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    
    m_imageLabel->setPixmap(scaledPixmap);
    m_imageLabel->setToolTip(QString("%1 (%2x%3)")
        .arg(fileInfo.fileName())
        .arg(m_pixmap.width())
        .arg(m_pixmap.height()));
} 