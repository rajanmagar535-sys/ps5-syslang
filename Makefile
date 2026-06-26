PS5_HOST ?= ps5
PS5_PORT ?= 9021

PS5_PAYLOAD_SDK ?= /opt/ps5-payload-sdk
include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk

ELF := ps5-syslang.elf
HOOK_ELF := shellui-hook.elf
EMBEDDED := shellui_hook_elf.c
ELFLDR_DIR ?= third_party/elfldr

CXXFLAGS := -Wall -Werror -g -O0 -std=c++17 -I.
CFLAGS := -Wall -Werror -g -O0 -I$(ELFLDR_DIR)
HOOK_LDLIBS := -lkernel_sys
LDLIBS := -lSceSystemService -lkernel_sys

all: $(ELF)

$(HOOK_ELF): main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(HOOK_LDLIBS)

$(EMBEDDED): $(HOOK_ELF)
	xxd -i $< > $@

$(ELF): loader.c $(EMBEDDED) $(ELFLDR_DIR)/elfldr.c $(ELFLDR_DIR)/pt.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f $(ELF) $(HOOK_ELF) $(EMBEDDED)

test: $(ELF)
	$(PS5_DEPLOY) -h $(PS5_HOST) -p $(PS5_PORT) $^
