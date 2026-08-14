#include "theme.h"

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>

namespace Theme {

static const char *kSettingsKey = "theme/mode";
static Mode s_mode = Mode::System;
static Qt::ColorScheme s_systemScheme = Qt::ColorScheme::Unknown;

namespace {

struct StyleColors {
    QString window;
    QString base;
    QString altBase;
    QString text;
    QString subtext;
    QString border;
    QString button;
    QString buttonText;
    QString buttonHover;
    QString buttonPressed;
    QString highlight;
    QString highlightText;
    QString disabledText;
    QString tooltip;
    QString tooltipText;
    QString headerText;
    QString input;
    QString inputBorder;
    QString scrollbar;
    QString scrollbarHover;
};

StyleColors colorsFor(Mode mode)
{
    StyleColors c;
    if (mode == Mode::Light) {
        c.window = QStringLiteral("#f5f5f7");
        c.base = QStringLiteral("#ffffff");
        c.altBase = QStringLiteral("#f0f0f4");
        c.text = QStringLiteral("#1d1d1f");
        c.subtext = QStringLiteral("#6e6e73");
        c.border = QStringLiteral("#d0d0d8");
        c.button = QStringLiteral("#e9e9ee");
        c.buttonHover = QStringLiteral("#e0e0e6");
        c.buttonPressed = QStringLiteral("#d6d6dd");
        c.highlight = QStringLiteral("#0a84ff");
        c.highlightText = QStringLiteral("#ffffff");
        c.disabledText = QStringLiteral("#a0a0a8");
        c.tooltip = QStringLiteral("#ffffff");
        c.tooltipText = QStringLiteral("#1d1d1f");
        c.headerText = QStringLiteral("#4a4a52");
        c.input = QStringLiteral("#ffffff");
        c.inputBorder = QStringLiteral("#8a8a92");
        c.scrollbar = QStringLiteral("#c0c0c8");
        c.scrollbarHover = QStringLiteral("#a0a0a8");
    } else {
        c.window = QStringLiteral("#242427");
        c.base = QStringLiteral("#1c1c1e");
        c.altBase = QStringLiteral("#2a2a2e");
        c.text = QStringLiteral("#e8e8ea");
        c.subtext = QStringLiteral("#9a9aa2");
        c.border = QStringLiteral("#3f3f46");
        c.button = QStringLiteral("#333336");
        c.buttonHover = QStringLiteral("#3d3d41");
        c.buttonPressed = QStringLiteral("#47474c");
        c.highlight = QStringLiteral("#0a84ff");
        c.highlightText = QStringLiteral("#ffffff");
        c.disabledText = QStringLiteral("#686870");
        c.tooltip = QStringLiteral("#3a3a3e");
        c.tooltipText = QStringLiteral("#e8e8ea");
        c.headerText = QStringLiteral("#b8b8c0");
        c.input = QStringLiteral("#1c1c1e");
        c.inputBorder = QStringLiteral("#5a5a62");
        c.scrollbar = QStringLiteral("#45454a");
        c.scrollbarHover = QStringLiteral("#55555c");
    }
    return c;
}

QColor toColor(const QString &hex)
{
    return QColor(hex);
}

} // namespace

Mode mode()
{
    return s_mode;
}

QString modeName(Mode mode)
{
    switch (mode) {
    case Mode::System:
        return QStringLiteral("System");
    case Mode::Light:
        return QStringLiteral("Light");
    case Mode::Dark:
        return QStringLiteral("Dark");
    }
    return QStringLiteral("System");
}

Mode currentEffectiveMode()
{
    if (s_mode != Mode::System)
        return s_mode;
    if (s_systemScheme == Qt::ColorScheme::Dark)
        return Mode::Dark;
    if (s_systemScheme == Qt::ColorScheme::Light)
        return Mode::Light;
    const QColor window = QApplication::palette().color(QPalette::Window);
    return window.lightness() < 128 ? Mode::Dark : Mode::Light;
}

QPalette paletteFor(Mode mode)
{
    QPalette pal;
    if (mode == Mode::Dark) {
        pal.setColor(QPalette::Window, QColor(0x24, 0x24, 0x27));
        pal.setColor(QPalette::WindowText, QColor(0xe8, 0xe8, 0xea));
        pal.setColor(QPalette::Base, QColor(0x1c, 0x1c, 0x1e));
        pal.setColor(QPalette::AlternateBase, QColor(0x2a, 0x2a, 0x2e));
        pal.setColor(QPalette::ToolTipBase, QColor(0x3a, 0x3a, 0x3e));
        pal.setColor(QPalette::ToolTipText, QColor(0xe8, 0xe8, 0xea));
        pal.setColor(QPalette::Text, QColor(0xe8, 0xe8, 0xea));
        pal.setColor(QPalette::Button, QColor(0x33, 0x33, 0x36));
        pal.setColor(QPalette::ButtonText, QColor(0xe8, 0xe8, 0xea));
        pal.setColor(QPalette::BrightText, QColor(0xff, 0x40, 0x40));
        pal.setColor(QPalette::Highlight, QColor(0x0a, 0x84, 0xff));
        pal.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
        pal.setColor(QPalette::PlaceholderText, QColor(0x9a, 0x9a, 0xa2));
        pal.setColor(QPalette::Link, QColor(0x4f, 0xa8, 0xff));
        pal.setColor(QPalette::LinkVisited, QColor(0x9a, 0x7a, 0xe8));
        pal.setColor(QPalette::Light, QColor(0x3f, 0x3f, 0x43));
        pal.setColor(QPalette::Midlight, QColor(0x39, 0x39, 0x3d));
        pal.setColor(QPalette::Mid, QColor(0x2a, 0x2a, 0x2e));
        pal.setColor(QPalette::Dark, QColor(0x1a, 0x1a, 0x1c));
        pal.setColor(QPalette::Shadow, QColor(0x00, 0x00, 0x00));
        const QColor disabled(0x68, 0x68, 0x70);
        pal.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
        pal.setColor(QPalette::Disabled, QPalette::Text, disabled);
        pal.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
        pal.setColor(QPalette::Disabled, QPalette::HighlightedText, disabled);
        pal.setColor(QPalette::Disabled, QPalette::PlaceholderText, disabled);
        pal.setColor(QPalette::Disabled, QPalette::ToolTipText, disabled);
    } else {
        pal.setColor(QPalette::Window, QColor(0xf5, 0xf5, 0xf7));
        pal.setColor(QPalette::WindowText, QColor(0x1d, 0x1d, 0x1f));
        pal.setColor(QPalette::Base, QColor(0xff, 0xff, 0xff));
        pal.setColor(QPalette::AlternateBase, QColor(0xf0, 0xf0, 0xf4));
        pal.setColor(QPalette::ToolTipBase, QColor(0xff, 0xff, 0xff));
        pal.setColor(QPalette::ToolTipText, QColor(0x1d, 0x1d, 0x1f));
        pal.setColor(QPalette::Text, QColor(0x1d, 0x1d, 0x1f));
        pal.setColor(QPalette::Button, QColor(0xe9, 0xe9, 0xee));
        pal.setColor(QPalette::ButtonText, QColor(0x1d, 0x1d, 0x1f));
        pal.setColor(QPalette::BrightText, QColor(0xff, 0x00, 0x00));
        pal.setColor(QPalette::Highlight, QColor(0x0a, 0x84, 0xff));
        pal.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
        pal.setColor(QPalette::PlaceholderText, QColor(0x8a, 0x8a, 0x92));
        pal.setColor(QPalette::Link, QColor(0x00, 0x66, 0xcc));
        pal.setColor(QPalette::LinkVisited, QColor(0x6a, 0x34, 0xb0));
        pal.setColor(QPalette::Light, QColor(0xff, 0xff, 0xff));
        pal.setColor(QPalette::Midlight, QColor(0xf2, 0xf2, 0xf6));
        pal.setColor(QPalette::Mid, QColor(0xd6, 0xd6, 0xdd));
        pal.setColor(QPalette::Dark, QColor(0xb8, 0xb8, 0xc0));
        pal.setColor(QPalette::Shadow, QColor(0x80, 0x80, 0x88));
        const QColor disabled(0xa0, 0xa0, 0xa8);
        pal.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
        pal.setColor(QPalette::Disabled, QPalette::Text, disabled);
        pal.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
        pal.setColor(QPalette::Disabled, QPalette::HighlightedText, disabled);
        pal.setColor(QPalette::Disabled, QPalette::PlaceholderText, disabled);
        pal.setColor(QPalette::Disabled, QPalette::ToolTipText, disabled);
    }
    return pal;
}

QString stylesheetFor(Mode mode)
{
    const StyleColors c = colorsFor(mode);
    return QStringLiteral(
        "QMainWindow, QDialog { background: %1; }\n"
        "QWidget { color: %2; font-size: 13px; }\n"
        "QMenuBar { background: %1; color: %2; border-bottom: 1px solid %5; }\n"
        "QMenuBar::item { background: transparent; padding: 4px 8px; }\n"
        "QMenuBar::item:selected { background: %7; }\n"
        "QMenu { background: %3; color: %2; border: 1px solid %5; padding: 4px; }\n"
        "QMenu::item { padding: 4px 24px 4px 8px; border-radius: 3px; }\n"
        "QMenu::item:selected { background: %7; color: %8; }\n"
        "QMenu::item:disabled { color: %10; }\n"
        "QMenu::separator { height: 1px; background: %5; margin: 4px 8px; }\n"
        "QToolTip { background: %11; color: %12; border: 1px solid %5; padding: 4px; }\n"
        "QStatusBar { background: %1; color: %4; border-top: 1px solid %5; }\n"
        "QStatusBar::item { border: none; }\n"
        "QGroupBox { border: 1px solid %5; border-radius: 4px; margin-top: 12px; "
        "padding-top: 4px; font-weight: bold; background: transparent; }\n"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }\n"
        "QTabWidget::pane { border: 1px solid %5; border-radius: 4px; background: %3; }\n"
        "QTabBar::tab { background: transparent; color: %4; padding: 5px 14px; "
        "border: 1px solid transparent; border-top-left-radius: 4px; "
        "border-top-right-radius: 4px; }\n"
        "QTabBar::tab:selected { background: %3; color: %2; border-color: %5; "
        "border-bottom-color: %3; }\n"
        "QTabBar::tab:hover:!selected { color: %2; }\n"
        "QPushButton { background: %6; color: %2; border: 1px solid %5; "
        "border-radius: 4px; padding: 4px 12px; }\n"
        "QPushButton:hover { background: %15; }\n"
        "QPushButton:pressed { background: %16; }\n"
        "QPushButton:disabled { color: %10; background: %6; }\n"
        "QPushButton:default { border-color: %7; }\n"
        "QComboBox, QLineEdit, QSpinBox { background: %17; color: %2; "
        "border: 1px solid %5; border-radius: 4px; padding: 3px 6px; "
        "selection-background-color: %7; selection-color: %8; }\n"
        "QComboBox:hover, QLineEdit:hover, QSpinBox:hover { border-color: %18; }\n"
        "QComboBox:focus, QLineEdit:focus, QSpinBox:focus { border-color: %7; }\n"
        "QComboBox QAbstractItemView { background: %3; color: %2; border: 1px solid %5; "
        "selection-background-color: %7; selection-color: %8; }\n"
        "QComboBox::drop-down { border: none; width: 18px; }\n"
        "QCheckBox, QRadioButton { spacing: 6px; }\n"
        "QCheckBox::indicator, QRadioButton::indicator { width: 14px; height: 14px; }\n"
        "QCheckBox::indicator { border: 1px solid %5; border-radius: 3px; background: %17; }\n"
        "QCheckBox::indicator:checked { background: %7; border-color: %7; }\n"
        "QCheckBox::indicator:hover, QRadioButton::indicator:hover { border-color: %18; }\n"
        "QRadioButton::indicator { border: 1px solid %5; border-radius: 7px; background: %17; }\n"
        "QRadioButton::indicator:checked { border: 4px solid %7; background: %17; }\n"
        "QTableWidget, QTableView { background: %3; alternate-background-color: %19; "
        "color: %2; gridline-color: %5; border: 1px solid %5; border-radius: 4px; }\n"
        "QHeaderView::section { background: %6; color: %14; padding: 4px 6px; "
        "border: none; border-right: 1px solid %5; border-bottom: 1px solid %5; }\n"
        "QTableCornerButton::section { background: %6; border: none; }\n"
        "QTableView::item:selected { background: %7; color: %8; }\n"
        "QScrollBar:vertical { background: transparent; width: 12px; margin: 0; }\n"
        "QScrollBar::handle:vertical { background: %20; min-height: 24px; "
        "border-radius: 5px; margin: 2px; }\n"
        "QScrollBar::handle:vertical:hover { background: %21; }\n"
        "QScrollBar:horizontal { background: transparent; height: 12px; }\n"
        "QScrollBar::handle:horizontal { background: %20; min-width: 24px; "
        "border-radius: 5px; margin: 2px; }\n"
        "QScrollBar::handle:horizontal:hover { background: %21; }\n"
        "QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }\n"
        "QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }\n"
        "QToolButton { background: transparent; border: 1px solid transparent; "
        "border-radius: 4px; padding: 4px; }\n"
        "QToolButton:hover { background: %15; border-color: %5; }\n"
        "QToolButton:pressed { background: %16; }\n"
        "QLabel { background: transparent; }\n"
        "QMessageBox { background: %1; }\n"
        "QListWidget, QListView { background: %3; color: %2; border: 1px solid %5; "
        "border-radius: 4px; }\n"
        "QListView::item:selected { background: %7; color: %8; }\n"
        "QProgressBar { background: %6; border: 1px solid %5; border-radius: 4px; "
        "text-align: center; }\n"
        "QProgressBar::chunk { background: %7; border-radius: 3px; }\n"
        "QSplitter::handle { background: %5; }\n")
        .arg(c.window, c.text, c.base, c.subtext, c.border, c.button, c.highlight,
             c.highlightText, c.buttonText, c.disabledText, c.tooltip, c.tooltipText,
             c.altBase, c.headerText, c.buttonHover, c.buttonPressed, c.input,
             c.inputBorder, c.altBase, c.scrollbar, c.scrollbarHover);
}

void apply()
{
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    const Mode effective = currentEffectiveMode();
    QApplication::setPalette(paletteFor(effective));
    if (QApplication *app = qobject_cast<QApplication *>(QCoreApplication::instance()))
        app->setStyleSheet(stylesheetFor(effective));
}

void setMode(Mode mode)
{
    s_mode = mode;
    apply();

    QSettings settings;
    settings.setValue(QLatin1String(kSettingsKey), static_cast<int>(mode));
}

void refresh()
{
    apply();
}

void restoreSavedMode()
{
    QSettings settings;
    const int stored = settings.value(QLatin1String(kSettingsKey),
                                      static_cast<int>(Mode::System)).toInt();
    s_mode = stored >= static_cast<int>(Mode::System) && stored <= static_cast<int>(Mode::Dark)
                 ? static_cast<Mode>(stored)
                 : Mode::System;
}

void setSystemColorScheme(Qt::ColorScheme scheme)
{
    if (scheme == Qt::ColorScheme::Unknown)
        return;
    s_systemScheme = scheme;
    if (s_mode == Mode::System)
        refresh();
}

} // namespace Theme
