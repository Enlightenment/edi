#ifdef HAVE_CONFIG
# include "config.h"
#endif

#include <unistd.h>
#include <Eina.h>
#include <Ecore.h>
#include <Ecore_File.h>

#include "Edi.h"

#include "edi_private.h"

#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__OpenBSD__) || defined(__NetBSD__)
 #define MAKE_COMMAND " gmake"
#else
 #define MAKE_COMMAND " make -w"
#endif


static Eina_Bool
_make_project_supported(const char *path)
{
   return edi_path_relative_exists(path, "Makefile") ||
          edi_path_relative_exists(path, "makefile") ||
          edi_path_relative_exists(path, "configure") ||
          edi_path_relative_exists(path, "autogen.sh");
}

static Eina_Bool
_make_file_hidden_is(const char *file)
{
   if (!file || strlen(file) == 0)
     return EINA_FALSE;

   if (eina_str_has_extension(file, ".o") || eina_str_has_extension(file, ".so") ||
       eina_str_has_extension(file, ".lo"))
     return EINA_TRUE;
   if (eina_str_has_extension(file, ".a") || eina_str_has_extension(file, ".la"))
     return EINA_TRUE;

   if (!strcmp(ecore_file_file_get(file), "autom4te.cache"))
     return EINA_TRUE;

   return EINA_FALSE;
}

static Eina_Bool
_make_project_runnable_is(const char *path)
{
   if (!path || !path[0])
     return EINA_FALSE;

   return ecore_file_exists(path);
}

static const char *
_make_comand_compound_get(const char *prepend, const char *append)
{
   const char *cmd;
   Eina_Strbuf *buf;

   buf = eina_strbuf_new();
   eina_strbuf_append_printf(buf, "%s" BEAR_COMMAND MAKE_COMMAND " -j %d %s", prepend, eina_cpu_count(), append);
   cmd = eina_strbuf_release(buf);

   return cmd;
}

static void
_make_build_make(void)
{
   static const char *cmd = NULL;
   if (!cmd)
     cmd = _make_comand_compound_get("", "");

   if (chdir(edi_project_get()) != 0)
     ERR("Could not chdir");

   edi_exe_notify("edi_build", cmd);
}

static void
_make_build_configure(void)
{
   static const char *cmd = NULL;
   if (!cmd)
     cmd = _make_comand_compound_get("./configure && ", "");

   if (chdir(edi_project_get()) != 0)
     ERR("Could not chdir");

   edi_exe_notify("edi_build", cmd);
}

static void
_make_build_autogen(void)
{
   static const char *cmd = NULL;
   if (!cmd)
     cmd = _make_comand_compound_get("./autogen.sh && ", "");

   if (chdir(edi_project_get()) != 0)
     ERR("Could not chdir");

   edi_exe_notify("edi_build", cmd);
}

static void
_make_build(void)
{
   if (edi_project_file_exists("Makefile") || edi_project_file_exists("makefile"))
     _make_build_make();
   else if (edi_project_file_exists("configure"))
     _make_build_configure();
   else if (edi_project_file_exists("autogen.sh"))
     _make_build_autogen();
}

static void
_make_test(void)
{
   static const char *cmd = NULL;
   if (!cmd)
     cmd = _make_comand_compound_get("env CK_VERBOSITY=verbose ", "check");

   if (chdir(edi_project_get()) != 0)
     ERR("Could not chdir");
   chdir(edi_project_get());

   edi_exe_notify("edi_test", cmd);
}

static void
_make_run(const char *path, const char *args)
{
   char *full_cmd;
   int full_len;

   if (!path) return;
   if (chdir(edi_project_get()) !=0)
     ERR("Could not chdir");

   if (!args)
     {
        ecore_exe_pipe_run(path, ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                                 ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                                 ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);

        return;
     }

   full_len = strlen(path) + strlen(args) + 1;
   full_cmd = malloc(sizeof(char) * (full_len + 1));
   snprintf(full_cmd, full_len + 1, "%s %s", path, args);

   ecore_exe_pipe_run(full_cmd, ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                                ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                                ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);

   free(full_cmd);
}

static void
_make_clean(void)
{
   static const char *cmd = NULL;
   if (!cmd)
     cmd = _make_comand_compound_get("", "clean");

   if (chdir(edi_project_get()) !=0)
     ERR("Could not chdir");

   edi_exe_notify("edi_clean", cmd);
}

Edi_Build_Provider _edi_build_provider_make =
   {"make", _make_project_supported, _make_file_hidden_is, _make_project_runnable_is,
     _make_build, _make_test, _make_run, _make_clean};
