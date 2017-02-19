#ifdef HAVE_CONFIG
# include "config.h"
#endif

#include <unistd.h>
#include <Ecore.h>
#include <Ecore_File.h>

#include "Edi.h"

#include "edi_private.h"

static Eina_Bool
_cmake_project_supported(const char *path)
{
   return edi_path_relative_exists(path, "CMakeLists.txt");
}

static Eina_Bool
_cmake_file_hidden_is(const char *file)
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
_cmake_build(void)
{
   chdir(edi_project_get());
   ecore_exe_pipe_run("mkdir -p build && cd build && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 .. && make && cd ..",
                              ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                              ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);
}

static void
_cmake_test(void)
{
   chdir(edi_project_get());
   ecore_exe_pipe_run("env CK_VERBOSITY=verbose make check", ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                              ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);
}

static void
_cmake_clean(void)
{
   chdir(edi_project_get());
   ecore_exe_pipe_run("make clean", ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                              ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);
}

Edi_Build_Provider _edi_build_provider_cmake =
   {"cmake", _cmake_project_supported, _cmake_file_hidden_is,
     _cmake_build, _cmake_test, _cmake_clean};
