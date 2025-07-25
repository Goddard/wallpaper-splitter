#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QTimer>
#include <QListWidget>
#include <QListWidgetItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

class ImageGalleryItem : public QWidget {
    Q_OBJECT

public:
    explicit ImageGalleryItem(const QString& imagePath, QWidget* parent = nullptr);
    QString getImagePath() const { return m_imagePath; }
    void setSelected(bool selected);

signals:
    void removeRequested();
    void selected();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    QString m_imagePath;
    QPushButton* m_removeButton;
    bool m_selected;
    
    void updateStyle();
};

class ImageGallery : public QWidget {
    Q_OBJECT

public:
    explicit ImageGallery(QWidget* parent = nullptr);
    
    QString getCurrentImage() const;
    void setCurrentImage(const QString& imagePath);
    QStringList getAllImages() const;
    bool hasImages() const;

public slots:
    void addImage();
    void removeImage(const QString& imagePath);
    void startAutoChange();
    void stopAutoChange();
    void nextImage();
    void previousImage();

signals:
    void imageSelected(const QString& imagePath);
    void autoChangeToggled(bool enabled);

private slots:
    void onTimerTimeout();
    void onIntervalChanged(int value);
    void onItemSelected(QListWidgetItem* item);
    void onRemoveRequested();

private:
    void setupUI();
    void updateTimerLabel();
    void loadImages();
    void saveImages();
    
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_controlsLayout;
    QSlider* m_intervalSlider;
    QLabel* m_intervalLabel;
    QPushButton* m_autoChangeButton;
    QPushButton* m_addButton;
    QPushButton* m_previousButton;
    QPushButton* m_nextButton;
    QListWidget* m_imageList;
    QTimer* m_changeTimer;
    
    QString m_currentImage;
    QStringList m_imagePaths;
    bool m_autoChangeEnabled;
    int m_currentIndex;
    
    static const QString CONFIG_FILE;
}; 