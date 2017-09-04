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
_python_project_supported(const char *path)
{
   return _relative_path_exists(path, "setup.py");
}

static Eina_Bool
_python_file_hidden_is(const char *file)
{
   if (!file || strlen(file) == 0)
     return EINA_FALSE;

   if (eina_str_has_extension(file, ".pyc") || eina_str_has_extension(file, ".pyo"))
     return EINA_TRUE;

   return EINA_FALSE;
}

static Eina_Bool
_python_project_runnable_is(const char *file EINA_UNUSED)
{
   return EINA_TRUE;
}

static int
_python_build(void)
{
   if (chdir(edi_project_get()) == 0)
     return edi_exe_wait("./setup.py build");

   return -1;
}

static void
_python_test(void)
{
   if (chdir(edi_project_get()) == 0)
     _exec_cmd("./setup.py test");
}

static void
_python_run(const char *path EINA_UNUSED, const char *args EINA_UNUSED)
{
   if (chdir(edi_project_get()) == 0)
     _exec_cmd("./setup.py run");
}

static void
_python_clean(void)
{
   if (chdir(edi_project_get()) == 0)
     _exec_cmd("./setup.py clean --all");
}

Edi_Build_Provider _edi_build_provider_python =
   {
      "python",
      _python_project_supported,
      _python_file_hidden_is,
      _python_project_runnable_is,
      _python_build,
      _python_test,
      _python_run,
      _python_clean
   };
