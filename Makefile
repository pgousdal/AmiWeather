PLATFORM ?= posix
AMIAPI_DIR ?= ../AmiAPI
BUILD := build/$(PLATFORM)
AMIAPI_INCLUDE := $(AMIAPI_DIR)/include
AMIAPI_LIB := $(AMIAPI_DIR)/build/libamiapi.a

CFLAGS ?= -O2 -Wall -Wextra -pedantic
CPPFLAGS += -Iinclude -I$(AMIAPI_INCLUDE)
COMMON_SRC := src/weather.c src/preferences.c src/openmeteo.c
COMMON_OBJ := $(patsubst src/%.c,$(BUILD)/%.o,$(COMMON_SRC))
DOOR_OBJ := $(BUILD)/door/door.o

ifeq ($(PLATFORM),posix)
CC ?= cc
PROGRAM := $(BUILD)/amiweather-posix
PLATFORM_OBJ := $(BUILD)/main_posix.o
DOOR_PROGRAM :=
LDLIBS += -lssl -lcrypto
else ifeq ($(PLATFORM),amiga)
ifeq ($(origin CC),default)
CC := m68k-amigaos-gcc
endif
CFLAGS += -m68020
CPPFLAGS += -I/opt/amiga/AmiSSL/Developer/include -DAMIWEATHER_AMIGA
PROGRAM := $(BUILD)/AmiWeather
PLATFORM_OBJ := $(BUILD)/main_amiga.o $(BUILD)/gui_amiga.o
DOOR_PROGRAM := $(BUILD)/AmiWeatherDoor
DOOR_PLATFORM_OBJ := $(BUILD)/door/main_amiga.o $(BUILD)/door/terminal_amiga.o
LDLIBS += -L/opt/amiga/AmiSSL/Developer/lib/AmigaOS3 -lamisslauto
else
$(error Unknown PLATFORM '$(PLATFORM)'; use posix or amiga)
endif

.PHONY: all clean test door-test smoke check-sdk release validate-release
all: check-sdk $(PROGRAM) $(DOOR_PROGRAM)

check-sdk:
	@test -f "$(AMIAPI_INCLUDE)/amiapi/amiapi.h" || \
	  { echo "Missing AmiAPI public headers under $(AMIAPI_INCLUDE)"; exit 1; }
	@test -f "$(AMIAPI_LIB)" || \
	  { echo "Missing AmiAPI library $(AMIAPI_LIB); build the matching platform in AmiAPI first"; exit 1; }

$(BUILD):
	mkdir -p $@

$(BUILD)/%.o: src/%.c | $(BUILD)
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(PROGRAM): $(COMMON_OBJ) $(PLATFORM_OBJ) $(AMIAPI_LIB)
	$(CC) $(CFLAGS) $(COMMON_OBJ) $(PLATFORM_OBJ) $(AMIAPI_LIB) $(LDLIBS) -o $@

$(DOOR_PROGRAM): $(COMMON_OBJ) $(DOOR_OBJ) $(DOOR_PLATFORM_OBJ) $(AMIAPI_LIB)
	$(CC) $(CFLAGS) $(COMMON_OBJ) $(DOOR_OBJ) $(DOOR_PLATFORM_OBJ) $(AMIAPI_LIB) -o $@

test: check-sdk
ifeq ($(PLATFORM),posix)
	$(MAKE) $(BUILD)/test_openmeteo PLATFORM=posix AMIAPI_DIR="$(AMIAPI_DIR)"
	$(BUILD)/test_openmeteo
	$(MAKE) $(BUILD)/test_preferences PLATFORM=posix AMIAPI_DIR="$(AMIAPI_DIR)"
	$(BUILD)/test_preferences
else
	@echo "Fixture tests run on the POSIX host: make PLATFORM=posix test"
endif

door-test: check-sdk
ifeq ($(PLATFORM),posix)
	$(MAKE) $(BUILD)/test_door PLATFORM=posix AMIAPI_DIR="$(AMIAPI_DIR)"
	$(BUILD)/test_door
else
	@echo "Door fixture tests run on the POSIX host: make PLATFORM=posix door-test"
endif

$(BUILD)/test_door: tests/test_door.c $(DOOR_OBJ) $(COMMON_OBJ) $(AMIAPI_LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(DOOR_OBJ) $(COMMON_OBJ) $(AMIAPI_LIB) $(LDLIBS) -lm -o $@

$(BUILD)/test_openmeteo: tests/test_openmeteo.c $(COMMON_OBJ) $(AMIAPI_LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(COMMON_OBJ) $(AMIAPI_LIB) $(LDLIBS) -lm -o $@

$(BUILD)/test_preferences: tests/test_preferences.c $(COMMON_OBJ) $(AMIAPI_LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(COMMON_OBJ) $(AMIAPI_LIB) $(LDLIBS) -lm -o $@

smoke: all
ifeq ($(PLATFORM),posix)
	$(PROGRAM)
else
	@echo "Run the Amiga executable on a configured AmigaOS system."
endif

release: PLATFORM := amiga
release: all
	sh scripts/make_release.sh
	sh scripts/validate_release.sh

validate-release:
	sh scripts/validate_release.sh

clean:
	rm -rf build
