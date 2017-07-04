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

   if (!strcmp("build", ecore_file_file_get(file)))
     return EINA_TRUE;

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
_cmake_project_runnable_is(const char *path)
{
   if (!path || !path[0])
     return EINA_FALSE;

   return ecore_file_exists(path);
}

static void
_cmake_build(void)
{
   if (chdir(edi_project_get()) != 0)
     ERR("Could not chdir");
   ecore_exe_pipe_run("mkdir -p build && cd build && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 .. && make && cd ..",
                              ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                              ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);
}

static void
_cmake_test(void)
{
   if (chdir(edi_project_get()) != 0)
     ERR("Could not chdir");
   ecore_exe_pipe_run("env CK_VERBOSITY=verbose make check", ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                              ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);
}

static void
_cmake_run(const char *path, const char *args)
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

   full_len = strlen(path) + strlen(path);
   full_cmd = malloc(sizeof(char) * (full_len + 1));
   snprintf(full_cmd, full_len + 2, "%s %s", path, args);

   ecore_exe_pipe_run(full_cmd, ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                                ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                                ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);

   free(full_cmd);
}

static void
_cmake_clean(void)
{
   if (chdir(edi_project_get()) != 0)
     ERR("Could not chdir");
   ecore_exe_pipe_run("make clean", ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                              ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);
}

Edi_Build_Provider _edi_build_provider_cmake =
   {"cmake", _cmake_project_supported, _cmake_file_hidden_is, _cmake_project_runnable_is,
     _cmake_build, _cmake_test, _cmake_run, _cmake_clean};
