#include "MainMenuWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "ui/components/MenuButton.h"

MainMenuWidget::MainMenuWidget(
    std::function<void()> onCreateMap,
    std::function<void()> onLoadMap,
    std::function<void()> onExit,
    QWidget* parent)
    : QWidget(parent) {
    setObjectName("mainMenu");

    auto* content = new QWidget(this);
    content->setMaximumWidth(520);

    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(18);

    auto* eyebrow = new QLabel("STARDEW VALLEY  /  MAP EDITOR", content);
    eyebrow->setObjectName("eyebrow");

    auto* title = new QLabel("Editor de mapas", content);
    title->setObjectName("menuTitle");

    auto* subtitle = new QLabel("Crear un mundo nuevo o continuar uno existente.", content);
    subtitle->setObjectName("menuSubtitle");
    subtitle->setWordWrap(true);

    auto* createButton = new MenuButton("Crear nuevo mapa", content);
    auto* loadButton = new MenuButton("Cargar mapa existente", content);
    auto* exitButton = new MenuButton("Salir", content);

    contentLayout->addWidget(eyebrow);
    contentLayout->addWidget(title);
    contentLayout->addWidget(subtitle);
    contentLayout->addSpacing(14);
    contentLayout->addWidget(createButton);
    contentLayout->addWidget(loadButton);
    contentLayout->addWidget(exitButton);

    auto* centerLayout = new QHBoxLayout;
    centerLayout->addStretch();
    centerLayout->addWidget(content);
    centerLayout->addStretch();

    auto* pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(40, 32, 40, 32);
    pageLayout->addStretch();
    pageLayout->addLayout(centerLayout);
    pageLayout->addStretch();

    // Conecta el boton de crear con la accion provista por la ventana principal.
    connect(createButton, &QPushButton::clicked, this, [onCreateMap]() {
        if (onCreateMap) {
            onCreateMap();
        }
    });
    // Conecta el boton de carga con la accion que busca un mapa existente.
    connect(loadButton, &QPushButton::clicked, this, [onLoadMap]() {
        if (onLoadMap) {
            onLoadMap();
        }
    });
    // Ejecuta la accion de cierre provista por la ventana principal.
    connect(exitButton, &QPushButton::clicked, this, [onExit]() {
        if (onExit) {
            onExit();
        }
    });
}