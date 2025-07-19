#include "monitoroverlay.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QApplication>
#include <KLocalizedString>

MonitorOverlay::MonitorOverlay(const WallpaperCore::MonitorInfo& monitor, int index, QWidget* parent)
    : QWidget(parent)
    , m_monitor(monitor)
    , m_index(index)
    , m_hovered(false)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setMouseTracking(true);
    
    // Create layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(2);
    
    // Checkbox for enable/disable
    m_checkBox = new QCheckBox(this);
    m_checkBox->setChecked(true);
    m_checkBox->setText(QString::number(index + 1));
    m_checkBox->setStyleSheet("QCheckBox { color: white; font-weight: bold; }");
    layout->addWidget(m_checkBox);
    
    // Info label
    m_infoLabel = new QLabel(this);
    m_infoLabel->setAlignment(Qt::AlignCenter);
    m_infoLabel->setStyleSheet("QLabel { color: white; font-size: 10px; background: transparent; }");
    
    // Display actual resolution
    m_infoLabel->setText(QString("%1x%2").arg(monitor.actualResolution.width()).arg(monitor.actualResolution.height()));
    
    layout->addWidget(m_infoLabel);
    
    // Connect checkbox signal
    connect(m_checkBox, &QCheckBox::toggled, this, [this](bool checked) {
        emit toggled(m_index, checked);
        updateStyle();
    });
    
    updateStyle();
}

void MonitorOverlay::setGeometry(const QRect& geometry)
{
    QWidget::setGeometry(geometry);
    updateStyle();
}

bool MonitorOverlay::isEnabled() const
{
    return m_checkBox->isChecked();
}

void MonitorOverlay::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw border
    QPen pen;
    pen.setWidth(2);
    
    if (m_checkBox->isChecked()) {
        pen.setColor(QColor(0, 255, 0, 200)); // Green when enabled
    } else {
        pen.setColor(QColor(255, 0, 0, 200)); // Red when disabled
    }
    
    painter.setPen(pen);
    painter.drawRect(rect().adjusted(1, 1, -1, -1));
    
    // Draw semi-transparent background
    QColor bgColor;
    if (m_checkBox->isChecked()) {
        bgColor = QColor(0, 255, 0, 30); // Light green
    } else {
        bgColor = QColor(255, 0, 0, 30); // Light red
    }
    
    painter.fillRect(rect().adjusted(1, 1, -1, -1), bgColor);
}

void MonitorOverlay::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_checkBox->toggle();
    }
    QWidget::mousePressEvent(event);
}

void MonitorOverlay::updateStyle()
{
    // Update visual appearance based on state
    update();
} 