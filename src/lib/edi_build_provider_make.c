#ifdef HAVE_CONFIG
# include "config.h"
#endif

#include <unistd.h>
#include <Ecore.h>
#include <Ecore_File.h>

#include "Edi.h"

#include "edi_private.h"

#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__OpenBSD__) || defined(__NetBSD__)
 #define MAKE_COMMAND " gmake"
#else
 #define MAKE_COMMAND " make"
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

   return EINA_FALSE;
}

static void
_make_build_make(void)
{
   if (chdir(edi_project_get()) != 0)
     ERR("Could not chdir");
   ecore_exe_pipe_run(BEAR_COMMAND MAKE_COMMAND, ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                              ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);
}

static void
_make_build_configure(void)
{
   if (chdir(edi_project_get()) != 0)
     ERR("Could not chdir");
   ecore_exe_pipe_run("./configure && " BEAR_COMMAND MAKE_COMMAND,
                              ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                              ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);
}

static void
_make_build_autogen(void)
{
   if (chdir(edi_project_get()) != 0)
     ERR("Could not chdir");
   ecore_exe_pipe_run("./autogen.sh && " BEAR_COMMAND MAKE_COMMAND,
                              ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                              ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);
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
   if (chdir(edi_project_get()) != 0)
     ERR("Could not chdir");
   ecore_exe_pipe_run("env CK_VERBOSITY=verbose make check", ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                              ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);
}

static void
_make_clean(void)
{
   if (chdir(edi_project_get()) !=0)
     ERR("Could not chdir");
   ecore_exe_pipe_run("make clean", ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                              ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);
}

Edi_Build_Provider _edi_build_provider_make =
   {"make", _make_project_supported, _make_file_hidden_is,
     _make_build, _make_test, _make_clean};
