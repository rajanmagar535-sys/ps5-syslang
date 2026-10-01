// SPDX-License-Identifier: GPL-3.0-or-later

#include <ps5/kernel.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct MonoDomain;
struct MonoAssembly;
struct MonoImage;
struct MonoClass;
struct MonoMethod;
struct MonoObject;
struct MonoThread;

static MonoDomain *g_root_domain;

#ifndef LANGUAGE_ID
#define LANGUAGE_ID 11
#endif

#if LANGUAGE_ID < 0 || LANGUAGE_ID > 30
#error "LANGUAGE_ID must be in the 0-30 range"
#endif

static MonoDomain *(*mono_get_root_domain)(void);
static MonoThread *(*mono_thread_attach)(MonoDomain *);
static MonoAssembly *(*mono_domain_assembly_open)(MonoDomain *, const char *);
static MonoImage *(*mono_assembly_get_image)(MonoAssembly *);
static MonoClass *(*mono_class_from_name)(MonoImage *, const char *, const char *);
static MonoObject *(*mono_object_new)(MonoDomain *, MonoClass *);
static MonoMethod *(*mono_class_get_method_from_name)(MonoClass *, const char *, int);
static MonoObject *(*mono_runtime_invoke)(MonoMethod *, void *, void **, MonoObject **);
static void (*mono_gchandle_new)(MonoObject *, int);

extern "C" int sceKernelDebugOutText(int channel, const char *text);

#define LOG_PREFIX "[ps5-syslang]"

static void log_line(const char *fmt, ...)
{
  char line[512];
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(line, sizeof(line), fmt, ap);
  va_end(ap);

  char output[sizeof(line) + sizeof(LOG_PREFIX) + 4];
  snprintf(output, sizeof(output), LOG_PREFIX " %s\n", line);
  sceKernelDebugOutText(0, output);
}

static void *resolve(uint32_t handle, const char *name)
{
  return (void *)kernel_dynlib_dlsym(-1, handle, name);
}

static bool resolve_mono(void)
{
  uint32_t mono_handle = 0;
  if (kernel_dynlib_handle(-1, "libmonosgen-2.0.sprx", &mono_handle) != 0) {
    return false;
  }

#define RESOLVE_MONO(name) (*(void **)(&name) = resolve(mono_handle, #name))
  RESOLVE_MONO(mono_get_root_domain);
  RESOLVE_MONO(mono_thread_attach);
  RESOLVE_MONO(mono_domain_assembly_open);
  RESOLVE_MONO(mono_assembly_get_image);
  RESOLVE_MONO(mono_class_from_name);
  RESOLVE_MONO(mono_object_new);
  RESOLVE_MONO(mono_class_get_method_from_name);
  RESOLVE_MONO(mono_runtime_invoke);
  RESOLVE_MONO(mono_gchandle_new);
#undef RESOLVE_MONO

  return mono_get_root_domain && mono_thread_attach && mono_domain_assembly_open &&
         mono_assembly_get_image && mono_class_from_name && mono_object_new &&
         mono_class_get_method_from_name && mono_runtime_invoke && mono_gchandle_new;
}

static MonoObject *invoke_method(MonoMethod *method, void *instance, void **args,
                                 const char *label)
{
  MonoObject *exception = nullptr;
  MonoObject *result = mono_runtime_invoke(method, instance, args, &exception);
  if (exception) {
    log_line("%s exception", label);
    return nullptr;
  }
  // log_line("%s returned %p", label, result);
  return result;
}

static int configured_language_id(void)
{
  return LANGUAGE_ID;
}

static void change_system_language(MonoImage *shellapp_image)
{
  MonoClass *language_class = mono_class_from_name(
      shellapp_image, "ReactNative.Modules.ShellUI.Settings", "LanguageModule");
  if (!language_class) {
    log_line("LanguageModule not found");
    return;
  }

  MonoObject *language = mono_object_new(g_root_domain, language_class);
  if (!language) {
    log_line("LanguageModule allocation failed");
    return;
  }
  mono_gchandle_new(language, 1);

  MonoMethod *ctor = mono_class_get_method_from_name(language_class, ".ctor", 0);
  if (ctor) {
    invoke_method(ctor, language, nullptr, "LanguageModule::.ctor()");
  }

  int language_id = configured_language_id();
  // log_line("configured language_id=%d", language_id);
  MonoMethod *change = mono_class_get_method_from_name(
      language_class, "ChangeSystemLanguage", 2);
  if (!change) {
    log_line("ChangeSystemLanguage not found");
    return;
  }

  void *change_args[] = {&language_id, nullptr};
  invoke_method(change, language, change_args, "ChangeSystemLanguage");

  MonoMethod *reload = mono_class_get_method_from_name(
      language_class, "ReloadApplicationForChangeSystemLanguage", 1);
  if (!reload) {
    log_line("ReloadApplication not found");
    return;
  }

  void *reload_args[] = {nullptr};
  invoke_method(reload, language, reload_args, "ReloadApplicationForChangeSystemLanguage");
}

int main(void)
{
  // log_line("starting");

  if (!resolve_mono()) {
    log_line("not running in SceShellUI");
    return 1;
  }

  g_root_domain = mono_get_root_domain();
  if (!g_root_domain) {
    log_line("Mono domain not found");
    return 1;
  }
  mono_thread_attach(g_root_domain);

  MonoAssembly *shellapp = mono_domain_assembly_open(
      g_root_domain,
      "/system_ex/common_ex/lib/Sce.Vsh.ShellUI.ReactNativeShellApp.dll.sprx");
  MonoImage *shellapp_image = shellapp ? mono_assembly_get_image(shellapp) : nullptr;
  if (!shellapp_image) {
    log_line("ReactNativeShellApp not found");
    return 1;
  }

  change_system_language(shellapp_image);
  // log_line("done");
  return 0;
}
