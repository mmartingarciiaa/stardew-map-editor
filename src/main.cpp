#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
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
            QString mapsDirectory = QDir(QCoreApplication::applicationDirPath())
                                        .absoluteFilePath("../maps");
            if (!QDir(mapsDirectory).exists()) {
                mapsDirectory = QDir(QCoreApplication::applicationDirPath())
                                    .absoluteFilePath("..");
            }

            const QString mapPath = QFileDialog::getOpenFileName(
                &window,
                "Seleccionar mapa",
                mapsDirectory,
                "Mapas TOML (*.toml)");
            if (mapPath.isEmpty()) {
                return;
            }

            QMessageBox::information(
                &window,
                "Mapa seleccionado",
                QString("Se selecciono el archivo:\n%1")
                    .arg(QFileInfo(mapPath).absoluteFilePath()));
        },
        [&window]() {
            window.close();
        });
    window.setCentralWidget(mainMenu);
    window.show();

    return app.exec();
}
