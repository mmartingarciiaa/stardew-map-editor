#include <QApplication>
#include <QMainWindow>

#include "map/persistence/MapRepository.h"
#include "map/persistence/MapRequirementsLoader.h"
#include "map/service/MapService.h"
#include "ui/screens/MapEditorWidget.h"
#include "ui/theme/StyleManager.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setStyleSheet(StyleManager::applicationStyleSheet());

    auto repository = std::make_unique<stardew::maps::TomlMapRepository>();
    auto requirements = stardew::maps::loadMapRequirements("config/map_requirements.toml");
    auto service = stardew::maps::MapService(*repository, std::move(requirements));

    QMainWindow window;
    window.setWindowTitle("Stardew Valley - Editor de mapas");
    window.resize(960, 640);

    auto* editor = new MapEditorWidget(service, []() {}, &window);
    window.setCentralWidget(editor);
    window.show();

    return app.exec();
}
