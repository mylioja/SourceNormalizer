#
#	Simple makefile for building the source_normalizer
#
TARGET   := source_normalizer

#	Directory for the build results
BUILD_DIR := ./build

SRC      := $(wildcard *.cpp)
HEADERS  := $(wildcard *.h)
OBJECTS  := $(SRC:%.cpp=$(BUILD_DIR)/%.o)

CXX      := g++-8
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -DDEBUG -O0 -g
LIBS     := -lstdc++fs

all: $(BUILD_DIR)/$(TARGET)

#	Do a full build if any header is updated
$(BUILD_DIR)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/$(TARGET) $^ $(LIBS)

.PHONY: all clean

clean:
	-@rm -rvf $(BUILD_DIR)/*
