CXX ?= g++
PKG_CONFIG ?= pkg-config
TARGET := stardew-map-editor
SOURCES := $(shell find src -name '*.cpp')
HEADERS := $(shell find src -name '*.h')
QT_CFLAGS := $(shell $(PKG_CONFIG) --cflags Qt6Widgets)
QT_LIBS := $(shell $(PKG_CONFIG) --libs Qt6Widgets)
CXXFLAGS ?= -std=c++20 -Wall -Wextra -pedantic -fPIC

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -Isrc $(QT_CFLAGS) $(SOURCES) -o $@ $(QT_LIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
