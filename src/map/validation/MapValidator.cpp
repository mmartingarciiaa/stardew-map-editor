#include "map/validation/MapValidator.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace stardew::maps {
namespace {

const AreaRequirement* findAreaRequirement(
    const MapRequirements& requirements,
    AreaType type) {
    for (const auto& requirement : requirements.areas) {
        if (requirement.type == type) {
            return &requirement;
        }
    }
    return nullptr;
}

// Comprueba si dos estructuras ocupan alguna de las mismas celdas.
bool rectanglesOverlap(const MapElement& first, const MapElement& second) {
    return first.x < second.x + second.width &&
           second.x < first.x + first.width &&
           first.y < second.y + second.height &&
           second.y < first.y + first.height;
}

}  // namespace

std::vector<MapValidationIssue> validateMap(
    const MapDefinition& map,
    const MapRequirements& requirements) {
    // Se acumulan errores para poder informar varios problemas en una sola validacion.
    std::vector<MapValidationIssue> errors;
    if (!map.formatVersion.has_value()) {
        errors.push_back({.code = MapIssueCode::MissingFormatVersion});
    } else if (*map.formatVersion != requirements.formatVersion) {
        errors.push_back({
            .code = MapIssueCode::UnsupportedFormatVersion,
            .expected = requirements.formatVersion,
            .actual = *map.formatVersion,
        });
    }
    if (map.name.empty()) {
        errors.push_back({.code = MapIssueCode::MissingMapName});
    }

    // Indexa las areas y cuenta los tipos y pisos declarados en los requisitos.
    std::set<std::string> areaIds;
    std::map<std::string, const MapArea*> areasById;
    std::map<std::string, AreaType> areaTypesById;
    std::map<std::string, std::optional<int>> areaFloorsById;
    std::map<AreaType, int> areaCounts;
    std::map<AreaType, std::set<int>> floorNumbersByType;

    for (const auto& area : map.areas) {
        if (area.id.empty()) {
            errors.push_back({.code = MapIssueCode::EmptyAreaId});
        } else if (!areaIds.insert(area.id).second) {
            errors.push_back({.code = MapIssueCode::DuplicateAreaId, .areaId = area.id});
        } else {
            areasById.emplace(area.id, &area);
        }

        if (!area.type.has_value()) {
            errors.push_back({.code = MapIssueCode::MissingAreaType, .areaId = area.id});
        } else {
            ++areaCounts[*area.type];
            if (!area.id.empty()) {
                areaTypesById.try_emplace(area.id, *area.type);
                areaFloorsById.try_emplace(area.id, area.floorNumber);
            }
        }

        // Comprueba que las dimensiones declaradas coincidan con la grilla de celdas.
        if (area.width <= 0 || area.height <= 0) {
            errors.push_back({.code = MapIssueCode::InvalidAreaDimensions, .areaId = area.id});
        }
        if (area.tiles.size() != static_cast<std::size_t>(area.height)) {
            errors.push_back({.code = MapIssueCode::TileGridHeightMismatch, .areaId = area.id});
        }
        for (const auto& row : area.tiles) {
            if (row.size() != static_cast<std::size_t>(area.width)) {
                errors.push_back({.code = MapIssueCode::TileGridWidthMismatch, .areaId = area.id});
                break;
            }
        }

        // Comprueba los numeros de piso usando las reglas configuradas para cada tipo de area.
        if (area.type.has_value()) {
            const auto* requirement = findAreaRequirement(requirements, *area.type);
            if (!area.floorNumber.has_value()) {
                if (requirement != nullptr && !requirement->floorNumbers.empty()) {
                    errors.push_back({
                        .code = MapIssueCode::MissingAreaFloorNumber,
                        .areaId = area.id,
                        .areaType = *area.type,
                    });
                }
            } else if (requirement == nullptr || requirement->floorNumbers.empty() ||
                       std::find(
                           requirement->floorNumbers.begin(),
                           requirement->floorNumbers.end(),
                           *area.floorNumber) == requirement->floorNumbers.end()) {
                errors.push_back({
                    .code = MapIssueCode::UnexpectedAreaFloorNumber,
                    .areaId = area.id,
                    .areaType = *area.type,
                    .floorNumber = *area.floorNumber,
                });
            } else if (!floorNumbersByType[*area.type].insert(*area.floorNumber).second) {
                errors.push_back({
                    .code = MapIssueCode::DuplicateFloorNumber,
                    .areaId = area.id,
                    .areaType = *area.type,
                    .floorNumber = *area.floorNumber,
                });
            }
        }
    }

    // Verifica que el mundo tenga todas las areas requeridas.
    for (const auto& requirement : requirements.areas) {
        const auto countIt = areaCounts.find(requirement.type);
        const int actualCount = countIt == areaCounts.end() ? 0 : countIt->second;
        if (actualCount != requirement.count) {
            errors.push_back({
                .code = MapIssueCode::AreaCountMismatch,
                .areaType = requirement.type,
                .expected = requirement.count,
                .actual = actualCount,
            });
        }
        for (const int floorNumber : requirement.floorNumbers) {
            if (floorNumbersByType[requirement.type].find(floorNumber) ==
                floorNumbersByType[requirement.type].end()) {
                errors.push_back({
                    .code = MapIssueCode::MissingRequiredFloorNumber,
                    .areaType = requirement.type,
                    .floorNumber = floorNumber,
                });
            }
        }
    }

    // Verifica IDs, referencias, tamanos y posiciones de todos los elementos.
    std::set<std::string> elementIds;
    std::map<std::string, const MapElement*> elementsById;
    std::map<std::string, std::vector<const MapElement*>> elementsByArea;
    std::set<const MapElement*> validElements;
    for (const auto& element : map.elements) {
        if (element.id.empty()) {
            errors.push_back({.code = MapIssueCode::EmptyElementId});
        } else if (!elementIds.insert(element.id).second) {
            errors.push_back({.code = MapIssueCode::DuplicateElementId, .elementId = element.id});
        } else {
            elementsById.emplace(element.id, &element);
        }
        if (!element.type.has_value()) {
            errors.push_back({.code = MapIssueCode::MissingElementType, .elementId = element.id});
        }
        if (!element.layer.has_value()) {
            errors.push_back({.code = MapIssueCode::MissingElementLayer, .elementId = element.id});
        }

        const auto areaIt = areasById.find(element.areaId);
        if (areaIt == areasById.end()) {
            errors.push_back({
                .code = MapIssueCode::UnknownElementArea,
                .areaId = element.areaId,
                .elementId = element.id,
            });
            continue;
        }

        const auto& area = *areaIt->second;
        if (element.width <= 0 || element.height <= 0) {
            errors.push_back({
                .code = MapIssueCode::InvalidElementDimensions,
                .areaId = element.areaId,
                .elementId = element.id,
            });
            continue;
        }
        if (element.x < 0 || element.y < 0 || element.x >= area.width ||
            element.y >= area.height || element.width > area.width - element.x ||
            element.height > area.height - element.y) {
            errors.push_back({
                .code = MapIssueCode::ElementOutsideAreaBounds,
                .areaId = element.areaId,
                .elementId = element.id,
            });
            continue;
        }
        elementsByArea[element.areaId].push_back(&element);
        validElements.insert(&element);
    }

    // Valida que cada conexion una areas existentes mediante una puerta o escalera valida.
    std::set<std::string> transitionIds;
    std::map<std::string, std::vector<const MapTransition*>> transitionsByArea;
    for (const auto& transition : map.transitions) {
        bool validTransition = true;
        if (transition.id.empty()) {
            errors.push_back({.code = MapIssueCode::EmptyTransitionId});
            validTransition = false;
        } else if (!transitionIds.insert(transition.id).second) {
            errors.push_back({
                .code = MapIssueCode::DuplicateTransitionId,
                .transitionId = transition.id,
            });
            validTransition = false;
        }

        const auto sourceIt = areasById.find(transition.fromAreaId);
        const auto targetIt = areasById.find(transition.toAreaId);
        if (sourceIt == areasById.end()) {
            errors.push_back({
                .code = MapIssueCode::UnknownTransitionSourceArea,
                .areaId = transition.fromAreaId,
                .transitionId = transition.id,
            });
            validTransition = false;
        }
        if (targetIt == areasById.end()) {
            errors.push_back({
                .code = MapIssueCode::UnknownTransitionTargetArea,
                .areaId = transition.toAreaId,
                .transitionId = transition.id,
            });
            validTransition = false;
        }

        const auto triggerIt = elementsById.find(transition.triggerElementId);
        if (triggerIt == elementsById.end()) {
            errors.push_back({
                .code = MapIssueCode::UnknownTransitionTriggerElement,
                .elementId = transition.triggerElementId,
                .transitionId = transition.id,
            });
            validTransition = false;
        } else {
            const auto& trigger = *triggerIt->second;
            if (trigger.areaId != transition.fromAreaId) {
                errors.push_back({
                    .code = MapIssueCode::TriggerNotInSourceArea,
                    .areaId = transition.fromAreaId,
                    .elementId = trigger.id,
                    .transitionId = transition.id,
                });
                validTransition = false;
            }
            const bool isTravelElement =
                trigger.type == ElementType::Door || trigger.type == ElementType::Stairs ||
                trigger.type == ElementType::DungeonEntrance;
            if (!isTravelElement || trigger.layer != ElementLayer::Structure) {
                errors.push_back({
                    .code = MapIssueCode::InvalidTransitionTrigger,
                    .elementId = trigger.id,
                    .transitionId = transition.id,
                    .elementType = trigger.type,
                    .expectedLayer = ElementLayer::Structure,
                    .actualLayer = trigger.layer,
                });
                validTransition = false;
            }
            if (validElements.find(&trigger) == validElements.end()) {
                validTransition = false;
            }
        }

        if (targetIt != areasById.end()) {
            const auto& targetArea = *targetIt->second;
            if (transition.targetX < 0 || transition.targetY < 0 ||
                transition.targetX >= targetArea.width || transition.targetY >= targetArea.height) {
                errors.push_back({
                    .code = MapIssueCode::TransitionArrivalOutsideArea,
                    .areaId = transition.toAreaId,
                    .transitionId = transition.id,
                    .x = transition.targetX,
                    .y = transition.targetY,
                });
                validTransition = false;
            }
        }

        if (validTransition) {
            transitionsByArea[transition.fromAreaId].push_back(&transition);
        }
    }

    // Comprueba que todas las areas tengan una salida y cumplan sus destinos requeridos.
    if (requirements.requireOutgoingTransitionForEachArea) {
        for (const auto& [areaId, areaType] : areaTypesById) {
            const auto outgoingIt = transitionsByArea.find(areaId);
            if (outgoingIt == transitionsByArea.end() || outgoingIt->second.empty()) {
                errors.push_back({
                    .code = MapIssueCode::AreaWithoutOutgoingTransition,
                    .areaId = areaId,
                });
            }
        }
    }
    for (const auto& requirement : requirements.transitions) {
        for (const auto& [sourceId, sourceType] : areaTypesById) {
            if (sourceType != requirement.fromAreaType ||
                (requirement.fromFloor.has_value() &&
                 areaFloorsById[sourceId] != requirement.fromFloor)) {
                continue;
            }

            bool hasRequiredRoute = false;
            const auto outgoingIt = transitionsByArea.find(sourceId);
            if (outgoingIt != transitionsByArea.end()) {
                for (const auto* transition : outgoingIt->second) {
                    const auto targetTypeIt = areaTypesById.find(transition->toAreaId);
                    if (targetTypeIt == areaTypesById.end() ||
                        targetTypeIt->second != requirement.toAreaType) {
                        continue;
                    }
                    if (requirement.toFloor.has_value() &&
                        areaFloorsById[transition->toAreaId] != requirement.toFloor) {
                        continue;
                    }
                    hasRequiredRoute = true;
                    break;
                }
            }
            if (!hasRequiredRoute) {
                errors.push_back({
                    .code = MapIssueCode::MissingRequiredTransition,
                    .areaId = sourceId,
                    .targetAreaType = requirement.toAreaType,
                    .targetFloor = requirement.toFloor,
                });
            }
        }
    }

    // Evita que las huellas de dos estructuras se superpongan en una misma area.
    if (!requirements.allowStructureOverlap) {
        for (const auto& [areaId, elements] : elementsByArea) {
            for (std::size_t firstIndex = 0; firstIndex < elements.size(); ++firstIndex) {
                const auto& first = *elements[firstIndex];
                if (first.layer != ElementLayer::Structure) {
                    continue;
                }
                for (std::size_t secondIndex = firstIndex + 1; secondIndex < elements.size();
                     ++secondIndex) {
                    const auto& second = *elements[secondIndex];
                    if (second.layer == ElementLayer::Structure &&
                        rectanglesOverlap(first, second)) {
                        errors.push_back({
                            .code = MapIssueCode::StructureOverlap,
                            .areaId = areaId,
                            .elementId = first.id,
                            .relatedElementId = second.id,
                        });
                    }
                }
            }
        }
    }

    // Comprueba las entidades requeridas segun la configuracion, sin listas hardcodeadas aca.
    for (const auto& requirement : requirements.elements) {
        bool hasTargetArea = false;
        bool hasCorrectElement = false;
        const MapElement* wrongLayerElement = nullptr;
        for (const auto& [areaId, areaType] : areaTypesById) {
            if (areaType != requirement.areaType) {
                continue;
            }
            hasTargetArea = true;
            for (const auto& element : map.elements) {
                if (element.areaId != areaId || element.type != requirement.type) {
                    continue;
                }
                if (element.layer == requirement.layer &&
                    validElements.find(&element) != validElements.end()) {
                    hasCorrectElement = true;
                    break;
                }
                if (element.layer.has_value() && element.layer != requirement.layer) {
                    wrongLayerElement = &element;
                }
            }
        }
        if (!hasTargetArea || hasCorrectElement) {
            continue;
        }
        if (wrongLayerElement != nullptr) {
            errors.push_back({
                .code = MapIssueCode::RequiredElementWrongLayer,
                .areaId = wrongLayerElement->areaId,
                .elementId = wrongLayerElement->id,
                .areaType = requirement.areaType,
                .elementType = requirement.type,
                .expectedLayer = requirement.layer,
                .actualLayer = wrongLayerElement->layer,
            });
        } else {
            errors.push_back({
                .code = MapIssueCode::MissingRequiredElement,
                .areaType = requirement.areaType,
                .elementType = requirement.type,
                .expectedLayer = requirement.layer,
            });
        }
    }

    return errors;
}

}  // namespace stardew::maps