# Directories
BUILD_DIR = build
INSTALL_PREFIX ?= /usr/local

# Default target
.PHONY: all
all: build

# Create build directory and generate build files
.PHONY: configure
configure:
	@mkdir -p $(BUILD_DIR)
	cmake -B$(BUILD_DIR) -H.

# Build the project
.PHONY: build
build: configure
	cmake --build $(BUILD_DIR)

# Install the library and headers
.PHONY: install
install: build
	cmake --install $(BUILD_DIR)

# Clean build directory
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
