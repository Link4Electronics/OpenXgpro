// Copyright (C) 2026 OpenXgpro contributors
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program. If not, see <https://www.gnu.org/licenses/>.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "mainwindow.h"
#include "theme.h"
#include "version.h"

#include <QApplication>
#include <QStyleHints>
#include <QTimer>

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

    const QString screenshotPath = qEnvironmentVariable("OPENXGPRO_SCREENSHOT");
    if (!screenshotPath.isEmpty()) {
        QTimer::singleShot(300, [&window, screenshotPath] {
            window.grab().save(screenshotPath);
            QCoreApplication::quit();
        });
    }

    return app.exec();
}
