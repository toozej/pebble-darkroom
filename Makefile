# Set sane defaults for Make
SHELL = bash
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

# Set default goal such that `make` runs `make help`
.DEFAULT_GOAL := help

# Pebble app build configuration
APP_NAME := pebble-darkroom
VERSION := 1.4.0

# Pebble SDK configuration
PEBBLE_SDK_VERSION := 4.33.1

# Project directory
PROJ_DIR_LOCAL := app/pebble-darkroom
PROJ_DIR := $(PROJ_DIR_LOCAL)

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
BUILD_CONTAINER := pebble-sdk-build
DOCKER_CMD := pebble build


.PHONY: all build emulate test local local-prereqs local-init local-build local-test local-run local-install local-init local-watch local-package local-release local-logs pre-commit-install pre-commit-update pre-commit-run pre-commit clean help

build-docker-image: ## Build the Docker image for Pebble SDK
	docker build --build-arg UID=$$(id -u) --build-arg GID=$$(id -g) -f Dockerfile -t pebble-sdk:latest .

build: build-docker-image ## Build the Pebble app using Docker
	-docker rm -f $(BUILD_CONTAINER) 2>/dev/null || true
	docker run --name $(BUILD_CONTAINER) -v $(CURDIR)/app/$(APP_NAME):/workspace/$(APP_NAME) --workdir /workspace/$(APP_NAME) pebble-sdk:latest bash -c "$(DOCKER_CMD)"

emulate: build-docker-image ## Build and run the Pebble app in the emulator
	PEBBLE_UID=$$(id -u) PEBBLE_GID=$$(id -g) docker compose down --remove-orphans
	PEBBLE_UID=$$(id -u) PEBBLE_GID=$$(id -g) docker compose up --build -d
	@echo "Pebble emulator is running in the background."
	sleep 5 # Wait for emulator to start
	@echo "To connect, open your browser and go to: http://127.0.0.1:8080"
	@echo "To stop the emulator, run: make clean"

copy: ## Copy built Pebble app from Docker image to local filesystem
	docker cp $(BUILD_CONTAINER):/workspace/$(APP_NAME)/build/$(APP_NAME).pbw $(CURDIR)/$(APP_NAME).pbw
	-docker rm -f $(BUILD_CONTAINER)

test: ## Run unit tests using Docker
	@echo "Building and running unit tests in Docker..."
	docker build --no-cache --target test -f Dockerfile -t pebble-sdk:test .
	@echo "Unit tests completed"

local-prereqs: ## Install Pebble SDK and prereqs locally
	@echo "Installing Pebble SDK and prerequisites..."
	command -v uv || { echo "Install uv from https://docs.astral.sh/uv/getting-started/installation/"; exit 1; }
	uv tool install --upgrade pebble-tool
	pebble --version
	pebble sdk install $(PEBBLE_SDK_VERSION)
	@echo "Pebble SDK installed successfully"

local-init: local-prereqs ## Verify the local project and install its toolchain
	@test -f $(PROJ_DIR_LOCAL)/$(PACKAGE_JSON)
	@test -f $(PROJ_DIR_LOCAL)/wscript

local-build: local-init ## Build the application using locally installed toolchain
	@echo "Building application..."
	cd $(PROJ_DIR_LOCAL) && pebble build
	@echo "Build complete"

local-test: ## Run unit tests using locally installed toolchain
	@echo "Building and running unit tests locally..."
	gcc -I./app/$(APP_NAME)/tests -I./app/$(APP_NAME)/src/c \
	-o app/$(APP_NAME)/test_runner \
	app/$(APP_NAME)/tests/test_runner.c \
	app/$(APP_NAME)/tests/settings.c \
	app/$(APP_NAME)/tests/test_settings.c \
	app/$(APP_NAME)/tests/test_timer.c \
	app/$(APP_NAME)/tests/test_display.c \
	app/$(APP_NAME)/tests/unity.c \
	-lm
	cd app/$(APP_NAME) && ./test_runner
	@echo "Local unit tests completed"

local-install: local-build ## Install on connected Pebble using locally installed toolchain
	@echo "Installing on Pebble..."
	cd $(PROJ_DIR_LOCAL) && pebble install
	@echo "Installation complete"

local-run: local-build ## Run in emulator using locally-installed toolchain
	@echo "Running in emulator..."
	cd $(PROJ_DIR_LOCAL) && pebble install --emulator basalt
	@echo "Emulator launched"

local-package: local-build ## Package for distribution using locally installed toolchain
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
	cd $(PROJ_DIR_LOCAL) && pebble build --watch

local-logs: ## Show logs from connected Pebble using locally installed toolchain
	pebble logs

pre-commit: pre-commit-install pre-commit-run ## Install and run pre-commit hooks

pre-commit-install: ## Install pre-commit hooks and necessary binaries
	# shellcheck
	command -v shellcheck || sudo dnf install -y ShellCheck || sudo apt install -y shellcheck
	# checkmake
	go install github.com/checkmake/checkmake/cmd/checkmake@latest
	# actionlint
	command -v actionlint || brew install actionlint || go install github.com/rhysd/actionlint/cmd/actionlint@latest
	# install pre-commit hooks
	pre-commit install

pre-commit-update: ## Update pinned pre-commit hook revisions
	pre-commit autoupdate

pre-commit-run: ## Run pre-commit hooks against all files
	pre-commit run --all-files

clean: ## Remove any locally compiled binaries
	-docker-compose down --remove-orphans
	rm -rf $(BUILD_DIR)/*
	rm -rf $(DIST_DIR)/*

help: ## Display help text
	@grep -E '^[a-zA-Z_-]+ ?:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'
