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

#ifndef LANGUAGE_ID
#define LANGUAGE_ID 11
#endif

#if LANGUAGE_ID < 0 || LANGUAGE_ID > 30
#error "LANGUAGE_ID must be in the 0-30 range"
#endif

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
    {"Українська", "Вже %s"},
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

static int is_valid_language_id(int language_id)
{
  return language_id >= 0 && language_id < (int)ARRAY_SIZE(kLanguageTexts);
}

int main(void)
{
  int language_id = LANGUAGE_ID;
  int current_language_id = -1;

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
