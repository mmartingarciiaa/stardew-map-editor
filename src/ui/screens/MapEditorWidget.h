#pragma once

#include <functional>

#include <QButtonGroup>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QTableWidget>
#include <QWidget>

#include "map/editor/MapEditorState.h"
#include "map/service/MapService.h"

class QTabWidget;

class MapEditorWidget : public QWidget {
public:
    explicit MapEditorWidget(
        stardew::maps::MapService& service,
        std::function<void()> onBackToMenu,
        QWidget* parent = nullptr);

    void loadMapFromFile(const std::string& path);
    void saveMapToFile(const std::string& path);

private:
    stardew::maps::MapService& service_;
    std::function<void()> onBackToMenu_;
    stardew::maps::MapEditorState editorState_;
    QButtonGroup* tileButtonGroup_ = nullptr;
    QButtonGroup* elementButtonGroup_ = nullptr;
    QLineEdit* elementXEdit_ = nullptr;
    QLineEdit* elementYEdit_ = nullptr;
    QLineEdit* elementWidthEdit_ = nullptr;
    QLineEdit* elementHeightEdit_ = nullptr;
    QSpinBox* mapWidthSpin_ = nullptr;
    QSpinBox* mapHeightSpin_ = nullptr;
    QTabWidget* areaTabs_ = nullptr;
    QTableWidget* grid_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    bool isDragging_ = false;
    bool paintingElements_ = false;
    int placementStartRow_ = 0;
    int placementStartColumn_ = 0;
    int lastPaintRow_ = 0;
    int lastPaintColumn_ = 0;
    std::string currentFilePath_;

    bool eventFilter(QObject* watched, QEvent* event) override;
    void initializeDefaultWorld();
    void buildLayout();
    void refreshGrid();
    void refreshAreaTabs();
    void updateAreaSizeControls();
    void resizeActiveAreaFromControls();
    void setActiveAreaByIndex(int index);
    void paintCurrentTileAt(int row, int column);
    void handlePaintStart(int row, int column);
    void handlePaintMove(int row, int column);
    void handlePaintEnd(int row, int column);
    void updateElementPlacementBounds(int row, int column);
    void placeSelectedElement(int row, int column);
    void updateStatusLabel();
    void updateTilePaletteSelection();
    void updateElementPaletteSelection();
    void applyCloseAction();
};
