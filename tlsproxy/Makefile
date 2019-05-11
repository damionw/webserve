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
BUILD_SHARE_PATH := $(BUILD_PATH)/share

SOURCES := $(wildcard *.cc *.c *.cpp)
HEADERFILES := $(wildcard *.h *.hh *.hpp includes/*)
SUBDIRS  := $(shell find * -xdev \( -type d -o -type l \) -prune -exec test -f {}/Makefile \; -print)
OBJECTS := $(foreach pattern, $(addsuffix .o, $(basename $(SOURCES))), $(addprefix $(BUILD_LIB_PATH)/, $(pattern)))
TARGET_BASE := $(addprefix $(BUILD_BIN_PATH)/, $(PACKAGE_NAME))
BINARY_TARGET := $(BUILD_BIN_PATH)/$(PACKAGE_NAME)
DEPS := $(OBJECTS:.o=.d)

all: $(BINARY_TARGET) docs

test: all
	@echo Tests unimplemented

cert.pem:
	openssl req -nodes -new -x509 -keyout key.pem -out cert.pem -days 365
#	openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365

plaindemo:
	./webserve --debug --static=static --module=modules/plugin --port=12121

demo: $(BUILD_BIN_PATH)/monitored_runner
	@$(BUILD_BIN_PATH)/monitored_runner "make _demo" $(SOURCES) Makefile static modules

_demo: all cert.pem
# 	./$(BINARY_TARGET)
# 	tcpserver -q -d -H -c 1 -b 1 0 12121 ./$(BINARY_TARGET)
	./webserve --proxy="$(BINARY_TARGET)" --debug --static=static --module=modules/plugin --port=12121

subdirs: $(SUBDIRS)

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

$(SUBDIRS): forced
	@$(MAKE) -C "$@" $(MAKECMDGOALS)

$(BUILD_LIB_PATH): build
	@install -d $@

$(BUILD_BIN_PATH): build
	@install -d $@

$(BUILD_BIN_PATH)/monitored_runner: checkouts/recipes $(BUILD_BIN_PATH)
	@install -C $</bash/$(notdir $@) $@

checkouts/recipes: | $(@D)
	@git clone https://github.com/damionw/recipes.git $@

-include $(DEPS)

forced:
	@true

build:
	@mkdir -p $@ $(BUILD_BIN_PATH) $(BUILD_LIB_PATH)

checkouts:
	@mkdir -p $@

clean:
	-@rm -rf build checkouts packages local *.pem $(DEPS) checkouts