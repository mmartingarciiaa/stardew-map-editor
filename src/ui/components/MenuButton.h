#pragma once

#include <QPushButton>

class MenuButton : public QPushButton {
public:
    explicit MenuButton(const QString& text, QWidget* parent = nullptr);
};