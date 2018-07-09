PACKAGE_NAME := Webserve
BUILD_SHARE_PATH := build/share/$(PACKAGE_NAME)
INSTALL_PATH := $(shell python -c 'import sys; print sys.prefix if hasattr(sys, "real_prefix") else exit(255)' 2>/dev/null || echo "/usr/local")

.PHONY: tests clean

all: build
	@cp -R examples/. $(BUILD_SHARE_PATH)/examples/.
	@cp -R src/webserve build/bin/.

demo: all
	build/bin/webserve \
		--static=$(BUILD_SHARE_PATH)/examples/html \
		--api=$(BUILD_SHARE_PATH)/examples/api \
		--module=$(BUILD_SHARE_PATH)/examples/modules/primary-site \
		--module=$(BUILD_SHARE_PATH)/examples/modules/demo1 \
		--module=$(BUILD_SHARE_PATH)/examples/modules/postit \
		--port=11111

install: tests
	@echo "Installing into directory '$(INSTALL_PATH)'"
	@rsync -az build/ $(INSTALL_PATH)/

version: all
	@build/bin/webserve --version

build:
	@install -d build/bin $(BUILD_SHARE_PATH)/examples

tests: all
	@PATH="$(shell readlink -f build/bin):$(PATH)" unittests/testsuite

clean:
	-@rm -rf build checkouts
