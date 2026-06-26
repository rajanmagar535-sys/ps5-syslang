// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "elfldr.h"
#include "pt.h"

extern unsigned char shellui_hook_elf[];
extern unsigned int shellui_hook_elf_len;
extern int sceSystemServiceParamGetInt(int param_id, int *value);
extern int sceKernelSendNotificationRequest(int device, void *request, size_t size, int blocking);
extern int sceKernelDebugOutText(int channel, const char *text);

typedef struct HookConfig {
  uint32_t magic;
  int32_t language_id;
} HookConfig;

#define HOOK_CONFIG_MAGIC 0x534c414eU
#define DEFAULT_LANGUAGE_ID 11
#define SCE_SYSTEM_SERVICE_PARAM_ID_LANG 1
// #define LOG_PREFIX "[ps5-syslang-loader]"
#define NOTIFY_PREFIX "PS5 SysLang:"

typedef struct NotifyRequest {
  char reserved[45];
  char message[3075];
} NotifyRequest;

typedef struct LanguageText {
  const char *name;
  const char *already_fmt;
} LanguageText;

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

static const LanguageText kLanguageTexts[] = {
    {"日本語", "すでに%sです"},
    {"English (United States)", "Already %s"},
    {"Français", "Déjà en %s"},
    {"Español", "Ya está en %s"},
    {"Deutsch", "Bereits %s"},
    {"Italiano", "Già %s"},
    {"Nederlands", "Al %s"},
    {"Português (Portugal)", "Já está em %s"},
    {"Русский", "Уже %s"},
    {"한국어", "이미 %s입니다"},
    {"繁體中文", "已經是%s"},
    {"简体中文", "已经是%s"},
    {"Suomi", "Jo %s"},
    {"Svenska", "Redan %s"},
    {"Dansk", "Allerede %s"},
    {"Norsk", "Allerede %s"},
    {"Polski", "Już %s"},
    {"Português (Brasil)", "Já está em %s"},
    {"English (United Kingdom)", "Already %s"},
    {"Türkçe", "Zaten %s"},
    {"Español (Latinoamérica)", "Ya está en %s"},
    {"العربية", "بالفعل %s"},
    {"Français (Canada)", "Déjà en %s"},
    {"Čeština", "Již %s"},
    {"Magyar", "Már %s"},
    {"Ελληνικά", "Ήδη %s"},
    {"Română", "Deja %s"},
    {"ไทย", "เป็น%sอยู่แล้ว"},
    {"Tiếng Việt", "Đã là %s"},
    {"Bahasa Indonesia", "Sudah %s"},
};

static int is_valid_language_id(int language_id);
/*
static void log_line(const char *fmt, ...)
{
  (void)fmt;
  char line[512];
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(line, sizeof(line), fmt, ap);
  va_end(ap);

  char output[sizeof(line) + sizeof(LOG_PREFIX) + 4];
  snprintf(output, sizeof(output), LOG_PREFIX " %s\n", line);
  sceKernelDebugOutText(0, output);
}
*/
static void notify(const char *fmt, ...)
{
  NotifyRequest request;
  char message[sizeof(request.message)];
  va_list ap;

  memset(&request, 0, sizeof(request));
  va_start(ap, fmt);
  vsnprintf(message, sizeof(message), fmt, ap);
  va_end(ap);

  snprintf(request.message, sizeof(request.message), NOTIFY_PREFIX " %s", message);
  sceKernelSendNotificationRequest(0, &request, sizeof(request), 0);
}

static const char *language_name_from_id(int language_id)
{
  if (!is_valid_language_id(language_id)) {
    return "Unknown";
  }

  return kLanguageTexts[language_id].name;
}

static const char *already_format_from_id(int language_id)
{
  if (!is_valid_language_id(language_id)) {
    return NULL;
  }

  return kLanguageTexts[language_id].already_fmt;
}

static void notify_already_language(int language_id)
{
  const char *fmt = already_format_from_id(language_id);
  if (!fmt) {
    notify("invalid language_id");
    return;
  }

  notify(fmt, language_name_from_id(language_id));
}

static char *trim_ascii(char *s)
{
  while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') {
    s++;
  }

  char *end = s + strlen(s);
  while (end > s &&
         (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) {
    end--;
  }
  *end = '\0';
  return s;
}

static int is_valid_language_id(int language_id)
{
  return language_id >= 0 && language_id < (int)ARRAY_SIZE(kLanguageTexts);
}

static int parse_language_id_file(const char *path, int *language_id)
{
  FILE *fp = fopen(path, "r");
  if (!fp) {
    return 0;
  }

  char line[128];
  while (fgets(line, sizeof(line), fp)) {
    char *p = trim_ascii(line);
    if (*p == '#' || *p == ';' || *p == '\0') {
      continue;
    }

    char *comment = strchr(p, '#');
    char *semicolon = strchr(p, ';');
    if (comment && (!semicolon || comment < semicolon)) {
      *comment = '\0';
    } else if (semicolon) {
      *semicolon = '\0';
    }

    p = trim_ascii(p);
    char *equals = strchr(p, '=');
    if (!equals) {
      continue;
    }

    *equals = '\0';
    char *key = trim_ascii(p);
    char *value_text = trim_ascii(equals + 1);
    if (strcmp(key, "language_id") != 0 || *value_text == '\0') {
      continue;
    }

    char *end = NULL;
    long value = strtol(value_text, &end, 10);
    end = trim_ascii(end);
    if (*end != '\0') {
      notify("invalid language_id in %s", path);
      continue;
    }

    if (!is_valid_language_id((int)value)) {
      notify("language_id out of range");
      fclose(fp);
      return 0;
    }

    *language_id = (int)value;
    fclose(fp);
    // log_line("loaded language_id=%d from %s", *language_id, path);
    return 1;
  }

  fclose(fp);
  return 0;
}

static int load_language_id(void)
{
  int language_id = DEFAULT_LANGUAGE_ID;
  char path[64];

  for (int i = 0; i <= 7; i++) {
    snprintf(path, sizeof(path), "/mnt/usb%d/syslang.ini", i);
    if (parse_language_id_file(path, &language_id)) {
      return language_id;
    }
  }

  if (parse_language_id_file("/data/syslang.ini", &language_id)) {
    return language_id;
  }

  // log_line("syslang.ini not found; using default language_id=%d", language_id);
  return language_id;
}

static HookConfig *find_hook_config(unsigned char *elf, unsigned int elf_len)
{
  HookConfig needle = {HOOK_CONFIG_MAGIC, DEFAULT_LANGUAGE_ID};

  if (elf_len < sizeof(needle)) {
    return NULL;
  }

  for (unsigned int i = 0; i <= elf_len - sizeof(needle); i++) {
    if (memcmp(elf + i, &needle, sizeof(needle)) == 0) {
      return (HookConfig *)(elf + i);
    }
  }

  return NULL;
}

int main(void)
{
  int language_id = load_language_id();
  int current_language_id = -1;
  HookConfig *hook_config;

  if (sceSystemServiceParamGetInt(SCE_SYSTEM_SERVICE_PARAM_ID_LANG,
                                  &current_language_id) == 0) {
    if (current_language_id == language_id) {
      notify_already_language(language_id);
      return 0;
    }

    // log_line("current language=%d target=%d", current_language_id, language_id);
  }// else {
    // log_line("failed to query current language via sceSystemServiceParamGetInt(%d); continuing", SCE_SYSTEM_SERVICE_PARAM_ID_LANG);
  //}

  if (elfldr_sanity_check(shellui_hook_elf, shellui_hook_elf_len)) {
    notify("embedded hook ELF is invalid");
    return 1;
  }

  hook_config = find_hook_config(shellui_hook_elf, shellui_hook_elf_len);
  if (!hook_config) {
    notify("hook config block not found");
    return 1;
  }
  hook_config->language_id = language_id;

  // log_line("finding SceShellUI");

  pid_t pid = elfldr_find_pid("SceShellUI");
  if (pid < 0) {
    notify("SceShellUI not found");
    return 1;
  }

  if (pt_attach(pid)) {
    notify("pt_attach failed");
    return 1;
  }

  // log_line("injecting hook ELF into pid=%d", pid);

  if (elfldr_exec(pid, -1, shellui_hook_elf)) {
    notify("elfldr_exec failed");
    return 1;
  }

  // log_line("injected; language switch hook is active");
  return 0;
}
