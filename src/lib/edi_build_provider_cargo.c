#ifdef HAVE_CONFIG
# include "config.h"
#endif

#include <unistd.h>
#include <Ecore.h>
#include <Ecore_File.h>

#include "Edi.h"

#include "edi_private.h"

static Eina_Bool
_relative_path_exists(const char *base, const char *relative)
{
   char *path;
   Eina_Bool ret;

   path = edi_path_append(base, relative);
   ret = ecore_file_exists(path);

   free(path);
   return ret;
}

static void
_exec_cmd(const char *cmd)
{
   ecore_exe_pipe_run(cmd,
                      ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                      ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                      ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);
}

static Eina_Bool
_cargo_project_supported(const char *path)
{
   return _relative_path_exists(path, "Cargo.toml");
}

static Eina_Bool
_cargo_file_hidden_is(const char *file)
{
   if (!file || strlen(file) == 0)
     return EINA_FALSE;

   if (eina_str_has_extension(file, ".o") || !strcmp(ecore_file_file_get(file), "target"))
     return EINA_TRUE;

   return EINA_FALSE;
}

static Eina_Bool
_cargo_project_runnable_is(const char *file EINA_UNUSED)
{
   return EINA_TRUE;
}

static void
_cargo_build(void)
{
   if (chdir(edi_project_get()) == 0)
     edi_exe_notify("edi_build", "cargo build");
}

static void
_cargo_test(void)
{
   if (chdir(edi_project_get()) == 0)
     edi_exe_notify("edi_test", "cargo test");
}

static void
_cargo_run(const char *path EINA_UNUSED, const char *args EINA_UNUSED)
{
   if (chdir(edi_project_get()) == 0)
     _exec_cmd("cargo run");
}

static void
_cargo_clean(void)
{
   if (chdir(edi_project_get()) == 0)
     edi_exe_notify("edi_clean", "cargo clean");
}

Edi_Build_Provider _edi_build_provider_cargo =
   {
      "cargo",
      _cargo_project_supported,
      _cargo_file_hidden_is,
      _cargo_project_runnable_is,
      _cargo_build,
      _cargo_test,
      _cargo_run,
      _cargo_clean
   };
