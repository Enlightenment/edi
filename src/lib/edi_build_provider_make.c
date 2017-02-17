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

static Eina_Bool
_make_project_supported(const char *path)
{
   return _relative_path_exists(path, "Makefile") ||
          _relative_path_exists(path, "configure") ||
          _relative_path_exists(path, "CMakeLists.txt") || // TODO move this one to a cmake file
          _relative_path_exists(path, "autogen.sh");
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
   chdir(edi_project_get());
   ecore_exe_pipe_run(BEAR_COMMAND " make", ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                              ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);
}

static void
_make_build_configure(void)
{
   chdir(edi_project_get());
   ecore_exe_pipe_run("./configure && " BEAR_COMMAND " make",
                              ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                              ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);
}

static void
_make_build_cmake(void)
{
   chdir(edi_project_get());
   ecore_exe_pipe_run("mkdir -p build && cd build && cmake .. && " BEAR_COMMAND " make && cd ..",
                              ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                              ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);
}

static void
_make_build_autogen(void)
{
   chdir(edi_project_get());
   ecore_exe_pipe_run("./autogen.sh && " BEAR_COMMAND " make",
                              ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                              ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);
}

static void
_make_build(void)
{
   if (edi_project_file_exists("Makefile"))
     _make_build_make();
   else if (edi_project_file_exists("configure"))
     _make_build_configure();
   else if (edi_project_file_exists("CMakeLists.txt"))
     _make_build_cmake();
   else if (edi_project_file_exists("autogen.sh"))
     _make_build_autogen();
}

static void
_make_test(void)
{
   chdir(edi_project_get());
   ecore_exe_pipe_run("env CK_VERBOSITY=verbose make check", ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                              ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);
}

static void
_make_clean(void)
{
   chdir(edi_project_get());
   ecore_exe_pipe_run("make clean", ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                              ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);
}

Edi_Build_Provider _edi_build_provider_make =
   {"make", _make_project_supported, _make_file_hidden_is,
     _make_build, _make_test, _make_clean};
