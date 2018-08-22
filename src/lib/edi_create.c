#ifdef HAVE_CONFIG
# include "config.h"
#endif

#include <Eio.h>
#include <Ecore.h>
#include <Ecore_File.h>

#include "Edi.h"

#include "edi_private.h"

#define EXAMPLES_GIT_URL "https://git.enlightenment.org/tools/examples.git"

typedef struct _Edi_Create
{
   char *path, *temp, *name, *skelfile;
   char *url, *user, *email;

   Edi_Create_Cb callback;
   Ecore_Event_Handler *handler;

   int filters;
} Edi_Create;

typedef struct _Edi_Create_Example
{
   char *path, *name;

   Edi_Create_Cb callback;

} Edi_Create_Example;


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

   snprintf(ret, copylen + 1, "%s", text);
   snprintf(ret + copylen, strlen(value) + 1, "%s", value);
   snprintf(ret + copylen + strlen(value), strlen(text) - copylen - strlen(variable) + 1, "%s", text + copylen + strlen(variable));

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

char *
edi_create_escape_quotes(const char *in)
{
   char buf[1024], *out_ptr;
   const char *pos, *in_ptr;
   int replace_len;

   pos = strstr(in, "'");
   if (!pos)
     return strdup(in);

   in_ptr = in;
   out_ptr = buf;
   while (pos)
     {
        replace_len = pos - in_ptr;
        snprintf(out_ptr, replace_len + 1, "%s", in_ptr);
        snprintf(out_ptr + replace_len, 8, "'\\\"'\\\"'");

	in_ptr += replace_len + 1;
	out_ptr += replace_len + 7;
        pos = strstr(in_ptr, "'");
     }
   snprintf(out_ptr, strlen(in) - (in_ptr - in) + 1, "%s", in_ptr);

   return strdup(buf);
}

static void
_edi_create_filter_file(Edi_Create *create, const char *path)
{
   char *cmd, *name, *lowername, *uppername, *user;
   const char *template;
   int length;

   name = edi_create_escape_quotes(create->name);
   user = edi_create_escape_quotes(create->user);
   create->filters++;
// TODO speed this up - pre-cache this filter!
   template = "sh -c \"sed -i.bak 's|\\${edi_name}|%s|g;s|\\${Edi_Name}|%s|g;s|\\${EDI_NAME}|%s|g;s|\\${Edi_User}|%s|g;s|\\${Edi_Email}|%s|g;s|\\${Edi_Url}|%s|g;s|\\${Edi_Year}|%d|g' %s\"; rm %s.bak";
   length = strlen(template) + (strlen(name) * 3)  + strlen(user) + strlen(create->email) + strlen(create->url) + (strlen(path) * 2) + 4 - 16 + 1;

   lowername = strdup(name);
   eina_str_tolower(&lowername);
   uppername = strdup(name);
   eina_str_toupper(&uppername);

   cmd = malloc(sizeof(char) * length);
   snprintf(cmd, length, template, lowername, name, uppername , user, create->email, create->url, _edi_create_year_get(), path, path);

   ecore_exe_run(cmd, NULL);
   free(lowername);
   free(uppername);
   free(name);
   free(user);
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

   if (!create)
     return;

   if (create->temp && ecore_file_exists(create->temp))
     ecore_file_recursive_rm(create->temp);

   free(create->url);
   free(create->user);
   free(create->email);
   free(create->name);
   free(create->path);
   free(create->temp);
   free(create->skelfile);

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
   if (create->callback)
     create->callback(create->path, EINA_TRUE);

   _edi_create_free_data();
   return ECORE_CALLBACK_DONE; // or ECORE_CALLBACK_PASS_ON
}

static Eina_Bool
_edi_create_filter_file_done(void *data, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   Edi_Create *create;
   Ecore_Event_Handler *handler;
   Eina_Strbuf *command;
   char *escaped;

   create = (Edi_Create *)data;

   if (--create->filters > 0)
     return ECORE_CALLBACK_PASS_ON;

   ecore_event_handler_del(create->handler);

   handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _edi_create_project_done, data);
   create->handler = handler;

   if (chdir(create->path) != 0)
     ERR("Could not chdir");

   command = eina_strbuf_new();

   eina_strbuf_append(command, "sh -c \"git init && git add .");

   if (create->user && strlen(create->user))
     {
        escaped = ecore_file_escape_name(create->user);
        eina_strbuf_append_printf(command, " && git config user.name %s", escaped);
        free(escaped);
     }

   if (create->email && strlen(create->email))
     {
        escaped = ecore_file_escape_name(create->email);
        eina_strbuf_append_printf(command, " && git config user.email %s", escaped);
        free(escaped);
     }

   eina_strbuf_append(command, " \" ");

   ecore_exe_run(eina_strbuf_string_get(command), data);

   eina_strbuf_free(command);

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
   WRN("move error for %s: [%s]\n", (char *) data, strerror(error));

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
   ERR("copy error: [%s]\n", strerror(error));

   _edi_create_free_data();
}

static Eina_Bool
_edi_create_extract_done(void *data, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   Edi_Create *create;
   Ecore_Event_Handler *handler;
   char tmpinner[PATH_MAX];

   create = (Edi_Create *)data;
   snprintf(tmpinner, sizeof(tmpinner), "%s/%s", create->temp, create->skelfile);
   tmpinner[strlen(tmpinner) - 7] = '\0'; // strip extension

   ecore_event_handler_del(create->handler);

   create->filters = 0;
   handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _edi_create_filter_file_done, data);
   create->handler = handler;

   eio_dir_copy(tmpinner, create->path, NULL, _edi_create_notify_cb, _edi_create_done_cb,
                _edi_create_error_cb, data);

   return ECORE_CALLBACK_DONE;
}

EAPI void
edi_create_project(const char *template_name, const char *parentdir,
                   const char *name, const char *url, const char *user,
                   const char *email, Edi_Create_Cb func)
{
   char *cmd, *extract;
   char tmp[PATH_MAX], dest[PATH_MAX], skelpath[PATH_MAX];
   Edi_Create *data;
   Ecore_Event_Handler *handler;

   extract = "tar zxf %s -C %s";
   snprintf(tmp, sizeof(tmp), "%s/edi_%s", eina_environment_tmp_get(), name);
   snprintf(dest, sizeof(dest), "%s/%s", parentdir, name);

   snprintf(skelpath, sizeof(skelpath), PACKAGE_DATA_DIR "/templates/%s.tar.gz", template_name);

   INF("Creating project \"%s\" at path %s for %s<%s>\n", name, dest, user, email);
   DBG("Extracting project files from %s\n", skelpath);

   data = calloc(1, sizeof(Edi_Create));
   data->path = strdup(dest);
   data->name = strdup(name);
   data->skelfile = strdup(ecore_file_file_get(skelpath));

   data->url = strdup(url);
   data->user = strdup(user);
   data->email = strdup(email);
   data->callback = func;
   _edi_create_data = data;

   if (!ecore_file_mkpath(tmp) || !ecore_file_mkpath(dest))
     {
        ERR("Failed to create path %s\n", dest);
        _edi_create_free_data();
        return;
     }

   cmd = malloc(sizeof(char) * (strlen(extract) + strlen(skelpath) + strlen(tmp) + 1));
   sprintf(cmd, extract, skelpath, tmp);

   data->temp = strdup(tmp);
   handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _edi_create_extract_done, data);
   data->handler = handler;

   ecore_exe_run(cmd, data);
   free(cmd);
}

static void
_edi_create_example_done_cb(void *data, Eio_File *file EINA_UNUSED)
{
   Edi_Create_Example *create = data;

   if (create->callback)
     create->callback(create->path, EINA_TRUE);
}

static void
_edi_create_example_extract_dir(char *examples_path, Edi_Create_Example *create)
{
   char path[PATH_MAX];

   eina_file_path_join(path, sizeof(path), examples_path, create->name);

   eio_dir_copy(path, create->path, NULL, NULL, _edi_create_example_done_cb,
                _edi_create_error_cb, create);

   free(examples_path);
}

EAPI void
edi_create_example(const char *example_name, const char *parentdir,
                   const char *name, Edi_Create_Cb func)
{
   char dest[PATH_MAX], examplepath[PATH_MAX];
   int status = 0;
   Edi_Create_Example *data;

   snprintf(dest, sizeof(dest), "%s/%s", parentdir, name);
   snprintf(examplepath, sizeof(examplepath), "%s/%s/examples.git",
            efreet_cache_home_get(), PACKAGE_NAME);

   data = calloc(1, sizeof(Edi_Create_Example));
   data->path = strdup(dest);
   data->name = strdup(example_name);
   data->callback = func;

   INF("Extracting example project \"%s\" at path %s\n", example_name, dest);

   if (ecore_file_exists(examplepath))
     ERR("TODO: UPDATE NOT IMPLEMENTED");
//     status = edi_scm_git_update(examplepath);
   else
     status = edi_scm_git_clone(EXAMPLES_GIT_URL, examplepath);

   if (status)
     {
        ERR("git error: [%d]\n", status);

        if (func)
          func(dest, EINA_FALSE);
     }
   else
     _edi_create_example_extract_dir(strdup(examplepath), data);
}

