# Set sane defaults for Make
SHELL = bash
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

# Set default goal such that `make` runs `make help`
.DEFAULT_GOAL := help

# Pebble app build configuration
APP_NAME := pebble-darkroom
VERSION := 1.0.0

# Pebble SDK configuration
PEBBLE_SDK_VERSION := 4.5
PEBBLE_TOOL_PATH := $(HOME)/pebble-dev/pebble-sdk-$(PEBBLE_SDK_VERSION)

# Project directory
PROJ_DIR_LOCAL := app/pebble-darkroom
PROJ_DIR := pebble-darkroom

# Build directories
BUILD_DIR := $(PROJ_DIR)/build
DIST_DIR := $(PROJ_DIR)/dist

# Source files
SRC_DIR := $(PROJ_DIR)/src
C_FILES := $(wildcard $(SRC_DIR)/*.c)
H_FILES := $(wildcard $(SRC_DIR)/*.h)

# Resource files
RESOURCES_DIR := $(PROJ_DIR)/resources

# Package metadata
PBW_FILE := $(BUILD_DIR)/$(APP_NAME).pbw
PACKAGE_JSON := package.json
APP_JSON := appinfo.json

# Docker info
DOCKER_CMD := pebble new-project pebble-darkroom && find /pebble-darkroom -maxdepth 1 -type f -not -name 'build' -exec cp {} /workspace/pebble-darkroom/ \; && find /pebble-darkroom/src -type f -exec cp {} /workspace/pebble-darkroom/src/ \; && cd /workspace/pebble-darkroom && pebble build


.PHONY: all build test local local-prereqs local-init local-build local-test local-run local-install local-init local-watch local-package local-release local-logs pre-commit-install pre-commit-run pre-commit clean help

build: ## Build the Pebble app using Docker
	docker build -f Dockerfile -t pebble-sdk:latest .
	-docker rm -f pebble-sdk 2>/dev/null || true
	docker run --name pebble-sdk --user 1000:100 -v $(CURDIR)/app/$(APP_NAME):/$(APP_NAME) --workdir /workspace/ pebble-sdk:latest bash -c "$(DOCKER_CMD)"
	docker stop pebble-sdk

copy: ## Copy built Pebble app from Docker image to local filesystem
	docker cp pebble-sdk:/workspace/$(PBW_FILE) $(CURDIR)/$(APP_NAME).pbw
	-docker rm -f pebble-sdk

test: ## Run unit tests using Docker
	@echo "Building and running unit tests in Docker..."
	docker build --target test -f Dockerfile -t pebble-sdk:test .
	@echo "Unit tests completed"

local-prereqs: ## Install Pebble SDK and prereqs locally
	@echo "Installing Pebble SDK and prerequisites..."
	command -v uv || brew install uv || sudo apt-get update -qq && sudo apt-get install -y uv || sudo dnf install -y uv || curl -LsSf https://astral.sh/uv/install.sh | sh
	uv tool install pebble-tool
	pebble --version
	pebble sdk install latest
	@echo "Pebble SDK installed successfully"

local-init: local-prereqs ## Initialize project using locally installed toolchain
	@echo "Initializing project..."
	mkdir -p $(SRC_DIR) $(RESOURCES_DIR) $(BUILD_DIR) $(DIST_DIR)
	# Create package.json if it doesn't exist
	@if [ ! -f "$(PACKAGE_JSON)" ]; then \
	echo '{"name":"$(APP_NAME)","version":"$(VERSION)","private":true}' > $(PACKAGE_JSON); \
	fi
	# Create appinfo.json if it doesn't exist
	@if [ ! -f "$(APP_JSON)" ]; then \
	echo '{ \
	"uuid": "'$$(uuidgen)'", \
	"shortName": "$(APP_NAME)", \
	"longName": "Darkroom Timer", \
	"companyName": "Your Company", \
	"versionLabel": "$(VERSION)", \
	"sdkVersion": "3", \
	"targetPlatforms": ["aplite", "basalt", "chalk", "diorite"], \
	"watchapp": { \
	"watchface": false \
	}, \
	"resources": { \
	"media": [] \
	} \
	}' > $(APP_JSON); \
	fi

local-build: init ## Build the application using locally installed toolchain
	@echo "Building application..."
	source $(PEBBLE_TOOL_PATH)/.env/bin/activate && \
	pebble build
	@echo "Build complete"

local-test: ## Run unit tests using locally installed toolchain
	@echo "Building and running unit tests locally..."
	gcc -I./app/$(APP_NAME)/tests -I./app/$(APP_NAME)/src/c \
	-o app/$(APP_NAME)/test_runner \
	app/$(APP_NAME)/tests/test_runner.c \
	app/$(APP_NAME)/src/c/settings.c \
	app/$(APP_NAME)/tests/test_settings.c \
	app/$(APP_NAME)/tests/test_timer.c \
	app/$(APP_NAME)/tests/test_display.c \
	-lm
	cd app/$(APP_NAME) && ./test_runner
	@echo "Local unit tests completed"

local-install: build ## Install on connected Pebble using locally installed toolchain
	@echo "Installing on Pebble..."
	source $(PEBBLE_TOOL_PATH)/.env/bin/activate && \
	pebble install
	@echo "Installation complete"

local-run: build ## Run in emulator using locally-installed toolchain
	@echo "Running in emulator..."
	source $(PEBBLE_TOOL_PATH)/.env/bin/activate && \
	pebble install --emulator basalt
	@echo "Emulator launched"

local-package: build ## Package for distribution using locally installed toolchain
	@echo "Creating distribution package..."
	mkdir -p $(DIST_DIR)
	cp $(BUILD_DIR)/*.pbw $(DIST_DIR)/
	cd $(DIST_DIR) && \
	tar -czf $(APP_NAME)-$(VERSION).tar.gz *.pbw
	@echo "Package created in $(DIST_DIR)"

# Create release bundle
local-release: local-package ## Create release bundle using locally installed toolchain
	@echo "Creating release..."
	mkdir -p $(DIST_DIR)/release
	cp $(DIST_DIR)/$(APP_NAME)-$(VERSION).tar.gz $(DIST_DIR)/release/
	cp README.md $(DIST_DIR)/release/ 2>/dev/null || echo "No README.md found"
	cp LICENSE $(DIST_DIR)/release/ 2>/dev/null || echo "No LICENSE found"
	cd $(DIST_DIR)/release && \
	zip -r ../$(APP_NAME)-$(VERSION)-release.zip *
	@echo "Release bundle created in $(DIST_DIR)"

local-watch: local-build ## Watch development mode using locally installed toolchain
	@echo "Starting development watch mode..."
	source $(PEBBLE_TOOL_PATH)/.env/bin/activate && \
	pebble build --watch

local-logs: ## Show logs from connected Pebble using locally installed toolchain
	source $(PEBBLE_TOOL_PATH)/.env/bin/activate && \
	pebble logs

pre-commit: pre-commit-install pre-commit-run ## Install and run pre-commit hooks

pre-commit-install: ## Install pre-commit hooks and necessary binaries
	# shellcheck
	command -v shellcheck || sudo dnf install -y ShellCheck || sudo apt install -y shellcheck
	# checkmake
	go install github.com/checkmake/checkmake/cmd/checkmake@latest
	# install and update pre-commits
	pre-commit install
	pre-commit autoupdate

pre-commit-run: ## Run pre-commit hooks against all files
	pre-commit run --all-files

clean: ## Remove any locally compiled binaries
	-docker kill pebble-sdk
	-docker rm pebble-sdk
	rm -rf $(BUILD_DIR)/*
	rm -rf $(DIST_DIR)/*

help: ## Display help text
	@grep -E '^[a-zA-Z_-]+ ?:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'
