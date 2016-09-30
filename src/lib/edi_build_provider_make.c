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
   const char *path;
   Eina_Bool ret;

   path = edi_path_append(base, relative);
   ret = ecore_file_exists(path);

   free((void *)path);
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
_make_file_hidden_is(const char *relative)
{
   int len;

   if (!relative || strlen(relative) == 0)
     return EINA_FALSE;

   len = strlen(relative);
   if (relative[len-1] == 'o' && len >= 2)
     {
        if (relative[len-2] == '.')
          return EINA_TRUE;
        else
          {
            if ((relative[len-2] == 's' || relative[len-2] == 'l') &&
              len >= 3 && relative[len-3] == '.')
              return EINA_TRUE;
          }
     }
   if (relative[len-1] == 'a' && len >= 2)
     {
        if (relative[len-2] == '.')
          return EINA_TRUE;
        else
          {
            if ((relative[len-2] == 'l') &&
              len >= 3 && relative[len-3] == '.')
              return EINA_TRUE;
          }
     }
   return EINA_FALSE;
}

static void
_make_build_make(void)
{
   chdir(edi_project_get());
   ecore_exe_pipe_run("make", ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                              ECORE_EXE_USE_SH, NULL);
}

static void
_make_build_configure(void)
{
   chdir(edi_project_get());
   ecore_exe_pipe_run("./configure && make",
                              ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                              ECORE_EXE_USE_SH, NULL);
}

static void
_make_build_cmake(void)
{
   chdir(edi_project_get());
   ecore_exe_pipe_run("mkdir -p build && cd build && cmake .. && make && cd ..",
                              ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                              ECORE_EXE_USE_SH, NULL);
}

static void
_make_build_autogen(void)
{
   chdir(edi_project_get());
   ecore_exe_pipe_run("./autogen.sh && make",
                              ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                              ECORE_EXE_USE_SH, NULL);
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
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR, NULL);
}

static void
_make_clean(void)
{
   chdir(edi_project_get());
   ecore_exe_pipe_run("make clean", ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR, NULL);
}

Edi_Build_Provider _edi_build_provider_make =
   {"make", _make_project_supported, _make_file_hidden_is,
     _make_build, _make_test, _make_clean};
