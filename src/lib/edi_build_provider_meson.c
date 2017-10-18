#ifdef HAVE_CONFIG
# include "config.h"
#endif

#include <unistd.h>
#include <Eina.h>
#include <Ecore.h>
#include <Ecore_File.h>

#include "Edi.h"

#include "edi_private.h"

typedef struct {
   char *basedir;
   char *builddir;
   char *fulldir; /* basedir + builddir */
} Meson_Data;

static Eina_Bool
_meson_project_supported(const char *path)
{
   return edi_path_relative_exists(path, "meson.build");
}

static Meson_Data *
_meson_data_get(void)
{
   // FIXME: I hate static globals
   // FIXME: this needs to be freed on project change / shutdown!
   static Meson_Data *md = NULL;

   if (md) return md;

   md = calloc(1, sizeof(*md));
   md->basedir = strdup(edi_project_get());
   md->builddir = strdup("build"); // FIXME: "build" dir needs to be user-defined
   md->fulldir = edi_path_append(md->basedir, md->builddir);
   return md;
}

static Eina_Bool
_meson_configured_check(const char *dir)
{
   const char *build_ninja;

   build_ninja = eina_slstr_steal_new(edi_path_append(dir, "build.ninja"));
   if (ecore_file_exists(build_ninja))
     return EINA_TRUE;

   return EINA_FALSE;
}

static Eina_Bool
_meson_file_hidden_is(const char *file)
{
   if (!file || strlen(file) == 0)
     return EINA_FALSE;

   static const char *hidden_exts[] = {
      ".o", ".so", ".lo",
      ".ninja", ".ninja", ".ninja_deps", ".ninja_log",
      "compile_commands.json", "meson-logs", "meson-private", "@exe"
   };

   if (ecore_file_is_dir(file) && _meson_configured_check(file))
     return EINA_TRUE;

   for (size_t k = 0; k < EINA_C_ARRAY_LENGTH(hidden_exts); k++)
     if (eina_str_has_extension(file, hidden_exts[k]))
       return EINA_TRUE;

   return EINA_FALSE;
}

static Eina_Bool
_meson_project_runnable_is(const char *path)
{
   return ecore_file_can_exec(path);
}

static const char *
_meson_ninja_cmd(Meson_Data *md, const char *arg)
{
   return eina_slstr_printf("ninja -C %s %s", md->fulldir, arg ?: "");
}

static void
_meson_ninja_do(Meson_Data *md, const char *arg)
{
   const char *cmd;

   cmd = _meson_ninja_cmd(md, arg);
   if (arg && !strcmp(arg, "clean"))
     edi_exe_notify("edi_clean", cmd);
   else if (arg && !strcmp(arg, "test"))
     edi_exe_notify("edi_test", cmd);
   else
     edi_exe_notify("edi_build", cmd);
}

static Eina_Bool
_meson_prepare_end(void *data, int evtype EINA_UNUSED, void *evinfo)
{
   Meson_Data *me = data;
   Ecore_Exe_Event_Del *ev = evinfo;

   if (!ev->exe) return ECORE_CALLBACK_RENEW;
   if (ecore_exe_data_get(ev->exe) != me) return ECORE_CALLBACK_RENEW;

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_meson_prepare(Meson_Data *md)
{
   const char *cmd;
   Ecore_Exe *exe;

   if (_meson_configured_check(md->fulldir)) return EINA_TRUE;
   if (chdir(md->basedir) != 0) return EINA_FALSE;

   cmd = eina_slstr_printf("meson %s && %s", md->builddir, _meson_ninja_cmd(md, ""));

   exe = ecore_exe_pipe_run(cmd,
                            ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                            ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                            ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, md);

   if (!exe) return EINA_FALSE;
   ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _meson_prepare_end, md);

   return EINA_FALSE;
}

static void
_meson_build(void)
{
   Meson_Data *md = _meson_data_get();

   if (!_meson_prepare(md))
     return;

   _meson_ninja_do(md, NULL);
}

static void
_meson_test(void)
{
   Meson_Data *md = _meson_data_get();

   //if (!_meson_configured_check(md->fulldir)) return;
   _meson_ninja_do(md, "test");
}

static void
_meson_run(const char *path, const char *args)
{
   Meson_Data *md = _meson_data_get();
   const char *cmd;

   if (chdir(edi_project_get()) != 0)
     ERR("Could not chdir");

   if (args) cmd = eina_slstr_printf("%s %s", path, args);
   else cmd = path;
   ecore_exe_pipe_run(cmd,
                      ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                      ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                      ECORE_EXE_PIPE_WRITE /*| ECORE_EXE_USE_SH*/, md);
}

static void
_meson_clean(void)
{
   Meson_Data *md = _meson_data_get();

   //if (!_meson_configured_check(md->fulldir)) return;
   _meson_ninja_do(md, "clean");
}

Edi_Build_Provider _edi_build_provider_meson =
   {"meson", _meson_project_supported, _meson_file_hidden_is,
    _meson_project_runnable_is, _meson_build, _meson_test,
    _meson_run, _meson_clean};
