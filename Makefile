CXX ?= g++
PKG_CONFIG ?= pkg-config
TARGET := stardew-map-editor
TEST_TARGET := map-validator-tests
SOURCES := $(shell find src -name '*.cpp')
HEADERS := $(shell find src -name '*.h')
TEST_SOURCES := tests/map_validator_test.cpp src/map/validation/MapValidator.cpp \
	src/map/persistence/MapFile.cpp src/map/persistence/MapRepository.cpp \
	src/map/service/MapService.cpp
QT_CFLAGS := $(shell $(PKG_CONFIG) --cflags Qt6Widgets)
QT_LIBS := $(shell $(PKG_CONFIG) --libs Qt6Widgets)
TOML_CFLAGS := $(shell $(PKG_CONFIG) --cflags tomlplusplus)
TOML_LIBS := $(shell $(PKG_CONFIG) --libs tomlplusplus)
CXXFLAGS ?= -std=c++20 -Wall -Wextra -pedantic -fPIC

.PHONY: all run test clean

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -Isrc $(QT_CFLAGS) $(TOML_CFLAGS) $(SOURCES) -o $@ $(QT_LIBS) $(TOML_LIBS)

run: $(TARGET)
	./$(TARGET)

$(TEST_TARGET): $(TEST_SOURCES) src/map/persistence/MapRequirementsLoader.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -Isrc $(TOML_CFLAGS) $(TEST_SOURCES) src/map/persistence/MapRequirementsLoader.cpp -o $@ $(TOML_LIBS)

test: $(TEST_TARGET) config/map_requirements.toml examples/maps/basic_valid.toml examples/maps/basic_missing.toml
	./$(TEST_TARGET) config/map_requirements.toml examples/maps/basic_valid.toml examples/maps/basic_missing.toml

clean:
	rm -f $(TARGET) $(TEST_TARGET)
