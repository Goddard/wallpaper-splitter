#pragma once

#include <QWidget>
#include <QLabel>
#include <QPixmap>

class ImagePreview : public QWidget {
    Q_OBJECT

public:
    explicit ImagePreview(QWidget* parent = nullptr);

public slots:
    void setImage(const QString& imagePath);

private:
    QLabel* m_imageLabel;
    QPixmap m_pixmap;
}; 