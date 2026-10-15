#include "MenuButton.h"

MenuButton::MenuButton(const QString& text, QWidget* parent)
    : QPushButton(text, parent) {
    setObjectName("menuButton");
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(58);
}