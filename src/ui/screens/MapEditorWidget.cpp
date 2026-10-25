#include "ui/screens/MapEditorWidget.h"

#include <QApplication>
#include <QColor>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>

namespace {

QString formatIssue(const stardew::maps::MapValidationIssue& issue) {
    QString text = QString::fromStdString(std::string(stardew::maps::toString(issue.code)));
    if (issue.areaId.has_value()) {
        text += " | area=" + QString::fromStdString(*issue.areaId);
    }
    if (issue.elementId.has_value()) {
        text += " | element=" + QString::fromStdString(*issue.elementId);
    }
    if (issue.relatedElementId.has_value()) {
        text += " | related=" + QString::fromStdString(*issue.relatedElementId);
    }
    return text;
}

}  // namespace

namespace {

struct TilePaletteEntry {
    stardew::maps::TileType type;
    const char* name;
    QColor color;
};

std::vector<TilePaletteEntry> tilePalette() {
    return {
        {stardew::maps::TileType::Grass, "grass", QColor(111, 169, 90)},
        {stardew::maps::TileType::Dirt, "dirt", QColor(140, 95, 55)},
        {stardew::maps::TileType::TilledSoil, "tilled_soil", QColor(179, 139, 80)},
        {stardew::maps::TileType::Water, "water", QColor(62, 125, 196)},
        {stardew::maps::TileType::Path, "path", QColor(196, 173, 112)},
        {stardew::maps::TileType::Stone, "stone", QColor(133, 141, 151)},
        {stardew::maps::TileType::CaveFloor, "cave_floor", QColor(109, 96, 94)},
        {stardew::maps::TileType::CaveWall, "cave_wall", QColor(70, 64, 67)},
        {stardew::maps::TileType::WoodFloor, "wood_floor", QColor(160, 118, 79)},
        {stardew::maps::TileType::Sand, "sand", QColor(210, 191, 120)},
    };
}

QColor tileColor(stardew::maps::TileType tile) {
    for (const auto& entry : tilePalette()) {
        if (entry.type == tile) {
            return entry.color;
        }
    }
    return QColor(Qt::white);
}

struct ElementPaletteEntry {
    stardew::maps::ElementType type;
    QColor color;
    const char* marker;
};

std::vector<ElementPaletteEntry> elementPalette() {
    using stardew::maps::ElementType;
    return {
        {ElementType::Cabin, QColor(153, 91, 57), "CA"},
        {ElementType::Market, QColor(217, 155, 48), "MK"},
        {ElementType::CommunityCenter, QColor(112, 103, 147), "CC"},
        {ElementType::DungeonEntrance, QColor(86, 79, 96), "DE"},
        {ElementType::MerchantMarnie, QColor(195, 92, 99), "MA"},
        {ElementType::MerchantClint, QColor(88, 121, 165), "CL"},
        {ElementType::MerchantPierre, QColor(65, 133, 105), "PI"},
        {ElementType::MerchantRobin, QColor(179, 104, 69), "RO"},
        {ElementType::Rock, QColor(112, 121, 128), "RK"},
        {ElementType::OreNode, QColor(55, 133, 139), "OR"},
        {ElementType::Chest, QColor(174, 129, 43), "CH"},
        {ElementType::Decoration, QColor(190, 105, 133), "DC"},
        {ElementType::Door, QColor(117, 82, 59), "DR"},
        {ElementType::Stairs, QColor(129, 113, 83), "ST"},
        {ElementType::SpawnPoint, QColor(207, 213, 132), "SP"},
    };
}

QColor elementColor(stardew::maps::ElementType type) {
    for (const auto& entry : elementPalette()) {
        if (entry.type == type) {
            return entry.color;
        }
    }
    return QColor(Qt::white);
}

const char* elementMarker(stardew::maps::ElementType type) {
    for (const auto& entry : elementPalette()) {
        if (entry.type == type) {
            return entry.marker;
        }
    }
    return "?";
}

QString paletteButtonStyle(const QColor& color) {
    const auto foreground = color.lightness() > 155 ? "#202820" : "#ffffff";
    const auto background = color.name();
    return QString(
        "QPushButton {"
        "  background-color: %1;"
        "  border: 1px solid #4d5d53;"
        "  border-radius: 4px;"
        "  padding: 6px 8px;"
        "  text-align: left;"
        "  color: %2;"
        "}"
        "QPushButton:checked {"
        "  background-color: %1;"
        "  border: 2px solid #ffffff;"
        "}")
        .arg(background, foreground);
}

}  // namespace

MapEditorWidget::MapEditorWidget(
    stardew::maps::MapService& service,
    std::function<void()> onBackToMenu,
    QWidget* parent)
    : QWidget(parent),
      service_(service),
      onBackToMenu_(std::move(onBackToMenu)) {
    // El editor arranca con un mundo simple de muestra para poder pintar y probar la
    // edicion sin depender de un archivo cargado aun.
    initializeDefaultWorld();
    buildLayout();
    refreshGrid();
}

void MapEditorWidget::initializeDefaultWorld() {
    editorState_.makeDefaultWorld();
    editorState_.setActiveArea("farm");
}

void MapEditorWidget::buildLayout() {
    auto* rootLayout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout;
    auto* saveButton = new QPushButton("Guardar", this);
    auto* loadButton = new QPushButton("Cargar", this);
    auto* undoButton = new QPushButton("Deshacer", this);
    auto* redoButton = new QPushButton("Rehacer", this);
    auto* closeButton = new QPushButton("Cerrar", this);

    toolbar->addWidget(saveButton);
    toolbar->addWidget(loadButton);
    toolbar->addWidget(undoButton);
    toolbar->addWidget(redoButton);
    toolbar->addStretch();
    toolbar->addWidget(closeButton);
    rootLayout->addLayout(toolbar);

    auto* contentLayout = new QHBoxLayout;

    auto* sidebarScroll = new QScrollArea(this);
    sidebarScroll->setWidgetResizable(true);
    sidebarScroll->setMinimumWidth(205);
    sidebarScroll->setMaximumWidth(260);
    auto* sidebar = new QWidget(sidebarScroll);
    auto* sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(8, 8, 8, 8);
    sidebarLayout->setSpacing(4);

    const auto addSectionHeader = [sidebar, sidebarLayout](const char* title) {
        auto* label = new QLabel(title, sidebar);
        label->setStyleSheet(
            "QLabel { color: #f0f0f0; font-weight: 600; padding-top: 8px; "
            "padding-bottom: 3px; border-bottom: 1px solid #4d5d53; }");
        sidebarLayout->addWidget(label);
    };

    addSectionHeader("PISOS");
    tileButtonGroup_ = new QButtonGroup(this);
    tileButtonGroup_->setExclusive(true);

    const auto allTiles = stardew::maps::allTileTypes();
    for (const auto tile : allTiles) {
        const auto name = std::string(stardew::maps::toString(tile));
        auto* button = new QPushButton(QString::fromStdString(name), sidebar);
        button->setCheckable(true);
        button->setAutoExclusive(true);
        button->setStyleSheet(paletteButtonStyle(tileColor(tile)));
        tileButtonGroup_->addButton(button, static_cast<int>(tile));
        sidebarLayout->addWidget(button);

        connect(button, &QPushButton::clicked, this, [this, tile]() {
            editorState_.selectedTile = tile;
            paintingElements_ = false;
            updateTilePaletteSelection();
            updateElementPaletteSelection();
            updateStatusLabel();
        });
    }
    editorState_.selectedTile = allTiles.front();
    updateTilePaletteSelection();

    addSectionHeader("ELEMENTOS");
    elementButtonGroup_ = new QButtonGroup(this);
    elementButtonGroup_->setExclusive(true);
    const auto allElements = stardew::maps::allElementTypes();
    for (const auto type : allElements) {
        auto* button = new QPushButton(QString::fromStdString(std::string(stardew::maps::toString(type))), sidebar);
        button->setCheckable(true);
        button->setAutoExclusive(true);
        button->setStyleSheet(paletteButtonStyle(elementColor(type)));
        elementButtonGroup_->addButton(button, static_cast<int>(type));
        sidebarLayout->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, type]() {
            editorState_.selectedElement = type;
            paintingElements_ = true;
            updateElementPaletteSelection();
            updateTilePaletteSelection();
            updateStatusLabel();
        });
    }
    editorState_.selectedElement = allElements.front();
    updateElementPaletteSelection();

    addSectionHeader("ELEMENTO");
    elementXEdit_ = new QLineEdit("0", sidebar);
    elementYEdit_ = new QLineEdit("0", sidebar);
    elementWidthEdit_ = new QLineEdit("1", sidebar);
    elementHeightEdit_ = new QLineEdit("1", sidebar);
    for (auto* edit : {elementXEdit_, elementYEdit_, elementWidthEdit_, elementHeightEdit_}) {
        edit->setReadOnly(true);
    }

    auto* positionsLayout = new QVBoxLayout;
    const auto addField = [&](const char* label, QLineEdit* edit) {
        auto* row = new QWidget(sidebar);
        auto* rowLayout = new QHBoxLayout(row);
        auto* labelWidget = new QLabel(label, row);
        edit->setFixedWidth(80);
        rowLayout->addWidget(labelWidget);
        rowLayout->addWidget(edit);
        positionsLayout->addWidget(row);
    };
    addField("x", elementXEdit_);
    addField("y", elementYEdit_);
    addField("width", elementWidthEdit_);
    addField("height", elementHeightEdit_);
    sidebarLayout->addLayout(positionsLayout);

    addSectionHeader("TAMAÑO DEL MAPA");
    mapWidthSpin_ = new QSpinBox(sidebar);
    mapHeightSpin_ = new QSpinBox(sidebar);
    mapWidthSpin_->setRange(1, 500);
    mapHeightSpin_->setRange(1, 500);
    auto* mapSizeLayout = new QVBoxLayout;
    const auto addSpinField = [sidebar, mapSizeLayout](const char* label, QSpinBox* spinBox) {
        auto* row = new QWidget(sidebar);
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->addWidget(new QLabel(label, row));
        rowLayout->addWidget(spinBox);
        mapSizeLayout->addWidget(row);
    };
    addSpinField("width", mapWidthSpin_);
    addSpinField("height", mapHeightSpin_);
    auto* resizeMapButton = new QPushButton("Aplicar tamaño", sidebar);
    mapSizeLayout->addWidget(resizeMapButton);
    sidebarLayout->addLayout(mapSizeLayout);
    sidebarLayout->addStretch();

    sidebarScroll->setWidget(sidebar);

    connect(resizeMapButton, &QPushButton::clicked, this, [this]() {
        resizeActiveAreaFromControls();
    });

    auto* mainPanel = new QWidget(this);
    auto* mainPanelLayout = new QVBoxLayout(mainPanel);
    areaTabs_ = new QTabWidget(mainPanel);
    const std::vector<std::pair<const char*, const char*>> areas = {
        {"farm", "Granja"},
        {"village", "Pueblo"},
        {"dungeon", "Mazmorra"},
    };
    for (std::size_t index = 0; index < areas.size(); ++index) {
        auto* areaGrid = new QTableWidget(12, 12, areaTabs_);
        areaGrid->setEditTriggers(QAbstractItemView::NoEditTriggers);
        areaGrid->setSelectionMode(QAbstractItemView::SingleSelection);
        areaGrid->setSelectionBehavior(QAbstractItemView::SelectItems);
        areaGrid->horizontalHeader()->setVisible(false);
        areaGrid->verticalHeader()->setVisible(false);
        areaGrid->setShowGrid(true);
        areaGrid->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        areaGrid->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        areaGrid->setMouseTracking(true);
        areaGrid->viewport()->setMouseTracking(true);
        areaGrid->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        areaGrid->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        areaGrid->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        areaGrid->setStyleSheet("QTableWidget { gridline-color: rgba(0,0,0,80); } QTableWidget::item { padding: 0px; }");
        areaGrid->viewport()->installEventFilter(this);
        areaTabs_->addTab(areaGrid, areas[index].second);
        if (index == 0) {
            grid_ = areaGrid;
        }

        connect(areaGrid, &QTableWidget::cellPressed, this, [this](int row, int column) {
            handlePaintStart(row, column);
        });
        connect(areaGrid, &QTableWidget::cellEntered, this, [this](int row, int column) {
            if (isDragging_) {
                handlePaintMove(row, column);
            }
        });
    }
    mainPanelLayout->addWidget(areaTabs_);

    statusLabel_ = new QLabel("Estado: editando granja", this);
    mainPanelLayout->addWidget(statusLabel_);

    contentLayout->addWidget(sidebarScroll, 0);
    contentLayout->addWidget(mainPanel, 1);
    rootLayout->addLayout(contentLayout);

    connect(saveButton, &QPushButton::clicked, this, [this]() {
        const auto path = QFileDialog::getSaveFileName(
            this,
            "Guardar mapa",
            QString::fromStdString(currentFilePath_.empty() ? "map.toml" : currentFilePath_),
            "Archivos TOML (*.toml)");
        if (path.isEmpty()) {
            return;
        }
        saveMapToFile(path.toStdString());
    });
    connect(loadButton, &QPushButton::clicked, this, [this]() {
        const auto path = QFileDialog::getOpenFileName(
            this,
            "Cargar mapa",
            QString(),
            "Archivos TOML (*.toml)");
        if (path.isEmpty()) {
            return;
        }
        loadMapFromFile(path.toStdString());
    });
    connect(undoButton, &QPushButton::clicked, this, [this]() {
        editorState_.undoActiveArea();
        refreshGrid();
    });
    connect(redoButton, &QPushButton::clicked, this, [this]() {
        editorState_.redoActiveArea();
        refreshGrid();
    });
    connect(closeButton, &QPushButton::clicked, this, [this]() {
        applyCloseAction();
    });
    connect(areaTabs_, &QTabWidget::currentChanged, this, [this](int index) {
        grid_ = qobject_cast<QTableWidget*>(areaTabs_->widget(index));
        setActiveAreaByIndex(index);
    });
    refreshAreaTabs();
    updateStatusLabel();
}

void MapEditorWidget::refreshAreaTabs() {
    if (grid_ == nullptr) {
        return;
    }
    if (const auto* area = editorState_.activeArea()) {
        grid_->setRowCount(static_cast<int>(area->height));
        grid_->setColumnCount(static_cast<int>(area->width));
    }
    updateAreaSizeControls();
    refreshGrid();
}

void MapEditorWidget::refreshGrid() {
    if (grid_ == nullptr) {
        return;
    }
    const auto* area = editorState_.activeArea();
    if (area == nullptr) {
        return;
    }

    grid_->setRowCount(static_cast<int>(area->height));
    grid_->setColumnCount(static_cast<int>(area->width));
    for (int row = 0; row < area->height; ++row) {
        for (int column = 0; column < area->width; ++column) {
            auto* item = new QTableWidgetItem();
            const auto tile = area->tiles[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)];
            item->setBackground(tileColor(tile));
            item->setText("");
            grid_->setItem(row, column, item);
        }
    }

    for (const auto& element : editorState_.map.elements) {
        if (element.areaId != area->id || !element.type.has_value()) {
            continue;
        }
        for (int y = element.y; y < element.y + element.height; ++y) {
            for (int x = element.x; x < element.x + element.width; ++x) {
                if (y < 0 || y >= area->height || x < 0 || x >= area->width) {
                    continue;
                }
                auto* item = grid_->item(y, x);
                if (item == nullptr) {
                    continue;
                }
                const auto color = elementColor(*element.type);
                item->setBackground(color);
                item->setForeground(color.lightness() > 155 ? Qt::black : Qt::white);
                item->setText(x == element.x && y == element.y ? elementMarker(*element.type) : "");
                item->setTextAlignment(Qt::AlignCenter);
                item->setToolTip(QString::fromStdString(element.id));
            }
        }
    }
    updateStatusLabel();
}

bool MapEditorWidget::eventFilter(QObject* watched, QEvent* event) {
    if (grid_ != nullptr && watched == grid_->viewport() && isDragging_ &&
        event->type() == QEvent::MouseButtonRelease) {
        const auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            const auto index = grid_->indexAt(mouseEvent->pos());
            if (index.isValid()) {
                lastPaintRow_ = index.row();
                lastPaintColumn_ = index.column();
            }
            handlePaintEnd(lastPaintRow_, lastPaintColumn_);
        }
    }
    return QWidget::eventFilter(watched, event);
}

void MapEditorWidget::updateAreaSizeControls() {
    const auto* area = editorState_.activeArea();
    if (area == nullptr || mapWidthSpin_ == nullptr || mapHeightSpin_ == nullptr) {
        return;
    }
    mapWidthSpin_->setValue(area->width);
    mapHeightSpin_->setValue(area->height);
}

void MapEditorWidget::resizeActiveAreaFromControls() {
    if (mapWidthSpin_ == nullptr || mapHeightSpin_ == nullptr) {
        return;
    }
    editorState_.resizeActiveArea(mapWidthSpin_->value(), mapHeightSpin_->value());
    refreshGrid();
}

void MapEditorWidget::setActiveAreaByIndex(int index) {
    const std::array<std::string, 3> areas = {"farm", "village", "dungeon"};
    if (index >= 0 && index < static_cast<int>(areas.size())) {
        editorState_.setActiveArea(areas[static_cast<std::size_t>(index)]);
        refreshGrid();
        updateAreaSizeControls();
    }
}

void MapEditorWidget::paintCurrentTileAt(int row, int column) {
    if (row < 0 || column < 0) {
        return;
    }
    editorState_.applyTilePaint(column, row, editorState_.selectedTile);
    refreshGrid();
}

void MapEditorWidget::handlePaintStart(int row, int column) {
    if (row < 0 || column < 0) {
        return;
    }
    isDragging_ = true;
    placementStartRow_ = row;
    placementStartColumn_ = column;
    lastPaintRow_ = row;
    lastPaintColumn_ = column;
    if (paintingElements_) {
        updateElementPlacementBounds(row, column);
        return;
    }
    editorState_.beginAreaAction();
    paintCurrentTileAt(row, column);
}

void MapEditorWidget::handlePaintMove(int row, int column) {
    if (!isDragging_) {
        return;
    }
    lastPaintRow_ = row;
    lastPaintColumn_ = column;
    if (paintingElements_) {
        updateElementPlacementBounds(row, column);
        return;
    }
    paintCurrentTileAt(row, column);
}

void MapEditorWidget::handlePaintEnd(int row, int column) {
    if (!isDragging_) {
        return;
    }
    if (paintingElements_) {
        placeSelectedElement(row, column);
    } else {
        editorState_.endAreaAction();
    }
    isDragging_ = false;
}

void MapEditorWidget::updateElementPlacementBounds(int row, int column) {
    const auto left = std::min(placementStartColumn_, column);
    const auto top = std::min(placementStartRow_, row);
    if (elementXEdit_ != nullptr) elementXEdit_->setText(QString::number(left));
    if (elementYEdit_ != nullptr) elementYEdit_->setText(QString::number(top));
    if (elementWidthEdit_ != nullptr) {
        elementWidthEdit_->setText(QString::number(std::abs(column - placementStartColumn_) + 1));
    }
    if (elementHeightEdit_ != nullptr) {
        elementHeightEdit_->setText(QString::number(std::abs(row - placementStartRow_) + 1));
    }
}

void MapEditorWidget::placeSelectedElement(int row, int column) {
    if (!editorState_.selectedElement.has_value()) {
        return;
    }
    const auto* area = editorState_.activeArea();
    if (area == nullptr) {
        return;
    }
    const auto x = std::min(placementStartColumn_, column);
    const auto y = std::min(placementStartRow_, row);
    stardew::maps::MapElement element;
    element.id = std::string(stardew::maps::toString(*editorState_.selectedElement)) + "_" +
        std::to_string(editorState_.map.elements.size());
    element.areaId = area->id;
    element.type = *editorState_.selectedElement;
    element.layer = stardew::maps::elementLayerFor(*editorState_.selectedElement);
    element.x = x;
    element.y = y;
    element.width = std::abs(column - placementStartColumn_) + 1;
    element.height = std::abs(row - placementStartRow_) + 1;
    editorState_.map.elements.push_back(element);
    editorState_.dirty = true;
    updateElementPlacementBounds(row, column);
    refreshGrid();
}

void MapEditorWidget::updateStatusLabel() {
    if (statusLabel_ == nullptr) {
        return;
    }
    const auto* activeArea = editorState_.activeArea();
    if (activeArea == nullptr) {
        statusLabel_->setText("Estado: sin area activa");
        return;
    }
    const auto toolName = paintingElements_ && editorState_.selectedElement.has_value()
        ? QString("elemento=%1")
            .arg(QString::fromStdString(std::string(stardew::maps::toString(*editorState_.selectedElement))))
        : QString("tile=%1")
            .arg(QString::fromStdString(std::string(stardew::maps::toString(editorState_.selectedTile))));
    statusLabel_->setText(
        QString("Estado: %1 | %2 | %3x%4 | dirty=%5")
            .arg(QString::fromStdString(activeArea->id))
            .arg(toolName)
            .arg(activeArea->width)
            .arg(activeArea->height)
            .arg(editorState_.dirty ? "si" : "no"));
}

void MapEditorWidget::updateTilePaletteSelection() {
    if (tileButtonGroup_ == nullptr) {
        return;
    }
    for (auto* button : tileButtonGroup_->buttons()) {
        const auto id = tileButtonGroup_->id(button);
        const auto tile = static_cast<stardew::maps::TileType>(id);
        button->setChecked(!paintingElements_ && tile == editorState_.selectedTile);
    }
}

void MapEditorWidget::updateElementPaletteSelection() {
    if (elementButtonGroup_ == nullptr || !editorState_.selectedElement.has_value()) {
        return;
    }
    for (auto* button : elementButtonGroup_->buttons()) {
        const auto id = elementButtonGroup_->id(button);
        const auto type = static_cast<stardew::maps::ElementType>(id);
        button->setChecked(paintingElements_ && editorState_.selectedElement == type);
    }
}

void MapEditorWidget::applyCloseAction() {
    if (editorState_.dirty) {
        const auto answer = QMessageBox::question(
            this,
            "Guardar cambios",
            "Hay cambios sin guardar. ¿Deseas guardar antes de cerrar?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save);

        if (answer == QMessageBox::Cancel) {
            return;
        }
        if (answer == QMessageBox::Save) {
            const auto path = QFileDialog::getSaveFileName(
                this,
                "Guardar antes de cerrar",
                QString::fromStdString(currentFilePath_.empty() ? "map.toml" : currentFilePath_),
                "Archivos TOML (*.toml)");
            if (path.isEmpty()) {
                return;
            }
            saveMapToFile(path.toStdString());
            if (editorState_.dirty) {
                return;
            }
        }
    }
    QApplication::quit();
}

void MapEditorWidget::loadMapFromFile(const std::string& path) {
    try {
        const auto result = service_.loadMap(path);
        if (!result.issues.empty()) {
            QString details;
            for (const auto& issue : result.issues) {
                details += formatIssue(issue) + "\n";
            }
            QMessageBox::warning(this, "Mapa cargado con problemas", details);
        }

        editorState_.map = result.map;
        if (!editorState_.map.areas.empty()) {
            editorState_.setActiveArea(editorState_.map.areas.front().id);
        }
        currentFilePath_ = path;
        editorState_.dirty = false;
        refreshAreaTabs();
        updateStatusLabel();
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "No se pudo cargar", error.what());
    }
}

void MapEditorWidget::saveMapToFile(const std::string& path) {
    try {
        const auto issues = service_.saveMap(path, editorState_.map);
        if (!issues.empty()) {
            QString details;
            for (const auto& issue : issues) {
                details += formatIssue(issue) + "\n";
            }
            QMessageBox::warning(this, "El mapa no es valido", details);
            return;
        }
        currentFilePath_ = path;
        editorState_.dirty = false;
        QMessageBox::information(this, "Guardar", "Mapa guardado correctamente.");
        updateStatusLabel();
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "No se pudo guardar", error.what());
    }
}
