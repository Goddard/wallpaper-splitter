#include "monitorwidget.h"
#include <KLocalizedString>

MonitorWidget::MonitorWidget(const WallpaperCore::MonitorInfo& monitor, QWidget* parent)
    : QWidget(parent)
    , m_monitor(monitor)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    
    m_enabledCheckBox = new QCheckBox(this);
    m_enabledCheckBox->setChecked(true);
    layout->addWidget(m_enabledCheckBox);
    
    m_nameLabel = new QLabel(monitor.name, this);
    if (monitor.isPrimary) {
        m_nameLabel->setText(monitor.name + i18n(" (Primary)"));
    }
    layout->addWidget(m_nameLabel);
    
    m_resolutionLabel = new QLabel(QString("%1x%2")
        .arg(monitor.geometry.width())
        .arg(monitor.geometry.height()), this);
    layout->addWidget(m_resolutionLabel);
    
    m_positionLabel = new QLabel(QString("(%1, %2)")
        .arg(monitor.geometry.x())
        .arg(monitor.geometry.y()), this);
    layout->addWidget(m_positionLabel);
    
    layout->addStretch();
}

WallpaperCore::MonitorInfo MonitorWidget::getMonitorInfo() const
{
    WallpaperCore::MonitorInfo info = m_monitor;
    info.wallpaperPath = m_enabledCheckBox->isChecked() ? m_monitor.wallpaperPath : QString();
    return info;
}

bool MonitorWidget::isSelected() const
{
    return m_enabledCheckBox->isChecked();
} 