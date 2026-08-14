#pragma once

#include <QString>

class QPalette;

namespace Theme {

enum class Mode { System = 0, Light = 1, Dark = 2 };

Mode mode();
QString modeName(Mode mode);
Mode currentEffectiveMode();
QPalette paletteFor(Mode mode);
QString stylesheetFor(Mode mode);
void setMode(Mode mode);
void refresh();
void restoreSavedMode();
void setSystemColorScheme(Qt::ColorScheme scheme);

}
