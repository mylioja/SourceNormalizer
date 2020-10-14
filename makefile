#   A simple makefile for building the source_normalizer
#
#   Copyright (C) 2020  Martti Ylioja
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Usage:
#
#   A plain "make" will build a non-optimized debug version with symbols.
#
#   Do "make release" to build an optimized version.
#   Release builds always start with a "clean" to force a full build.
#
TARGET   := source_normalizer

#	Directory for the build results
BUILD_DIR := ./build

SRC      := $(wildcard *.cpp)
HEADERS  := $(wildcard *.h)
OBJECTS  := $(SRC:%.cpp=$(BUILD_DIR)/%.o)

CXX      := g++-8
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror
LIBS     := -lstdc++fs

#	Detect "release" in the build targets and act accordingly
ifeq (,$(findstring release, $(MAKECMDGOALS)))
#
#	This is for debug.
CXXFLAGS += -DDEBUG -O0 -g
else
#
#	This is for release. Set options and force a full build.
CXXFLAGS += -DNDEBUG -O3
FORCE_CLEAN := clean
endif

all release: $(BUILD_DIR)/$(TARGET)

#	Do a full build if any header is updated
$(BUILD_DIR)/%.o: %.cpp $(HEADERS) $(FORCE_CLEAN)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/$(TARGET) $^ $(LIBS)

.PHONY: all clean release

clean:
	-@rm -rvf $(BUILD_DIR)/*
