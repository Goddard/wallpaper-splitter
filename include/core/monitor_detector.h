#pragma once

#include "monitor_info.h"
#include <QObject>

namespace WallpaperCore {

class MonitorDetector : public QObject {
    Q_OBJECT

public:
    explicit MonitorDetector(QObject* parent = nullptr);
    virtual ~MonitorDetector() = default;

    // Detect all connected monitors
    virtual MonitorList detectMonitors();
    
    // Get the primary monitor
    virtual MonitorInfo getPrimaryMonitor();
    
    // Refresh monitor list
    virtual void refreshMonitors();

signals:
    void monitorsChanged();

protected:
    MonitorList m_monitors;
};

} // namespace WallpaperCore 