# ps5-syslang

Small PS5 payload for switching the ShellUI system language.

**Build:**  
It needs [ps5-payload-dev/sdk](https://github.com/ps5-payload-dev/sdk).  
The required `elfldr` source files are vendored in `third_party/elfldr`.

```bash
make
```

By default, the Makefile uses the vendored `elfldr` files. Override it if you want to build against another checkout:

```bash
make ELFLDR_DIR=/path/to/ps5-elfldr
```

**Output:**  

```text
ps5-syslang.elf
```

**Usage:**  
Send `ps5-syslang.elf` to `ps5-elfldr` (9021 port or `Payload Manager`). The loader finds `SceShellUI` and injects the embedded hook.

**Warnings:**  
- This payload may trigger a ShellUI software error, even when the language change succeeds. Avoid sending it repeatedly; repeated injections appear to increase the chance of an error.
- On JP region-locked consoles, the system language would revert to Japanese after a reboot. Other regions should keep the changed language.

**Config:**  
Config file search order:

```text
/mnt/usb0/syslang.ini ... /mnt/usb7/syslang.ini
/data/syslang.ini
```

The first config file found wins. USB configs are checked in order from `/mnt/usb0` to `/mnt/usb7`; `/data/syslang.ini` is only used if no USB config exists.

If no config file is found, it uses Simplified Chinese (`language_id=11`).
See `syslang.ini.example` for all known language IDs.

**Others:**  
Notifications are only shown for errors or when the target language is already active.

**Credits:**  
- **[etaHEN](https://github.com/etaHEN/etaHEN):** For the method to hook ShellUI.
- **[ps5-payload-dev/elfldr](https://github.com/ps5-payload-dev/elfldr):** For the method to spawn the hook payload.
- **[ps5-payload-dev/sdk](https://github.com/ps5-payload-dev/sdk):** For building the payload.
- **老拾:** For providing Y2JB backup with the JP region account and testing on FW 12.20.

**License:**  
GPL-3.0-or-later
