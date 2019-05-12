PACKAGE_NAME := webserve

PACKAGE_VERSION := $(shell src/bin/$(PACKAGE_NAME) --version)

LOCAL_PATH := local
LOCAL_BIN_PATH := $(LOCAL_PATH)/bin

BUILD_PATH := build
BUILD_BIN_PATH := $(BUILD_PATH)/bin
BUILD_LIB_PATH := $(BUILD_PATH)/lib
BUILD_SHARE_PATH := $(BUILD_PATH)/share
BUILD_SHARE_EXAMPLES := $(BUILD_SHARE_PATH)/$(PACKAGE_NAME)/examples

LIB_COMPONENTS := $(wildcard src/lib/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/*)
BIN_COMPONENTS := $(foreach name, $(wildcard src/bin/*), $(BUILD_BIN_PATH)/$(notdir $(name)))
DIR_COMPONENTS := $(foreach name, bin share lib, build/$(name))

ARCH := $(shell uname -s)-$(shell uname -r)
INSTALL_PATH := $(shell python -c 'import sys; print(sys.prefix if (hasattr(sys, "real_prefix") or (hasattr(sys, "base_prefix") and sys.base_prefix != sys.prefix)) else "/usr/local")')
PACKAGE_DESTINATION := /opt/$(PACKAGE_NAME)
PACKAGE_FILE := packages/$(PACKAGE_NAME)-$(PACKAGE_VERSION)-$(ARCH).deb
FREE_PORT := $(shell netstat -ln --tcp -4 | awk '{if (NR > 2) {print $$4;}}' | awk -F: '{print $$2;}' | sort -un | awk '{for (i=a + 1; i < $$1; ++i) {if (i >= start && i <= finish) {print i;}} a=$$1;}' a=0 start=13000 finish=13200 | head -1)

.PHONY: tests clean help $(BUILD_BIN_PATH)/tlsproxy

all: $(BUILD_BIN_PATH)/$(PACKAGE_NAME) $(BUILD_BIN_PATH)/tlsproxy examples

help:
	@echo "Usage: make build|tests|all|clean|version|install|dist|demo"

dist: $(PACKAGE_FILE)

version: $(BUILD_BIN_PATH)/$(PACKAGE_NAME)
	@$< --version

demo: $(BUILD_BIN_PATH)/$(PACKAGE_NAME) examples
	$< \
		--static=$(BUILD_SHARE_EXAMPLES)/html \
		--api=$(BUILD_SHARE_EXAMPLES)/api \
		--module=$(BUILD_SHARE_EXAMPLES)/modules/primary-site \
		--module=$(BUILD_SHARE_EXAMPLES)/modules/demo1 \
		--module=$(BUILD_SHARE_EXAMPLES)/modules/postit \
		--debug \
		--port=$(FREE_PORT)

tlsdemo: $(BUILD_BIN_PATH)/$(PACKAGE_NAME) $(BUILD_BIN_PATH)/tlsproxy examples
	$< \
	--static=$(BUILD_SHARE_EXAMPLES)/html \
	--api=$(BUILD_SHARE_EXAMPLES)/api \
	--proxy="$(BUILD_BIN_PATH)/tlsproxy" \
	--certfile=tlsproxy/build/share/tlsproxy/cert/cert.pem \
	--keyfile=tlsproxy/build/share/tlsproxy/cert/key.pem \
	--module=$(BUILD_SHARE_EXAMPLES)/modules/primary-site \
	--module=$(BUILD_SHARE_EXAMPLES)/modules/demo1 \
	--module=$(BUILD_SHARE_EXAMPLES)/modules/postit \
	--debug \
	--port=$(FREE_PORT)

install: tests
	@echo "Installing into directory '$(INSTALL_PATH)'"
	@rsync -az build/ $(INSTALL_PATH)/

$(PACKAGE_FILE): $(LOCAL_BIN_PATH)/debianizer packages all
	@$< \
		$(foreach pattern, $(PREREQUISITES), $(addprefix --requires=, $(pattern))) \
		--source=$(BUILD_PATH) \
		--root=$(PACKAGE_DESTINATION) \
		--target=$@ \
		--package="$(PACKAGE_NAME)" \
		--version=$(PACKAGE_VERSION) \
		--postinstall=install_hook \
		--preuninstall=install_hook

	@dpkg --info $@
	@dpkg --contents $@

$(BUILD_BIN_PATH)/tlsproxy: build
	@$(MAKE) -C $(notdir $@) all
	install $(notdir $@)/build/bin/$(ARCH)/$(notdir $@) $@

$(BUILD_BIN_PATH)/%: build | src/bin
	@install -m 755 src/bin/$(notdir $@) $@

$(DIR_COMPONENTS): build
	@install -d $@

$(LOCAL_BIN_PATH)/debianizer: checkouts/recipes local
	@install -C $</bash/$(notdir $@) $@

checkouts/recipes: | $(@D)
	@git clone https://github.com/damionw/recipes.git $@

examples: build
	@rsync -az examples/ $(BUILD_SHARE_PATH)/$(PACKAGE_NAME)/examples/

local:
	@mkdir -p $@ $@/bin $@/lib $@/share

forced:
	@true

packages:
	@mkdir -p $@

build:
	@install -d $@ $(BUILD_BIN_PATH) $(BUILD_SHARE_PATH)/$(PACKAGE_NAME)/examples

tests: all
	@PATH="$(shell readlink -f build/bin):$(PATH)" unittests/testsuite

clean:
	-@rm -rf build packages local checkouts
	-@make -C tlsproxy clean
