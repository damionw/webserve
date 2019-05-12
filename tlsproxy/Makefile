PACKAGE_NAME ?= $(shell basename $(shell pwd))
PACKAGE_VERSION := 0.0.1

INCLUDES := -I. -Iinclude
CPPFLAGS := -O2
LDFLAGS := -lssl -lcrypto
LIBS :=
FLAGS :=
CC := gcc
CXX := g++
LD := g++
ARCH := $(shell uname -s)-$(shell uname -r)

BUILD_PATH := build
BUILD_BIN_PATH := $(BUILD_PATH)/bin/$(ARCH)
BUILD_LIB_PATH := $(BUILD_PATH)/lib/$(ARCH)
BUILD_SHARE_PATH := $(BUILD_PATH)/share/$(PACKAGE_NAME)
BUILD_CERT_PATH := $(BUILD_SHARE_PATH)/cert

SOURCES := $(wildcard *.cc *.c *.cpp)
HEADERFILES := $(wildcard *.h *.hh *.hpp includes/*)
SUBDIRS  := $(shell find * -xdev \( -type d -o -type l \) -prune -exec test -f {}/Makefile \; -print)
OBJECTS := $(foreach pattern, $(addsuffix .o, $(basename $(SOURCES))), $(addprefix $(BUILD_LIB_PATH)/, $(pattern)))
CERTFILES := $(BUILD_CERT_PATH)/cert.pem $(BUILD_CERT_PATH)/key.pem

TARGET_BASE := $(addprefix $(BUILD_BIN_PATH)/, $(PACKAGE_NAME))
BINARY_TARGET := $(BUILD_BIN_PATH)/$(PACKAGE_NAME)
DEPS := $(OBJECTS:.o=.d)

all: $(BINARY_TARGET) $(CERTFILES) docs

test: all
	@echo Tests unimplemented

$(CERTFILES): build
	@openssl req \
		-nodes \
		-new \
		-x509 \
		-keyout $(BUILD_CERT_PATH)/key.pem \
		-out $(BUILD_CERT_PATH)/cert.pem \
		-days 365 \
		-batch \
		-subj "/C=GB/ST=Bermuda/L=Bermuda/O=DKW Heavy Industries/OU=DKW Heavy Industries/CN=dkw.bm"

docs:
	-@test -f Doxyfile && (sed -e 's/^\(PACKAGE_NAME[ ]*=[ ]*\)[A-Za-z0-9]*/\1 '"$(basename $(pwd))/" < Doxyfile | doxygen - >/dev/null 2>&1)

$(BINARY_TARGET): $(OBJECTS)
	$(LD) $(OBJECTS) $(LDFLAGS) -o "$@"

$(BUILD_LIB_PATH)/%.o: %.cpp $(BUILD_LIB_PATH) $(HEADERFILES)
	$(CXX) $(CPPFLAGS) $(INCLUDES) $(FLAGS) -MMD -MF $(patsubst %.o,%.d,$@) -c -o $@ $<

$(BUILD_LIB_PATH)/%.o: %.cc $(BUILD_LIB_PATH) $(HEADERFILES)
	$(CXX) $(CPPFLAGS) $(INCLUDES) $(FLAGS) -MMD -MF $(patsubst %.o,%.d,$@) -c -o $@ $<

$(BUILD_LIB_PATH)/%.o: %.c $(BUILD_LIB_PATH) $(HEADERFILES)
	$(CXX) $(CPPFLAGS) $(INCLUDES) $(FLAGS) -MMD -MF $(patsubst %.o,%.d,$@) -c -o $@ $<

$(BUILD_LIB_PATH): build
	@install -d $@

$(BUILD_BIN_PATH): build
	@install -d $@

$(BUILD_BIN_PATH)/monitored_runner: checkouts/recipes $(BUILD_BIN_PATH)
	@install -C $</bash/$(notdir $@) $@

checkouts/recipes: | $(@D)
	@git clone https://github.com/damionw/recipes.git $@

-include $(DEPS)

build:
	@mkdir -p $@ $(BUILD_BIN_PATH) $(BUILD_LIB_PATH) $(BUILD_CERT_PATH)

checkouts:
	@mkdir -p $@

clean:
	-@rm -rf build checkouts packages local *.pem $(DEPS) checkouts