#pragma once

#include <optional>
#include <string>

#include "map/MapDefinition.h"

namespace stardew::maps {

enum class MapIssueCode {
    MissingFormatVersion,
    UnsupportedFormatVersion,
    MissingMapName,
    EmptyAreaId,
    DuplicateAreaId,
    MissingAreaType,
    InvalidAreaDimensions,
    TileGridHeightMismatch,
    TileGridWidthMismatch,
    MissingAreaFloorNumber,
    UnexpectedAreaFloorNumber,
    DuplicateFloorNumber,
    AreaCountMismatch,
    MissingRequiredFloorNumber,
    EmptyElementId,
    DuplicateElementId,
    MissingElementType,
    MissingElementLayer,
    UnknownElementArea,
    InvalidElementDimensions,
    ElementOutsideAreaBounds,
    EmptyTransitionId,
    DuplicateTransitionId,
    UnknownTransitionSourceArea,
    UnknownTransitionTargetArea,
    UnknownTransitionTriggerElement,
    TriggerNotInSourceArea,
    InvalidTransitionTrigger,
    TransitionArrivalOutsideArea,
    AreaWithoutOutgoingTransition,
    MissingRequiredTransition,
    StructureOverlap,
    MissingRequiredElement,
    RequiredElementWrongLayer,
};

inline std::string_view toString(MapIssueCode code) {
    switch (code) {
        case MapIssueCode::MissingFormatVersion: return "MissingFormatVersion";
        case MapIssueCode::UnsupportedFormatVersion: return "UnsupportedFormatVersion";
        case MapIssueCode::MissingMapName: return "MissingMapName";
        case MapIssueCode::EmptyAreaId: return "EmptyAreaId";
        case MapIssueCode::DuplicateAreaId: return "DuplicateAreaId";
        case MapIssueCode::MissingAreaType: return "MissingAreaType";
        case MapIssueCode::InvalidAreaDimensions: return "InvalidAreaDimensions";
        case MapIssueCode::TileGridHeightMismatch: return "TileGridHeightMismatch";
        case MapIssueCode::TileGridWidthMismatch: return "TileGridWidthMismatch";
        case MapIssueCode::MissingAreaFloorNumber: return "MissingAreaFloorNumber";
        case MapIssueCode::UnexpectedAreaFloorNumber: return "UnexpectedAreaFloorNumber";
        case MapIssueCode::DuplicateFloorNumber: return "DuplicateFloorNumber";
        case MapIssueCode::AreaCountMismatch: return "AreaCountMismatch";
        case MapIssueCode::MissingRequiredFloorNumber: return "MissingRequiredFloorNumber";
        case MapIssueCode::EmptyElementId: return "EmptyElementId";
        case MapIssueCode::DuplicateElementId: return "DuplicateElementId";
        case MapIssueCode::MissingElementType: return "MissingElementType";
        case MapIssueCode::MissingElementLayer: return "MissingElementLayer";
        case MapIssueCode::UnknownElementArea: return "UnknownElementArea";
        case MapIssueCode::InvalidElementDimensions: return "InvalidElementDimensions";
        case MapIssueCode::ElementOutsideAreaBounds: return "ElementOutsideAreaBounds";
        case MapIssueCode::EmptyTransitionId: return "EmptyTransitionId";
        case MapIssueCode::DuplicateTransitionId: return "DuplicateTransitionId";
        case MapIssueCode::UnknownTransitionSourceArea: return "UnknownTransitionSourceArea";
        case MapIssueCode::UnknownTransitionTargetArea: return "UnknownTransitionTargetArea";
        case MapIssueCode::UnknownTransitionTriggerElement: return "UnknownTransitionTriggerElement";
        case MapIssueCode::TriggerNotInSourceArea: return "TriggerNotInSourceArea";
        case MapIssueCode::InvalidTransitionTrigger: return "InvalidTransitionTrigger";
        case MapIssueCode::TransitionArrivalOutsideArea: return "TransitionArrivalOutsideArea";
        case MapIssueCode::AreaWithoutOutgoingTransition: return "AreaWithoutOutgoingTransition";
        case MapIssueCode::MissingRequiredTransition: return "MissingRequiredTransition";
        case MapIssueCode::StructureOverlap: return "StructureOverlap";
        case MapIssueCode::MissingRequiredElement: return "MissingRequiredElement";
        case MapIssueCode::RequiredElementWrongLayer: return "RequiredElementWrongLayer";
    }
    return "UnknownMapIssue";
}

struct MapValidationIssue {
    MapIssueCode code;
    std::optional<std::string> areaId = std::nullopt;
    std::optional<std::string> elementId = std::nullopt;
    std::optional<std::string> relatedElementId = std::nullopt;
    std::optional<std::string> transitionId = std::nullopt;
    std::optional<AreaType> areaType = std::nullopt;
    std::optional<AreaType> targetAreaType = std::nullopt;
    std::optional<ElementType> elementType = std::nullopt;
    std::optional<ElementLayer> expectedLayer = std::nullopt;
    std::optional<ElementLayer> actualLayer = std::nullopt;
    std::optional<int> floorNumber = std::nullopt;
    std::optional<int> targetFloor = std::nullopt;
    std::optional<int> expected = std::nullopt;
    std::optional<int> actual = std::nullopt;
    std::optional<int> x = std::nullopt;
    std::optional<int> y = std::nullopt;

    bool operator==(const MapValidationIssue&) const = default;
};

}  // namespace stardew::maps