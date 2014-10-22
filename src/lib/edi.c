#ifdef HAVE_CONFIG
# include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Edi.h"

#include "edi_private.h"

static int _edi_init = 0;
int _edi_lib_log_dom = -1;
static const char *_edi_project_path;

EAPI int
edi_init(void)
{
   _edi_init++;
   if (_edi_init > 1) return _edi_init;

   eina_init();

   _edi_lib_log_dom = eina_log_domain_register("edi", EINA_COLOR_CYAN);
   if (_edi_lib_log_dom < 0)
     {
        EINA_LOG_ERR("Edi can not create its log domain.");
        goto shutdown_eina;
     }

   // Put here your initialization logic of your library

   eina_log_timing(_edi_lib_log_dom, EINA_LOG_STATE_STOP, EINA_LOG_STATE_INIT);

   return _edi_init;

   shutdown_eina:
   eina_shutdown();
   _edi_init--;

   return _edi_init;
}

EAPI int
edi_shutdown(void)
{
   _edi_init--;
   if (_edi_init != 0) return _edi_init;

   eina_log_timing(_edi_lib_log_dom,
                   EINA_LOG_STATE_START,
                   EINA_LOG_STATE_SHUTDOWN);

   // Put here your shutdown logic

   eina_log_domain_unregister(_edi_lib_log_dom);
   _edi_lib_log_dom = -1;

   eina_shutdown();

   return _edi_init;
}

static Eina_Bool
_edi_path_isdir(const char *path)
{
    struct stat buf;

    if (!path)
      return EINA_FALSE;

    stat(path, &buf);
    return S_ISDIR(buf.st_mode);
}

EAPI Eina_Bool
edi_project_set(const char *path)
{
   char *real = NULL;

   real = realpath(real, NULL);
   if (!_edi_path_isdir(path))
     {
        free(real);
        return EINA_FALSE;
     }

   if (_edi_project_path)
     eina_stringshare_del(_edi_project_path);

   _edi_project_path = eina_stringshare_add(path);
   free(real);
   return EINA_TRUE;
}

EAPI const char *
edi_project_get()
{
   return _edi_project_path;
}

EAPI const char *
edi_project_file_path_get(const char *file)
{
   char *path;
   int len;

   len = strlen(file) + strlen(edi_project_get()) + 2;
   path = malloc(sizeof(char) * len);
   snprintf(path, len, "%s/%s", edi_project_get(), file);

   return path;
}

EAPI Eina_Bool
edi_project_file_exists(const char *file)
{
   const char *path;
   Eina_Bool exists;

   path = edi_project_file_path_get(file);
   exists = ecore_file_exists(path);

   free((void *)path);
   return exists;
}
