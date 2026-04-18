# Makefile: Install CKPE Starfield files via symlinks
#
# Usage:
#   make SF_DIR=/path/to/starfield/creation_kit install
#   make SF_DIR=/path/to/starfield/creation_kit uninstall
#
# SF_DIR must point to the directory containing CreationKit.exe
# Default assumes a typical Steam Linux install path.

SF_DIR ?= $(HOME)/.local/share/Steam/steamapps/common/Starfield

REPO_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

BUILD_DIR   := $(REPO_ROOT)/x64
STUFFS_SF   := $(REPO_ROOT)/Stuffs/SF
DIALOGS_SF  := $(REPO_ROOT)/Dialogs/SF
DATABASE_SF := $(REPO_ROOT)/Database/SF

DLLS := \
	CKPE.dll \
	CKPE.Common.dll \
	CKPE.Starfield.dll \
	CKPE.PluginAPI.dll \
	winhttp.dll

PLUGIN_DLLS := \
	WineCompat.dll

PAK_DIALOG   := CreationKitPlatformExtended_SF_Dialogs.pak
PAK_RESOURCE := CreationKitPlatformExtended_SF_QResources.pak
PAK_DATABASE := CreationKitPlatformExtended_SF_Databases.pak

TOML_NAME := CreationKitPlatformExtended.toml
TOML_SRC  := $(STUFFS_SF)/$(TOML_NAME)
TOML_DST  := $(SF_DIR)/$(TOML_NAME)

PLUGIN_DIR     := $(SF_DIR)/CKPEPlugins
DLL_TARGETS    := $(addprefix $(SF_DIR)/,$(DLLS))
PLUGIN_TARGETS := $(addprefix $(PLUGIN_DIR)/,$(PLUGIN_DLLS))
PAK_TARGETS    := $(SF_DIR)/$(PAK_DIALOG) $(SF_DIR)/$(PAK_RESOURCE) $(SF_DIR)/$(PAK_DATABASE)

DOCKER_IMAGE ?= ckpe-build
CONFIG       ?= Release

.PHONY: all install uninstall check docker-build $(PLUGIN_DIR)

all: install

docker-build:
	docker run --rm \
		--user $(shell id -u):$(shell id -g) \
		-e HOME=/tmp \
		-e WINEPREFIX=/tmp/.wine \
		-e WINEDEBUG=-all \
		-v "$(REPO_ROOT):/workspace" \
		-w /workspace \
		$(DOCKER_IMAGE) \
		bash -c 'mkdir -p /tmp/.wine && git config --global --add safe.directory "*" && CONFIG=$(CONFIG) bash .devcontainer/build.sh'

check:
	@test -n "$(SF_DIR)" || (echo "ERROR: SF_DIR is not set"; exit 1)
	@test -d "$(SF_DIR)" || (echo "ERROR: SF_DIR does not exist: $(SF_DIR)"; exit 1)
	@for dll in $(DLLS); do \
		test -f "$(BUILD_DIR)/$$dll" || (echo "ERROR: Missing build artifact: $(BUILD_DIR)/$$dll"; exit 1); \
	done
	@for dll in $(PLUGIN_DLLS); do \
		test -f "$(BUILD_DIR)/$$dll" || (echo "ERROR: Missing plugin artifact: $(BUILD_DIR)/$$dll"; exit 1); \
	done
	@test -f "$(DIALOGS_SF)/$(PAK_DIALOG)"   || (echo "ERROR: Missing $(PAK_DIALOG)"; exit 1)
	@test -f "$(STUFFS_SF)/$(PAK_RESOURCE)"  || (echo "ERROR: Missing $(PAK_RESOURCE)"; exit 1)
	@test -f "$(DATABASE_SF)/$(PAK_DATABASE)" || (echo "ERROR: Missing $(PAK_DATABASE)"; exit 1)
	@echo "All sources verified."

$(PLUGIN_DIR):
	mkdir -p $@

install: check $(DLL_TARGETS) $(PLUGIN_DIR) $(PLUGIN_TARGETS) $(PAK_TARGETS)
	@if [ ! -f "$(TOML_DST)" ]; then \
		cp "$(TOML_SRC)" "$(TOML_DST)"; \
		echo "Copied TOML: $(TOML_DST)"; \
	else \
		echo "TOML already exists, skipping: $(TOML_DST)"; \
	fi
	@echo "Install complete -> $(SF_DIR)"

# DLL symlinks (all live in x64/)
$(SF_DIR)/%.dll: $(BUILD_DIR)/%.dll
	ln -sf $< $@
	@echo "Linked: $@"

# Plugin DLL symlinks (from x64/ into CKPEPlugins/)
$(PLUGIN_DIR)/%.dll: $(BUILD_DIR)/%.dll | $(PLUGIN_DIR)
	ln -sf $< $@
	@echo "Linked plugin: $@"

# PAK symlinks (each from its own source subdirectory)
$(SF_DIR)/$(PAK_DIALOG): $(DIALOGS_SF)/$(PAK_DIALOG)
	ln -sf $< $@
	@echo "Linked: $@"

$(SF_DIR)/$(PAK_RESOURCE): $(STUFFS_SF)/$(PAK_RESOURCE)
	ln -sf $< $@
	@echo "Linked: $@"

$(SF_DIR)/$(PAK_DATABASE): $(DATABASE_SF)/$(PAK_DATABASE)
	ln -sf $< $@
	@echo "Linked: $@"

uninstall:
	@echo "Removing CKPE Starfield symlinks from $(SF_DIR)..."
	@for dll in $(DLLS); do \
		rm -f "$(SF_DIR)/$$dll" && echo "Removed: $(SF_DIR)/$$dll" || true; \
	done
	@for dll in $(PLUGIN_DLLS); do \
		rm -f "$(PLUGIN_DIR)/$$dll" && echo "Removed: $(PLUGIN_DIR)/$$dll" || true; \
	done
	@rm -f "$(SF_DIR)/$(PAK_DIALOG)"   && echo "Removed: $(SF_DIR)/$(PAK_DIALOG)"   || true
	@rm -f "$(SF_DIR)/$(PAK_RESOURCE)" && echo "Removed: $(SF_DIR)/$(PAK_RESOURCE)" || true
	@rm -f "$(SF_DIR)/$(PAK_DATABASE)" && echo "Removed: $(SF_DIR)/$(PAK_DATABASE)" || true
	@echo "Uninstall complete. NOTE: TOML was not removed (may contain local edits)."
