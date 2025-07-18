#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <KLocalizedString>
#include <KAboutData>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
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