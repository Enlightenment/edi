#ifdef HAVE_CONFIG
# include "config.h"
#endif

#include <unistd.h>
#include <Ecore.h>
#include <Ecore_File.h>

#include "Edi.h"

#include "edi_private.h"

static Eina_Bool
_go_project_supported(const char *path)
{
   Eina_List *files, *l;
   const char *name;

   files = ecore_file_ls(path);

   EINA_LIST_FOREACH(files, l, name)
     {
        if (strlen(name) < 4)
          continue;

        if (strstr(name, ".go"))
          return EINA_TRUE;
     }

   return EINA_FALSE;
}

static Eina_Bool
_go_file_hidden_is(const char *file)
{
   if (!file || strlen(file) == 0)
     return EINA_FALSE;

   if (!strcmp(ecore_file_file_get(file), "_obj") || !strcmp(ecore_file_file_get(file), "target") ||
        eina_str_has_extension(file, ".so"))
     return EINA_TRUE;

   return EINA_FALSE;
}

static Eina_Bool
_go_project_runnable_is(const char *path EINA_UNUSED)
{
   if (!path || !path[0])
     return EINA_FALSE;

   return EINA_TRUE;
}

static void
_go_build(void)
{
   if (chdir(edi_project_get()) == 0)
     edi_exe_notify("edi_build", "go build");
}

static void
_go_test(void)
{
   if (chdir(edi_project_get()) == 0)
     edi_exe_notify("edi_test", "go test -v ./...");
}

static void
_go_run(const char *path EINA_UNUSED, const char *args EINA_UNUSED)
{
   const char *ext;
   char *full_cmd;
   int full_len, flags;

   if (!path) return;

   if (chdir(edi_project_get()) !=0)
     ERR("Could not chdir");

   full_len = strlen(path) + 8;
   if (args)
     full_len += strlen(args);

   full_cmd = malloc(sizeof(char) * (full_len + 1));

   flags = ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
           ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
           ECORE_EXE_PIPE_WRITE;

   ext = strrchr(path, '.');

   //  We may want to run a binary or via the go command.
   //  Simple and quicker to test for file extension.
   if (ext && !strcasecmp(ext, ".go"))
     {
        snprintf(full_cmd, full_len + 1, "go run %s %s", path, args?args:"");
        flags |= ECORE_EXE_USE_SH;
     }
   else
     {
        snprintf(full_cmd, full_len + 1, "%s", path);
     }

   edi_exe_project_run(full_cmd, flags, NULL);

   free(full_cmd);
}

static void
_go_clean(void)
{
   if (chdir(edi_project_get()) == 0)
     edi_exe_notify("edi_clean", "go clean");
}

Edi_Build_Provider _edi_build_provider_go =
   {
      "go",
      _go_project_supported,
      _go_file_hidden_is,
      _go_project_runnable_is,
      _go_build,
      _go_test,
      _go_run,
      _go_clean
   };
