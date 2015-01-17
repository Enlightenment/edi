#ifdef HAVE_CONFIG
# include "config.h"
#endif

#include "Edi.h"

#include "edi_private.h"

static Eina_Bool
_edi_create_project_done(void *data, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   Edi_Create *create;

   create = (Edi_Create *)data;

   ecore_event_handler_del(create->handler); 
   create->callback(create->path, EINA_TRUE);
   free(create->path);
   free(data);

   return ECORE_CALLBACK_DONE; // or ECORE_CALLBACK_PASS_ON
}

EAPI void
edi_create_project(const char *path, const char *name, const char *url,
                   const char *user, const char *email, Edi_Create_Cb func)
{
   char script[PATH_MAX], fullpath[PATH_MAX];
   char *cmd;
   int cmdlen;
   Edi_Create *data;
   Ecore_Event_Handler *handler;

   data = calloc(1, sizeof(Edi_Create));
   handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _edi_create_project_done, data); 
   snprintf(script, sizeof(script), "%s/skeleton/eflprj", elm_app_data_dir_get()
);
   snprintf(fullpath, sizeof(fullpath), "%s/%s", path, name);

   data->path = strdup(fullpath);
   data->callback = func;
   data->handler = handler;

   cmdlen = strlen(script) + 19 + strlen(path) + strlen(name) + strlen(url) + strlen(user) + strlen(email);
   cmd = malloc(sizeof(char) * cmdlen);
   snprintf(cmd, cmdlen, "%s \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"",
            script, fullpath, name, user, email, url);

   INF("Creating project \"%s\" at path %s for %s<%s>\n", name, fullpath, user, email);
   ecore_exe_run(cmd, data);
   free(cmd);
}


