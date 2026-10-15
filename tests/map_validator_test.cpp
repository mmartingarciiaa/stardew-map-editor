#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "map/persistence/TomlMapFile.h"
#include "map/persistence/MapRepository.h"
#include "map/persistence/MapRequirementsLoader.h"
#include "map/service/MapService.h"
#include "map/validation/MapValidator.h"

namespace {

using stardew::maps::AreaType;
using stardew::maps::ElementType;
using stardew::maps::ElementLayer;
using stardew::maps::MapArea;
using stardew::maps::MapDefinition;
using stardew::maps::MapElement;
using stardew::maps::MapRequirements;
using stardew::maps::MapTransition;
using stardew::maps::MapIssueCode;
using stardew::maps::MapValidationIssue;

MapArea makeArea(const std::string& id, AreaType type, int width, int height) {
    MapArea area;
    area.id = id;
    area.type = type;
    area.width = width;
    area.height = height;
    area.tiles = std::vector<std::vector<stardew::maps::TileType>>(
        static_cast<std::size_t>(height),
        std::vector<stardew::maps::TileType>(
            static_cast<std::size_t>(width), stardew::maps::TileType::Grass));
    return area;
}

void addElement(
    MapDefinition& map,
    const std::string& id,
    const std::string& areaId,
    ElementType type,
    ElementLayer layer,
    int x,
    int y,
    int width = 1,
    int height = 1) {
    map.elements.push_back({id, areaId, type, layer, x, y, width, height});
}

// Agrega una conexion dirigida desde un elemento de salida hacia una celda de llegada.
void addTransition(
    MapDefinition& map,
    const std::string& id,
    const std::string& fromAreaId,
    const std::string& triggerElementId,
    const std::string& toAreaId,
    int targetX,
    int targetY) {
    map.transitions.push_back(
        {id, fromAreaId, triggerElementId, toAreaId, targetX, targetY});
}

MapDefinition makeValidMap() {
    MapDefinition map;
    map.formatVersion = 1;
    map.name = "Test world";
    map.areas.push_back(makeArea("farm", AreaType::Farm, 8, 8));
    map.areas.push_back(makeArea("village", AreaType::Village, 8, 8));
    addElement(map, "cabin", "farm", ElementType::Cabin, ElementLayer::Structure, 0, 0);
    addElement(map, "market", "village", ElementType::Market, ElementLayer::Structure, 0, 0);
    addElement(
        map,
        "community-center",
        "village",
        ElementType::CommunityCenter,
        ElementLayer::Structure,
        2,
        0);
    addElement(
        map,
        "mine-entrance",
        "village",
        ElementType::DungeonEntrance,
        ElementLayer::Structure,
        4,
        0);
    addElement(map, "farm-gate", "farm", ElementType::Door, ElementLayer::Structure, 7, 0);
    addElement(
        map, "village-farm-gate", "village", ElementType::Door, ElementLayer::Structure, 6, 0);
    addElement(map, "marnie", "village", ElementType::MerchantMarnie, ElementLayer::Npc, 0, 2);
    addElement(map, "clint", "village", ElementType::MerchantClint, ElementLayer::Npc, 1, 2);
    addElement(map, "pierre", "village", ElementType::MerchantPierre, ElementLayer::Npc, 2, 2);
    addElement(map, "robin", "village", ElementType::MerchantRobin, ElementLayer::Npc, 3, 2);
    for (int floor = 1; floor <= 10; ++floor) {
        auto area = makeArea(
            "dungeon-" + std::to_string(floor), AreaType::DungeonFloor, 8, 8);
        area.floorNumber = floor;
        map.areas.push_back(std::move(area));

        const auto floorId = "dungeon-" + std::to_string(floor);
        const auto exitId = "dungeon-exit-" + std::to_string(floor);
        addElement(map, exitId, floorId, ElementType::Door, ElementLayer::Structure, 0, 0);
        addTransition(map, exitId, floorId, exitId, "village", 6, 1);

        if (floor < 10) {
            const auto stairsId = "dungeon-stairs-" + std::to_string(floor);
            addElement(map, stairsId, floorId, ElementType::Stairs, ElementLayer::Structure, 2, 0);
            addTransition(
                map,
                stairsId,
                floorId,
                stairsId,
                "dungeon-" + std::to_string(floor + 1),
                0,
                0);
        }
    }

    // Cada area puede salir; las rutas a pisos siguientes son unidireccionales.
    addTransition(map, "farm-to-village", "farm", "farm-gate", "village", 6, 1);
    addTransition(
        map, "village-to-farm", "village", "village-farm-gate", "farm", 7, 0);
    addTransition(
        map, "village-to-dungeon", "village", "mine-entrance", "dungeon-1", 0, 0);
    return map;
}

bool expectIssues(
    const std::string& testName,
    const MapDefinition& map,
    const MapRequirements& requirements,
    const std::vector<MapValidationIssue>& expected) {
    const auto actual = stardew::maps::validateMap(map, requirements);
    if (actual == expected) {
        return true;
    }

    std::cerr << testName << " returned unexpected issues\nExpected codes:\n";
    for (const auto& error : expected) {
        std::cerr << "  " << stardew::maps::toString(error.code) << '\n';
    }
    std::cerr << "Actual codes:\n";
    for (const auto& error : actual) {
        std::cerr << "  " << stardew::maps::toString(error.code) << '\n';
    }
    return false;
}

bool containsIssue(
    const std::vector<MapValidationIssue>& issues,
    const MapValidationIssue& expected) {
    return std::find(issues.begin(), issues.end(), expected) != issues.end();
}

std::filesystem::path temporaryMapPath();

bool testMapTomlFixtures(
    const std::string& validPath,
    const std::string& incompletePath,
    const MapRequirements& requirements) {
    stardew::maps::TomlMapRepository repository;
    stardew::maps::MapService service(repository, requirements);
    const auto rejectedSavePath = temporaryMapPath();
    const auto savedMapPath = temporaryMapPath();
    try {
        const auto validResult = service.loadMap(validPath);
        if (!validResult.issues.empty()) {
            std::cerr << "valid TOML map produced validation issues: " << validPath << '\n';
            return false;
        }

        const auto incompleteResult = service.loadMap(incompletePath);
        const std::vector<MapValidationIssue> expected{
            {.code = MapIssueCode::AreaCountMismatch,
             .areaType = AreaType::Village,
             .expected = 1,
             .actual = 0},
            {.code = MapIssueCode::AreaCountMismatch,
             .areaType = AreaType::DungeonFloor,
             .expected = 10,
             .actual = 0},
            {.code = MapIssueCode::MissingRequiredElement,
             .areaType = AreaType::Farm,
             .elementType = ElementType::Cabin,
             .expectedLayer = ElementLayer::Structure},
            {.code = MapIssueCode::AreaWithoutOutgoingTransition, .areaId = "farm"},
            {.code = MapIssueCode::MissingRequiredTransition,
             .areaId = "farm",
             .targetAreaType = AreaType::Village},
        };
        const bool hasExpectedIssues = std::all_of(
            expected.begin(), expected.end(), [&incompleteResult](const auto& item) {
                return containsIssue(incompleteResult.issues, item);
        });
        if (!hasExpectedIssues) {
            std::cerr << "incomplete TOML fixture did not report expected issues: "
                      << incompletePath << '\n';
            return false;
        }

        std::filesystem::remove(rejectedSavePath);
        const auto rejectedSaveIssues = service.saveMap(rejectedSavePath.string(), incompleteResult.map);
        if (rejectedSaveIssues.empty() || std::filesystem::exists(rejectedSavePath)) {
            std::cerr << "invalid map was saved instead of being rejected\n";
            return false;
        }

        const auto saveIssues = service.saveMap(savedMapPath.string(), validResult.map);
        if (!saveIssues.empty() || !std::filesystem::exists(savedMapPath)) {
            std::cerr << "valid map was not saved\n";
            return false;
        }
        const auto reloadedSavedMap = service.loadMap(savedMapPath.string());
        std::filesystem::remove(savedMapPath);
        if (!reloadedSavedMap.issues.empty() || reloadedSavedMap.map != validResult.map) {
            std::cerr << "saved map could not be reloaded unchanged\n";
            return false;
        }
        return true;
    } catch (const std::exception& error) {
        std::filesystem::remove(rejectedSavePath);
        std::filesystem::remove(savedMapPath);
        std::cerr << "failed to load TOML map fixture: " << error.what() << '\n';
        return false;
    }
}

bool testValidMap(const MapRequirements& requirements) {
    return expectIssues("valid map", makeValidMap(), requirements, {});
}

bool testMissingRequirements(const MapRequirements& requirements) {
    auto map = makeValidMap();
    map.elements.erase(
        std::remove_if(map.elements.begin(), map.elements.end(), [](const MapElement& element) {
            return element.id == "cabin" || element.id == "marnie";
        }),
        map.elements.end());
    return expectIssues(
        "missing requirements",
        map,
        requirements,
                {{.code = MapIssueCode::MissingRequiredElement,
                    .areaType = AreaType::Farm,
                    .elementType = ElementType::Cabin,
                    .expectedLayer = ElementLayer::Structure},
                 {.code = MapIssueCode::MissingRequiredElement,
                    .areaType = AreaType::Village,
                    .elementType = ElementType::MerchantMarnie,
                    .expectedLayer = ElementLayer::Npc}});
}

bool testMissingDungeonFloor(const MapRequirements& requirements) {
    auto map = makeValidMap();
    map.areas.erase(
        std::remove_if(map.areas.begin(), map.areas.end(), [](const MapArea& area) {
            return area.id == "dungeon-4";
        }),
        map.areas.end());
    map.elements.erase(
        std::remove_if(map.elements.begin(), map.elements.end(), [](const MapElement& element) {
            return element.areaId == "dungeon-4";
        }),
        map.elements.end());
    map.transitions.erase(
        std::remove_if(map.transitions.begin(), map.transitions.end(), [](const MapTransition& route) {
            return route.fromAreaId == "dungeon-4" || route.toAreaId == "dungeon-4";
        }),
        map.transitions.end());

    return expectIssues(
        "missing dungeon floor",
        map,
        requirements,
                {{.code = MapIssueCode::AreaCountMismatch,
                    .areaType = AreaType::DungeonFloor,
                    .expected = 10,
                    .actual = 9},
                 {.code = MapIssueCode::MissingRequiredFloorNumber,
                    .areaType = AreaType::DungeonFloor,
                    .floorNumber = 4},
                 {.code = MapIssueCode::MissingRequiredTransition,
                    .areaId = "dungeon-3",
                    .targetAreaType = AreaType::DungeonFloor,
                    .targetFloor = 4}});
}

bool testGridAndElementBounds(const MapRequirements& requirements) {
    auto map = makeValidMap();
    map.areas.front().tiles.front().pop_back();
    map.elements.front().x = 7;
    map.elements.front().width = 2;

    return expectIssues(
        "grid dimensions and element bounds",
        map,
        requirements,
                {{.code = MapIssueCode::TileGridWidthMismatch, .areaId = "farm"},
                 {.code = MapIssueCode::ElementOutsideAreaBounds,
                    .areaId = "farm",
                    .elementId = "cabin"},
                 {.code = MapIssueCode::MissingRequiredElement,
                    .areaType = AreaType::Farm,
                    .elementType = ElementType::Cabin,
                    .expectedLayer = ElementLayer::Structure}});
}

bool testOverlappingStructures(const MapRequirements& requirements) {
    auto map = makeValidMap();
    addElement(map, "barn", "farm", ElementType::Decoration, ElementLayer::Structure, 2, 2, 2, 2);
    addElement(map, "shed", "farm", ElementType::Decoration, ElementLayer::Structure, 3, 2, 2, 2);

    return expectIssues(
        "overlapping structures",
        map,
        requirements,
                {{.code = MapIssueCode::StructureOverlap,
                    .areaId = "farm",
                    .elementId = "barn",
                    .relatedElementId = "shed"}});
}

bool testDuplicateAreaId(const MapRequirements& requirements) {
    auto map = makeValidMap();
    map.areas.push_back(map.areas.front());
    return expectIssues(
        "duplicate area ID",
        map,
        requirements,
                {{.code = MapIssueCode::DuplicateAreaId, .areaId = "farm"},
                 {.code = MapIssueCode::AreaCountMismatch,
                    .areaType = AreaType::Farm,
                    .expected = 1,
                    .actual = 2}});
}

bool testDuplicateElementId(const MapRequirements& requirements) {
    auto map = makeValidMap();
    addElement(map, "cabin", "farm", ElementType::Rock, ElementLayer::Object, 3, 3);
    return expectIssues(
        "duplicate element ID",
        map,
        requirements,
        {{.code = MapIssueCode::DuplicateElementId, .elementId = "cabin"}});
}

bool testDuplicateFloorNumber(const MapRequirements& requirements) {
    auto map = makeValidMap();
    map.areas[3].floorNumber = 1;
    return expectIssues(
        "duplicate dungeon floor",
        map,
        requirements,
        {{.code = MapIssueCode::DuplicateFloorNumber,
          .areaId = "dungeon-2",
          .areaType = AreaType::DungeonFloor,
          .floorNumber = 1},
         {.code = MapIssueCode::MissingRequiredFloorNumber,
          .areaType = AreaType::DungeonFloor,
          .floorNumber = 2},
         {.code = MapIssueCode::MissingRequiredTransition,
          .areaId = "dungeon-1",
          .targetAreaType = AreaType::DungeonFloor,
          .targetFloor = 2},
         {.code = MapIssueCode::MissingRequiredTransition,
          .areaId = "dungeon-2",
          .targetAreaType = AreaType::DungeonFloor,
          .targetFloor = 2}});
}

bool testIncorrectRequiredLayer(const MapRequirements& requirements) {
    auto map = makeValidMap();
    const auto robin = std::find_if(map.elements.begin(), map.elements.end(), [](const MapElement& element) {
        return element.id == "robin";
    });
    robin->layer = ElementLayer::Structure;
    return expectIssues(
        "incorrect required element layer",
        map,
        requirements,
        {{.code = MapIssueCode::RequiredElementWrongLayer,
          .areaId = "village",
          .elementId = "robin",
          .areaType = AreaType::Village,
          .elementType = ElementType::MerchantRobin,
          .expectedLayer = ElementLayer::Npc,
          .actualLayer = ElementLayer::Structure}});
}

bool testMissingRequiredFields(const MapRequirements& requirements) {
    auto map = makeValidMap();
    map.formatVersion.reset();
    map.areas.front().type.reset();
    return expectIssues(
        "missing required fields",
        map,
        requirements,
        {{.code = MapIssueCode::MissingFormatVersion},
         {.code = MapIssueCode::MissingAreaType, .areaId = "farm"},
         {.code = MapIssueCode::AreaCountMismatch,
          .areaType = AreaType::Farm,
          .expected = 1,
          .actual = 0},
         {.code = MapIssueCode::MissingRequiredTransition,
          .areaId = "village",
          .targetAreaType = AreaType::Farm}});
}

bool testMissingElementTypeAndLayer(const MapRequirements& requirements) {
    auto map = makeValidMap();
    map.elements.front().type.reset();
    map.elements.front().layer.reset();
    return expectIssues(
        "missing element type and layer",
        map,
        requirements,
        {{.code = MapIssueCode::MissingElementType, .elementId = "cabin"},
         {.code = MapIssueCode::MissingElementLayer, .elementId = "cabin"},
         {.code = MapIssueCode::MissingRequiredElement,
          .areaType = AreaType::Farm,
          .elementType = ElementType::Cabin,
          .expectedLayer = ElementLayer::Structure}});
}

bool testElementTypeNames() {
    const bool passed = stardew::maps::elementTypeFromString("cabin") == ElementType::Cabin &&
                        !stardew::maps::elementTypeFromString("cabinn").has_value();
    if (!passed) {
        std::cerr << "element type conversion accepted or rejected an unexpected name\n";
    }
    return passed;
}

std::filesystem::path temporaryMapPath() {
    const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           ("stardew-map-test-" + std::to_string(timestamp) + ".toml");
}

bool testTomlMapFileRoundTrip(const MapRequirements& requirements) {
    const auto path = temporaryMapPath();
    const auto expected = makeValidMap();
    try {
        stardew::maps::saveTomlMapDefinition(path.string(), expected);
        const auto actual = stardew::maps::loadTomlMapDefinition(path.string());
        std::filesystem::remove(path);
        if (actual != expected) {
            std::cerr << "map TOML round-trip changed map data\n";
            return false;
        }
        if (!stardew::maps::validateMap(actual, requirements).empty()) {
            std::cerr << "map loaded from TOML did not pass validation\n";
            return false;
        }
        return true;
    } catch (const std::exception& error) {
        std::filesystem::remove(path);
        std::cerr << "map TOML round-trip failed: " << error.what() << '\n';
        return false;
    }
}

bool testUnknownTileRejected() {
    const auto path = temporaryMapPath();
    {
        std::ofstream output(path);
        output << "name = \"Broken map\"\n"
                  "[[areas]]\n"
              "id = \"farm\"\n"
              "type = \"farm\"\n"
                  "width = 1\n"
                  "height = 1\n"
              "tiles = [[\"gras\"]]\n";
    }
    try {
        static_cast<void>(stardew::maps::loadTomlMapDefinition(path.string()));
    } catch (const std::runtime_error&) {
        std::filesystem::remove(path);
        return true;
    }
    std::filesystem::remove(path);
    std::cerr << "unknown tile ID was accepted\n";
    return false;
}

bool testUnknownElementTypeRejected() {
    const auto path = temporaryMapPath();
    {
        std::ofstream output(path);
        output << "name = \"Unknown element\"\n"
                  "[[areas]]\n"
                  "id = \"farm\"\n"
                  "type = \"farm\"\n"
                  "width = 1\n"
                  "height = 1\n"
                  "tiles = [[\"grass\"]]\n"
                  "[[elements]]\n"
                  "id = \"cabin\"\n"
                  "area = \"farm\"\n"
                  "type = \"cabinn\"\n"
                  "layer = \"structure\"\n"
                  "x = 0\n"
                  "y = 0\n"
                  "width = 1\n"
                  "height = 1\n";
    }
    try {
        static_cast<void>(stardew::maps::loadTomlMapDefinition(path.string()));
    } catch (const std::runtime_error&) {
        std::filesystem::remove(path);
        return true;
    }
    std::filesystem::remove(path);
    std::cerr << "unknown element type was accepted\n";
    return false;
}

bool testMissingTypedFieldsRemainDetectable() {
    const auto path = temporaryMapPath();
    {
        std::ofstream output(path);
        output << "name = \"Incomplete map\"\n"
                  "[[areas]]\n"
              "id = \"farm\"\n"
                  "width = 1\n"
                  "height = 1\n"
              "tiles = [[\"grass\"]]\n";
    }
    try {
        const auto map = stardew::maps::loadTomlMapDefinition(path.string());
        std::filesystem::remove(path);
        return !map.formatVersion.has_value() && !map.areas.front().type.has_value();
    } catch (const std::exception& error) {
        std::filesystem::remove(path);
        std::cerr << "missing semantic fields were rejected before validation: " << error.what()
                  << '\n';
        return false;
    }
}

bool testAreaMustHaveOutgoingTransition(const MapRequirements& requirements) {
    auto map = makeValidMap();
    map.transitions.erase(
        std::remove_if(map.transitions.begin(), map.transitions.end(), [](const MapTransition& route) {
            return route.id == "farm-to-village";
        }),
        map.transitions.end());
    return expectIssues(
        "area without an outgoing transition",
        map,
        requirements,
        {{.code = MapIssueCode::AreaWithoutOutgoingTransition, .areaId = "farm"},
         {.code = MapIssueCode::MissingRequiredTransition,
          .areaId = "farm",
          .targetAreaType = AreaType::Village}});
}

bool testTransitionArrivalBounds(const MapRequirements& requirements) {
    auto map = makeValidMap();
    const auto route = std::find_if(
        map.transitions.begin(), map.transitions.end(), [](const MapTransition& transition) {
            return transition.id == "farm-to-village";
        });
    route->targetX = 8;
    return expectIssues(
        "transition target outside area",
        map,
        requirements,
        {{.code = MapIssueCode::TransitionArrivalOutsideArea,
          .areaId = "village",
          .transitionId = "farm-to-village",
          .x = 8,
          .y = 1},
         {.code = MapIssueCode::AreaWithoutOutgoingTransition, .areaId = "farm"},
         {.code = MapIssueCode::MissingRequiredTransition,
          .areaId = "farm",
          .targetAreaType = AreaType::Village}});
}

void printIssue(const MapValidationIssue& issue) {
    std::cout << "  - code: " << stardew::maps::toString(issue.code) << '\n';
    if (issue.areaId.has_value()) std::cout << "    area_id: " << *issue.areaId << '\n';
    if (issue.elementId.has_value()) std::cout << "    element_id: " << *issue.elementId << '\n';
    if (issue.areaType.has_value()) {
        std::cout << "    area_type: " << stardew::maps::toString(*issue.areaType) << '\n';
    }
    if (issue.elementType.has_value()) {
        std::cout << "    element_type: " << stardew::maps::toString(*issue.elementType) << '\n';
    }
    if (issue.expectedLayer.has_value()) {
        std::cout << "    expected_layer: " << stardew::maps::toString(*issue.expectedLayer) << '\n';
    }
    if (issue.actualLayer.has_value()) {
        std::cout << "    actual_layer: " << stardew::maps::toString(*issue.actualLayer) << '\n';
    }
}

void printValidatorPreview(const MapRequirements& requirements) {
    // Este ejemplo muestra el mapa C++ que se entrega al validador, no un mensaje de red.
    auto map = makeValidMap();
    map.elements.erase(
        std::remove_if(map.elements.begin(), map.elements.end(), [](const MapElement& element) {
            return element.id == "cabin";
        }),
        map.elements.end());
    const auto robin = std::find_if(map.elements.begin(), map.elements.end(), [](const MapElement& element) {
        return element.id == "robin";
    });
    robin->layer = ElementLayer::Structure;

    const auto issues = stardew::maps::validateMap(map, requirements);
    std::cout << "Validator preview (debug representation, not a wire protocol)\n"
              << "Call: validateMap(const MapDefinition&, const MapRequirements&)\n"
              << "MapDefinition: name=" << map.name << ", areas=" << map.areas.size()
              << ", elements=" << map.elements.size() << ", transitions=" << map.transitions.size()
              << '\n'
              << "MapRequirements: areas=" << requirements.areas.size()
              << ", required_elements=" << requirements.elements.size()
              << ", required_transitions=" << requirements.transitions.size() << '\n'
              << "Returned issues:\n";
    for (const auto& issue : issues) {
        printIssue(issue);
    }
    std::cout << '\n';
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 5) {
        std::cerr << "usage: map-validator-tests <requirements.toml> "
                     "[<valid-map.toml> <incomplete-map.toml>] [--show-issues]\n";
        return 2;
    }

    int argumentIndex = 2;
    bool showIssues = false;
    if (argumentIndex < argc && std::string_view(argv[argumentIndex]) == "--show-issues") {
        showIssues = true;
        ++argumentIndex;
    }
    const std::string validMapPath = argumentIndex < argc ? argv[argumentIndex++] : "";
    const std::string incompleteMapPath = argumentIndex < argc ? argv[argumentIndex++] : "";
    if (argumentIndex < argc && std::string_view(argv[argumentIndex]) == "--show-issues") {
        showIssues = true;
        ++argumentIndex;
    }
    if (argumentIndex != argc || validMapPath.empty() != incompleteMapPath.empty()) {
        std::cerr << "usage: map-validator-tests <requirements.toml> "
                     "[<valid-map.toml> <incomplete-map.toml>] [--show-issues]\n";
        return 2;
    }

    try {
        const auto requirements = stardew::maps::loadMapRequirements(argv[1]);
        if (showIssues) {
            printValidatorPreview(requirements);
        }
        bool passed = true;
        if (!validMapPath.empty()) {
            passed = testMapTomlFixtures(validMapPath, incompleteMapPath, requirements) && passed;
        }
        passed = testValidMap(requirements) && passed;
        passed = testMissingRequirements(requirements) && passed;
        passed = testMissingDungeonFloor(requirements) && passed;
        passed = testGridAndElementBounds(requirements) && passed;
        passed = testOverlappingStructures(requirements) && passed;
        passed = testDuplicateAreaId(requirements) && passed;
        passed = testDuplicateElementId(requirements) && passed;
        passed = testDuplicateFloorNumber(requirements) && passed;
        passed = testIncorrectRequiredLayer(requirements) && passed;
        passed = testMissingRequiredFields(requirements) && passed;
        passed = testMissingElementTypeAndLayer(requirements) && passed;
        passed = testElementTypeNames() && passed;
        passed = testTomlMapFileRoundTrip(requirements) && passed;
        passed = testUnknownTileRejected() && passed;
        passed = testUnknownElementTypeRejected() && passed;
        passed = testMissingTypedFieldsRemainDetectable() && passed;
        passed = testAreaMustHaveOutgoingTransition(requirements) && passed;
        passed = testTransitionArrivalBounds(requirements) && passed;
        if (passed) {
            std::cout << "All map validator tests passed\n";
            return 0;
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
    }
    return 1;
}