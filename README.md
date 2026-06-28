# ps5-syslang

Small PS5 payload for switching the ShellUI system language.

## Build
It needs [ps5-payload-dev/sdk](https://github.com/ps5-payload-dev/sdk).  
The required `elfldr` source files are vendored in `third_party/elfldr`.

```bash
make
```

This builds one fixed-language payload for every known language code. To build a
smaller set, override `LANGUAGES` with `code:id` pairs:

```bash
make LANGUAGES="en-US:1 zh-Hans:11"
```

By default, the Makefile uses the vendored `elfldr` files. Override it if you want to build against another checkout:

```bash
make ELFLDR_DIR=/path/to/ps5-elfldr
```

## Output

```text
build/ps5-syslang-ja.elf
build/ps5-syslang-en-US.elf
...
build/ps5-syslang-uk.elf
```

Intermediate hook build files are kept under `hook-build/`.

## Usage
Send the ELF for the target language code to `ps5-elfldr` (9021 port or `Payload Manager`). The loader finds `SceShellUI` and injects the embedded hook.

Examples:

```text
build/ps5-syslang-en-US.elf    English (United States)
build/ps5-syslang-zh-Hans.elf  Chinese (Simplified)
```

## Warnings
- This payload may trigger a ShellUI software error, even when the language change succeeds. Avoid sending it repeatedly; repeated injections appear to increase the chance of an error.
- On JP region-locked consoles, the system language would revert to Japanese after a reboot. Other regions should keep the changed language.

## Languages
Language IDs are fixed at build time. No runtime config file is read.

```text
 0 ja       Japanese
 1 en-US    English (United States)
 2 fr-FR    French (France)
 3 es-ES    Spanish (Spain)
 4 de       German
 5 it       Italian
 6 nl       Dutch
 7 pt-PT    Portuguese (Portugal)
 8 ru       Russian
 9 ko       Korean
10 zh-Hant  Traditional Chinese
11 zh-Hans  Simplified Chinese
12 fi       Finnish
13 sv       Swedish
14 da       Danish
15 no       Norwegian
16 pl       Polish
17 pt-BR    Portuguese (Brazil)
18 en-GB    English (United Kingdom)
19 tr       Turkish
20 es-419   Spanish (Latin America)
21 ar       Arabic
22 fr-CA    French (Canada)
23 cs       Czech
24 hu       Hungarian
25 el       Greek
26 ro       Romanian
27 th       Thai
28 vi       Vietnamese
29 in       Indonesian
30 uk       Ukrainian
```

## Others
Notifications are only shown for errors or when the target language is already active.

## Credits
- **[etaHEN](https://github.com/etaHEN/etaHEN):** For the method to hook ShellUI.
- **[ps5-payload-dev/elfldr](https://github.com/ps5-payload-dev/elfldr):** For the method to spawn the hook payload.
- **[ps5-payload-dev/sdk](https://github.com/ps5-payload-dev/sdk):** For building the payload.
- **老拾:** For providing Y2JB backup with the JP region account and testing on FW 12.20.

## License
GPL-3.0-or-later
