PS5_HOST ?= ps5
PS5_PORT ?= 9021

PS5_PAYLOAD_SDK ?= /opt/ps5-payload-sdk
include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk

LANGUAGES ?= ja:0 en-US:1 fr-FR:2 es-ES:3 de:4 it:5 nl:6 pt-PT:7 ru:8 ko:9 zh-Hant:10 zh-Hans:11 fi:12 sv:13 da:14 no:15 pl:16 pt-BR:17 en-GB:18 tr:19 es-419:20 ar:21 fr-CA:22 cs:23 hu:24 el:25 ro:26 th:27 vi:28 in:29 uk:30
BUILD_DIR ?= build
INTERMEDIATE_DIR ?= hook-build
lang_code = $(word 1,$(subst :, ,$(1)))
lang_id = $(word 2,$(subst :, ,$(1)))
ELFS := $(foreach lang,$(LANGUAGES),$(BUILD_DIR)/ps5-syslang-$(call lang_code,$(lang)).elf)
HOOK_ELFS := $(foreach lang,$(LANGUAGES),$(INTERMEDIATE_DIR)/shellui-hook-$(call lang_code,$(lang)).elf)
EMBEDDEDS := $(foreach lang,$(LANGUAGES),$(INTERMEDIATE_DIR)/shellui-hook-$(call lang_code,$(lang)).c)
ELFLDR_DIR ?= third_party/elfldr

CXXFLAGS := -Wall -Werror -g -O0 -std=c++17 -I.
CFLAGS := -Wall -Werror -g -O0 -I$(ELFLDR_DIR)
HOOK_LDLIBS := -lkernel_sys
LDLIBS := -lSceSystemService -lkernel_sys

all: $(ELFS)

.SECONDARY: $(HOOK_ELFS) $(EMBEDDEDS)

$(BUILD_DIR):
	mkdir -p $@

$(INTERMEDIATE_DIR):
	mkdir -p $@

define BUILD_LANGUAGE
$(INTERMEDIATE_DIR)/shellui-hook-$(call lang_code,$(1)).elf: main.cpp | $(INTERMEDIATE_DIR)
	$$(CXX) $$(CXXFLAGS) -DLANGUAGE_ID=$(call lang_id,$(1)) -o $$@ $$^ $$(HOOK_LDLIBS)

$(INTERMEDIATE_DIR)/shellui-hook-$(call lang_code,$(1)).c: $(INTERMEDIATE_DIR)/shellui-hook-$(call lang_code,$(1)).elf
	xxd -i -n shellui_hook_elf $$< > $$@

$(BUILD_DIR)/ps5-syslang-$(call lang_code,$(1)).elf: loader.c $(INTERMEDIATE_DIR)/shellui-hook-$(call lang_code,$(1)).c $$(ELFLDR_DIR)/elfldr.c $$(ELFLDR_DIR)/pt.c | $(BUILD_DIR)
	$$(CC) $$(CFLAGS) -DLANGUAGE_ID=$(call lang_id,$(1)) -o $$@ $$^ $$(LDLIBS)
endef

$(foreach lang,$(LANGUAGES),$(eval $(call BUILD_LANGUAGE,$(lang))))

clean:
	rm -rf $(BUILD_DIR) $(INTERMEDIATE_DIR)

test: $(BUILD_DIR)/ps5-syslang-zh-Hans.elf
	$(PS5_DEPLOY) -h $(PS5_HOST) -p $(PS5_PORT) $^
