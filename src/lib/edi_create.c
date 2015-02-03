#ifdef HAVE_CONFIG
# include "config.h"
#endif

#include "Edi.h"

#include "edi_private.h"

static Edi_Create *_edi_create_data;

static const char *
_edi_create_filter_variable(const char *text, const char *variable, const char *value)
{
   char *pos, *ret;
   int copylen;

   pos = strstr(text, variable);
   if (!pos)
     return strdup(text);

   copylen = pos - text;
   ret = malloc(sizeof(char) * (strlen(text) + strlen(value) - strlen(variable) + 1));

   snprintf(ret, copylen + 1, text);
   snprintf(ret + copylen, strlen(value) + 1, value);
   snprintf(ret + copylen + strlen(value), strlen(text) - copylen - strlen(variable) + 1, text + copylen + strlen(variable));

   return ret;
}

static const char *
_edi_create_filter_name(const char *text, const char *name)
{
   char *lowername;
   const char *filtered, *ret;

   filtered = _edi_create_filter_variable(text, "${Edi_Name}", name);

   lowername = strdup(name);
   eina_str_tolower(&lowername);

   ret = _edi_create_filter_variable(filtered, "${edi_name}", lowername);
   free(lowername);
   free((void *) filtered);

   return ret;
}

static int
_edi_create_year_get()
{
   time_t timeval;
   struct tm *tp;

   time (&timeval);
   tp = gmtime(&timeval);

   return tp->tm_year + 1900;
}

static void
_edi_create_filter_file(Edi_Create *create, const char *path)
{
   char *cmd, *lowername, *uppername;
   const char *template;
   int length;

   create->filters++;
// TODO speed this up - pre-cache this filter!
   template = "sed -i \"s|\\${edi_name}|%s|g;s|\\${Edi_Name}|%s|g;s|\\${EDI_NAME}|%s|g;s|\\${Edi_User}|%s|ig;s|\\${Edi_Email}|%s|g;s|\\${Edi_Url}|$%s|g;s|\\${Edi_Year}|%d|g\" %s";
   length = strlen(template) + (strlen(create->name) * 3)  + strlen(create->user) + strlen(create->email) + strlen(create->url) + strlen(path) + 4 - 16 + 1;

   lowername = strdup(create->name);
   eina_str_tolower(&lowername);
   uppername = strdup(create->name);
   eina_str_toupper(&uppername);

   cmd = malloc(sizeof(char) * length);
   snprintf(cmd, length, template, lowername, create->name, uppername , create->user, create->email, create->url, _edi_create_year_get(), path);

   ecore_exe_run(cmd, NULL);
   free(lowername);
   free(uppername);
   free(cmd);

   // This matches the filtered path copy created in the copy callback
   free((void *) path);
}

static void
_edi_create_free_data()
{
   Edi_Create *create;

   create = _edi_create_data;
   _edi_create_data = NULL;

   free(create->url);
   free(create->user);
   free(create->email);
   free(create->name);
   free(create->path);

   free(create);
}

static void
_edi_create_done_cb(void *data EINA_UNUSED, Eio_File *file EINA_UNUSED)
{
   // we're using the filter processes to determine when we're done
}

static Eina_Bool
_edi_create_project_done(void *data, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   Edi_Create *create;

   create = (Edi_Create *)data;

   ecore_event_handler_del(create->handler); 
   create->callback(create->path, EINA_TRUE);

   _edi_create_free_data();
   return ECORE_CALLBACK_DONE; // or ECORE_CALLBACK_PASS_ON
}

static Eina_Bool
_edi_create_filter_file_done(void *data, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   Edi_Create *create;
   Ecore_Event_Handler *handler;

   create = (Edi_Create *)data;

   if (--create->filters > 0)
     return ECORE_CALLBACK_PASS_ON;

   ecore_event_handler_del(create->handler);

   handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _edi_create_project_done, data);
   create->handler = handler;

   chdir(create->path);
   ecore_exe_run("git init && git add .", data);

   return ECORE_CALLBACK_PASS_ON;
}

static void
_edi_create_move_done_cb(void *data, Eio_File *file EINA_UNUSED)
{
   _edi_create_filter_file(_edi_create_data, (const char *) data);
}

static void
_edi_create_move_error_cb(void *data, Eio_File *handler EINA_UNUSED, int error)
{
   fprintf(stderr, "move error for %s: [%s]\n", (char *) data, strerror(error));

   // This matches the filtered path copy created in the copy callback
   free(data);
}

static void
_edi_create_notify_cb(void *d, Eio_File *handler EINA_UNUSED, const Eio_Progress *info)

{
   Edi_Create *data;
   const char *filtered;

   data = (Edi_Create *) d;

   switch (info->op)
     {
        case EIO_FILE_COPY:
          // this will get freed in the filter callback when it's done with
          filtered = _edi_create_filter_name(info->dest, data->name);

          if (strcmp(info->dest, filtered))
            {
               eio_file_move(info->dest, filtered, NULL, _edi_create_move_done_cb, _edi_create_move_error_cb, filtered);
            }
          else
            {
               _edi_create_filter_file(data, filtered);
            }

          break;
        default:
          break;
     }
}

static void
_edi_create_error_cb(void *data, Eio_File *handler EINA_UNUSED, int error)
{
   Edi_Create *create;

   create = (Edi_Create *) data;
   fprintf(stderr, "copy error: [%s]\n", strerror(error));
   create->callback(create->path, EINA_FALSE);

   _edi_create_free_data();
}

EAPI void
edi_create_efl_project(const char *parentdir, const char *name, const char *url,
                   const char *user, const char *email, Edi_Create_Cb func)
{
   char *source;
   char dest[PATH_MAX];
   Edi_Create *data;
   Ecore_Event_Handler *handler;

   source = PACKAGE_DATA_DIR "/skeleton/eflproject";
   snprintf(dest, sizeof(dest), "%s/%s", parentdir, name);

   INF("Creating project \"%s\" at path %s for %s<%s>\n", name, dest, user, email);
   DBG("Extracting project files from %s\n", source);

   data = calloc(1, sizeof(Edi_Create));
   data->path = strdup(dest);
   data->name = strdup(name);

   data->url = strdup(url);
   data->user = strdup(user);
   data->email = strdup(email);
   data->callback = func;
   _edi_create_data = data;

   data->filters = 0;
   handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _edi_create_filter_file_done, data);
   data->handler = handler;

   eio_dir_copy(source, dest, NULL, _edi_create_notify_cb, _edi_create_done_cb,
                _edi_create_error_cb, data);
}


