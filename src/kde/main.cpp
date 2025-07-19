#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QIcon>
#include <QFile>
#include <QDebug>
#include <KLocalizedString>
#include <KAboutData>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application icon for Wayland compatibility - try multiple paths
    QIcon appIcon;
    QStringList iconPaths = {
        "/app/share/icons/hicolor/256x256/apps/org.wallpapersplitter.app.png",
        "/app/share/icons/hicolor/128x128/apps/org.wallpapersplitter.app.png",
        "/app/share/icons/hicolor/64x64/apps/org.wallpapersplitter.app.png",
        "/app/share/icons/hicolor/48x48/apps/org.wallpapersplitter.app.png"
    };
    
    for (const QString& path : iconPaths) {
        if (QFile::exists(path)) {
            appIcon.addFile(path);
            qDebug() << "Added icon from:" << path;
        }
    }
    
    if (!appIcon.isNull()) {
        app.setWindowIcon(appIcon);
        qDebug() << "Set application icon successfully";
    } else {
        qDebug() << "Failed to load application icon";
    }
    
    KLocalizedString::setApplicationDomain("wallpaper-splitter");
    
    KAboutData aboutData(
        QStringLiteral("wallpaper-splitter-kde"),
        i18n("Wallpaper Splitter"),
        QStringLiteral("1.0.0"),
        i18n("Split wallpapers for multi-monitor setups"),
        KAboutLicense::GPL_V3,
        i18n("(c) 2024 Wallpaper Splitter Team")
    );
    
    aboutData.addAuthor(i18n("Wallpaper Splitter Team"), 
                       i18n("Maintainer"), 
                       QStringLiteral("team@wallpaper-splitter.org"));
    
    KAboutData::setApplicationData(aboutData);
    
    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);
    
    MainWindow window;
    window.show();
    
    return app.exec();
} 