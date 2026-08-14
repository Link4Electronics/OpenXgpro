#include "mainwindow.h"
#include "theme.h"
#include "version.h"

#include <QApplication>
#include <QStyleHints>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QLatin1String(APP_NAME));
    QApplication::setApplicationVersion(QLatin1String(APP_VERSION));
    QApplication::setOrganizationName(QStringLiteral("OpenXgpro"));

    Theme::restoreSavedMode();
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    Theme::setSystemColorScheme(app.styleHints()->colorScheme());
    QObject::connect(app.styleHints(), &QStyleHints::colorSchemeChanged, &app, [] {
        Theme::setSystemColorScheme(QApplication::styleHints()->colorScheme());
    });
#endif
    Theme::setMode(Theme::mode());

    MainWindow window;
    window.show();
    return app.exec();
}
