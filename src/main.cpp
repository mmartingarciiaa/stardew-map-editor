#include <QApplication>
#include <QMessageBox>
#include <QMainWindow>

#include "ui/screens/MainMenuWidget.h"
#include "ui/theme/StyleManager.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setStyleSheet(StyleManager::applicationStyleSheet());

    QMainWindow window;
    window.setWindowTitle("Stardew Valley - Editor de mapas");
    window.resize(960, 640);

    auto* mainMenu = new MainMenuWidget(
        [&window]() {
            QMessageBox::information(
                &window,
                "Crear nuevo mapa",
                "La creacion de mapas todavia no esta implementada.");
        },
        [&window]() {
            QMessageBox::information(
                &window,
                "Cargar mapa",
                "La carga de mapas todavia no esta implementada.");
        });
    window.setCentralWidget(mainMenu);
    window.show();

    return app.exec();
}
