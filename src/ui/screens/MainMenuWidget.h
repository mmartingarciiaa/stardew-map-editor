#pragma once

#include <QWidget>

#include <functional>

class MainMenuWidget : public QWidget {
public:
    explicit MainMenuWidget(
        std::function<void()> onCreateMap,
        std::function<void()> onLoadMap,
        std::function<void()> onExit,
        QWidget* parent = nullptr);
};