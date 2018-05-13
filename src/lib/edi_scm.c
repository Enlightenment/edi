#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Ecore.h>
#include <Ecore_File.h>
#include <Ethumb.h>

#include "Edi.h"
#include "edi_private.h"
#include "edi_exe.h"
#include "edi_path.h"
#include "edi_scm.h"
#include "md5.h"

Edi_Scm_Engine *_edi_scm_global_object = NULL;

static int
_edi_scm_exec(const char *command)
{
   int code;
   char *oldpwd;
   Edi_Scm_Engine *self = _edi_scm_global_object;

   if (!self) return -1;

   oldpwd = getcwd(NULL, PATH_MAX);

   chdir(self->root_directory);
   code = edi_exe_wait(command);
   chdir(oldpwd);

   free(oldpwd);

   return code;
}

static char *
_edi_scm_exec_response(const char *command)
{
   char *oldpwd, *response;
   Edi_Scm_Engine *self = _edi_scm_global_object;

   if (!self) return NULL;

   oldpwd = getcwd(NULL, PATH_MAX);

   chdir(self->root_directory);
   response = edi_exe_response(command);
   chdir(oldpwd);

   free(oldpwd);

   return response;
}

EAPI int
edi_scm_git_new(void)
{
   int code;
   char *oldpwd;

   oldpwd = getcwd(NULL, PATH_MAX);

   chdir(edi_project_get());
   code = edi_exe_wait("git init .");
   chdir(oldpwd);

   free(oldpwd);

   return code;
}

EAPI int
edi_scm_git_clone(const char *url, const char *dir)
{
   int code;
   Eina_Strbuf *command = eina_strbuf_new();

   eina_strbuf_append_printf(command, "git clone '%s' '%s'", url, dir);
   code = edi_exe_wait(eina_strbuf_string_get(command));

   eina_strbuf_free(command);
   return code;
}

static int
_edi_scm_git_file_stage(const char *path)
{
   int code;
   Eina_Strbuf *command = eina_strbuf_new();

   eina_strbuf_append_printf(command, "git add %s", path);

   code = _edi_scm_exec(eina_strbuf_string_get(command));

   eina_strbuf_free(command);

   return code;
}

static int
_edi_scm_git_file_unstage(const char *path)
{
   int code;
   Eina_Strbuf *command = eina_strbuf_new();

   eina_strbuf_append_printf(command, "git remote get-url origin");

   code = _edi_scm_exec(eina_strbuf_string_get(command));

   eina_strbuf_reset(command);

   if (code == 0)
     eina_strbuf_append_printf(command, "git reset HEAD %s", path);
   else
     eina_strbuf_append_printf(command, "git rm --cached %s", path);

   code = _edi_scm_exec(eina_strbuf_string_get(command));

   eina_strbuf_free(command);

  return code;
}

static int
_edi_scm_git_file_mod(const char *path)
{
   int code;
   Eina_Strbuf *command = eina_strbuf_new();

   eina_strbuf_append_printf(command, "git mod %s", path);

   code = _edi_scm_exec(eina_strbuf_string_get(command));

   eina_strbuf_free(command);

   return code;
}

static int
_edi_scm_git_file_move(const char *source, const char *dest)
{
   int code;
   Eina_Strbuf *command = eina_strbuf_new();

   eina_strbuf_append_printf(command, "git mv %s %s", source, dest);

   code = _edi_scm_exec(eina_strbuf_string_get(command));

   eina_strbuf_free(command);

   return code;
}

static int
_edi_scm_git_file_del(const char *path)
{
   int code;
   Eina_Strbuf *command = eina_strbuf_new();

   eina_strbuf_append_printf(command, "git rm %s", path);

   code = _edi_scm_exec(eina_strbuf_string_get(command));

   eina_strbuf_free(command);

   return code;
}

static int
_edi_scm_git_status(void)
{
   int code;
   Eina_Strbuf *command = eina_strbuf_new();

   eina_strbuf_append(command, "git status");

   code = _edi_scm_exec(eina_strbuf_string_get(command));

   eina_strbuf_free(command);

   return code;
}

static Edi_Scm_Status *
_parse_line(char *line)
{
   char *esc_path, *path, *fullpath, *change;
   Edi_Scm_Status *status;

   change = line;
   line[2] = '\0';
   path = line + 3;

   status = malloc(sizeof(Edi_Scm_Status));
   if (!status)
     return NULL;

   status->staged = EINA_FALSE;

   if (change[0] == 'A' || change[1] == 'A')
     {
        status->change = EDI_SCM_STATUS_ADDED;
        if (change[0] == 'A')
          {
            status->staged = status->change = EDI_SCM_STATUS_ADDED_STAGED;
          }
     }
   else if (change[0] == 'R' || change[1] == 'R')
     {
        status->change = EDI_SCM_STATUS_RENAMED;
        if (change[0] == 'R')
          {
             status->staged = status->change = EDI_SCM_STATUS_RENAMED_STAGED;
          }
     }
   else if (change[0] == 'M' || change[1] == 'M')
     {
        status->change = EDI_SCM_STATUS_MODIFIED;
        if (change[0] == 'M')
          {
             status->staged = status->change = EDI_SCM_STATUS_MODIFIED_STAGED;
          }
     }
   else if (change[0] == 'D' || change[1] == 'D')
     {
        status->change = EDI_SCM_STATUS_DELETED;
        if (change[0] == 'D')
          {
             status->staged = status->change = EDI_SCM_STATUS_DELETED_STAGED;
          }
     }
   else if (change[0] == '?' && change[1] == '?')
     {
        status->change = EDI_SCM_STATUS_UNTRACKED;
     }
   else
        status->change = EDI_SCM_STATUS_UNKNOWN;

   esc_path = ecore_file_escape_name(path);
   status->path = eina_stringshare_add(esc_path);
   fullpath = edi_path_append(edi_scm_engine_get()->root_directory, esc_path);
   status->fullpath = eina_stringshare_add(fullpath);
   status->unescaped = eina_stringshare_add(path);

   free(fullpath);
   free(esc_path);

   return status;
}

static Edi_Scm_Status_Code
_edi_scm_git_file_status(const char *path)
{
   Edi_Scm_Status *status;
   char command[4096];
   char *line, *escaped;
   Edi_Scm_Status_Code result;

   escaped = ecore_file_escape_name(path);
   snprintf(command, sizeof(command), "git status --porcelain %s", escaped);
   free(escaped);
   line = _edi_scm_exec_response(command);
   if (!line[0] || !line[1])
     {
        result = EDI_SCM_STATUS_NONE;
     }
   else
     {
        status = _parse_line(line);
        result = status->change;
        eina_stringshare_del(status->path);
        eina_stringshare_del(status->fullpath);
        eina_stringshare_del(status->unescaped);
        free(status);
     }

   free(line);

   return result;
}

static Eina_List *
_edi_scm_git_status_get(void)
{
   char *output, *pos, *start, *end;
   char *line;
   size_t size;
   Eina_Strbuf *command;
   Edi_Scm_Status *status;
   Eina_List *list = NULL;

   command = eina_strbuf_new();

   eina_strbuf_append(command, "git status --porcelain");

   output = _edi_scm_exec_response(eina_strbuf_string_get(command));

   eina_strbuf_free(command);

   end = NULL;

   pos = output;
   start = pos;

   while (*pos++)
     {
        if (*pos == '\n')
          end = pos;
        if (start && end)
          {
             size = end - start;
             line = malloc(size + 1);
             memcpy(line, start, size);
             line[size] = '\0';

             status = _parse_line(line);
             if (status)
               list = eina_list_append(list, status);

             free(line);
             start = end + 1;
             end = NULL;
          }
     }

   end = pos;
   size = end - start;
   if (size > 1)
     {
        line = malloc(size + 1);
        memcpy(line, start, size);
        line[size] = '\0';

        status = _parse_line(line);
        if (status)
          list = eina_list_append(list, status);

        free(line);
    }

   free(output);

   return list;
}

static char *
_edi_scm_git_diff(Eina_Bool cached)
{
   char *output;
   Eina_Strbuf *command;

   command = eina_strbuf_new();

   if (cached)
     eina_strbuf_append(command, "git diff --cached");
   else
     eina_strbuf_append(command, "git diff");

   output = _edi_scm_exec_response(eina_strbuf_string_get(command));

   eina_strbuf_free(command);

   return output;
}

static int
_edi_scm_git_commit(const char *message)
{
   int code;
   Eina_Strbuf *command = eina_strbuf_new();

   eina_strbuf_append_printf(command, "git commit -m \"%s\"", message);

   code = _edi_scm_exec(eina_strbuf_string_get(command));

   eina_strbuf_free(command);

   return code;
}

static int
_edi_scm_git_push(void)
{
   return _edi_scm_exec("git push");
}

static int
_edi_scm_git_pull(void)
{
   int code;
   Eina_Strbuf *command = eina_strbuf_new();

   eina_strbuf_append(command, "git pull");

   code = _edi_scm_exec(eina_strbuf_string_get(command));

   eina_strbuf_free(command);

   return code;
}

static int
_edi_scm_git_stash(void)
{
   int code;
   Eina_Strbuf *command = eina_strbuf_new();

   eina_strbuf_append(command, "git stash");

   code = _edi_scm_exec(eina_strbuf_string_get(command));

   eina_strbuf_free(command);

   return code;
}

static int
_edi_scm_git_remote_add(const char *remote_url)
{
   int code;
   Eina_Strbuf *command = eina_strbuf_new();

   eina_strbuf_append_printf(command, "git remote add origin %s", remote_url);

   code = _edi_scm_exec(eina_strbuf_string_get(command));
   eina_strbuf_free(command);

   if (code == 0)
     code = _edi_scm_exec("git push --set-upstream origin master");

   return code;
}

static const char *
_edi_scm_git_remote_name_get(void)
{
   static char *_remote_name;
   Edi_Scm_Engine *engine = _edi_scm_global_object;

   if (!engine)
     return NULL;

   if (_remote_name)
     return _remote_name;

   _remote_name = _edi_scm_exec_response("git config --get user.name");

   if (_remote_name && !_remote_name[0])
     {
        free(_remote_name);
        _remote_name = NULL;
     }

   return _remote_name;
}

static const char *
_edi_scm_git_remote_email_get(void)
{
   static char *_remote_email;
   Edi_Scm_Engine *engine = _edi_scm_global_object;

   if (!engine)
     return NULL;

   if (_remote_email)
     return _remote_email;

   _remote_email = _edi_scm_exec_response("git config --get user.email");

   if (_remote_email && !_remote_email[0])
     {
        free(_remote_email);
        _remote_email = NULL;
     }

   return _remote_email;
}

static const char *
_edi_scm_git_remote_url_get(void)
{
   static char *_remote_url;
   Edi_Scm_Engine *engine = _edi_scm_global_object;

   if (!engine)
     return NULL;

   if (_remote_url)
     return _remote_url;

   _remote_url = _edi_scm_exec_response("git remote get-url origin");

   if (_remote_url && !_remote_url[0])
     {
        free(_remote_url);
        _remote_url = NULL;
     }

   return _remote_url;
}

static int
_edi_scm_git_credentials_set(const char *name, const char *email)
{
   int code;
   Eina_Strbuf *command = eina_strbuf_new();

   eina_strbuf_append_printf(command, "git config user.name '%s'", name);
   code = _edi_scm_exec(eina_strbuf_string_get(command));
   eina_strbuf_reset(command);
   eina_strbuf_append_printf(command, "git config user.email '%s'", email);
   code = _edi_scm_exec(eina_strbuf_string_get(command));

   eina_strbuf_free(command);

   return code;
}

static Eina_Bool
_edi_scm_enabled(Edi_Scm_Engine *engine)
{
   char *path;
   if (!engine) return EINA_FALSE;

   if (!engine->path)
     {
        path = edi_path_append(edi_project_get(), engine->directory);
        engine->path = eina_stringshare_add(path);
        free(path);
     }

   return ecore_file_exists(engine->path);
}

EAPI Eina_Bool
edi_scm_remote_enabled(void)
{
   Edi_Scm_Engine *e = _edi_scm_global_object;
   if (!e)
     return EINA_FALSE;

   return !!e->remote_url_get();
}

EAPI Eina_Bool
edi_scm_enabled(void)
{
   Edi_Scm_Engine *engine = _edi_scm_global_object;
   if (!engine)
     return EINA_FALSE;

   if (!engine->initialized)
     return EINA_FALSE;

   return _edi_scm_enabled(engine);
}

EAPI Edi_Scm_Engine *
edi_scm_engine_get(void)
{
   return _edi_scm_global_object;
}

EAPI void
edi_scm_shutdown()
{
   Edi_Scm_Engine *engine = _edi_scm_global_object;

   if (!engine)
     return;

   eina_stringshare_del(engine->path);
   free(engine->root_directory);
   free(engine);

   _edi_scm_global_object = NULL;
}

EAPI int
edi_scm_stage(const char *path)
{
   char *escaped;
   int result;
   Edi_Scm_Engine *e = edi_scm_engine_get();

   escaped = ecore_file_escape_name(path);

   result = e->file_stage(escaped);

   free(escaped);

   return result;
}

EAPI int
edi_scm_del(const char *path)
{
   char *escaped;
   int result;
   Edi_Scm_Engine *e = edi_scm_engine_get();

   escaped = ecore_file_escape_name(path);

   result = e->file_del(escaped);

   free(escaped);

   return result;
}

EAPI int
edi_scm_unstage(const char *path)
{
   char *escaped;
   int result;
   Edi_Scm_Engine *e = edi_scm_engine_get();

   escaped = ecore_file_escape_name(path);

   result = e->file_unstage(escaped);

   free(escaped);

   return result;
}

EAPI int
edi_scm_move(const char *src, const char *dest)
{
   char *esc_src, *esc_dst;
   int result;
   Edi_Scm_Engine *e = edi_scm_engine_get();

   esc_src = ecore_file_escape_name(src);
   esc_dst = ecore_file_escape_name(dest);

   result = e->move(esc_src, esc_dst);

   free(esc_src);
   free(esc_dst);

   return result;
}

EAPI Eina_Bool
edi_scm_status_get(void)
{
   Edi_Scm_Engine *e = edi_scm_engine_get();

   e->statuses = e->status_get();

   if (!e->statuses)
     return EINA_FALSE;

   return EINA_TRUE;
}

EAPI Edi_Scm_Status_Code
edi_scm_file_status(const char *path)
{
   char *escaped;
   int result;
   Edi_Scm_Engine *e = edi_scm_engine_get();

   escaped = ecore_file_escape_name(path);

   result = e->file_status(escaped);

   free(escaped);

   return result;
}

EAPI void
edi_scm_commit(const char *message)
{
   Edi_Scm_Engine *e = edi_scm_engine_get();

   e->commit(message);
}

static void
_edi_scm_status_thread_cb(void *data, Ecore_Thread *thread)
{
   Edi_Scm_Engine *e = data;

   e->status();

   ecore_thread_cancel(thread);
}

EAPI void
edi_scm_status(void)
{
   Edi_Scm_Engine *e = edi_scm_engine_get();

   ecore_thread_run(_edi_scm_status_thread_cb, NULL, NULL, e);
}

EAPI int
edi_scm_remote_add(const char *remote_url)
{
   Edi_Scm_Engine *e = edi_scm_engine_get();

   return e->remote_add(remote_url);
}

EAPI char *
edi_scm_diff(Eina_Bool cached)
{
   Edi_Scm_Engine *e = edi_scm_engine_get();

   return e->diff(cached);
}

EAPI void
edi_scm_stash(void)
{
   Edi_Scm_Engine *e = edi_scm_engine_get();

   e->stash();
}

EAPI int
edi_scm_credentials_set(const char *user, const char *email)
{
   Edi_Scm_Engine *e = edi_scm_engine_get();

   return e->credentials_set(user, email);
}

static void
_edi_scm_pull_thread_cb(void *data, Ecore_Thread *thread)
{
   Edi_Scm_Engine *e = data;

   e->pull();

   ecore_thread_cancel(thread);
}

EAPI void
edi_scm_pull(void)
{
   Edi_Scm_Engine *e = edi_scm_engine_get();

   ecore_thread_run(_edi_scm_pull_thread_cb, NULL, NULL, e);
}

static void
_edi_scm_push_thread_cb(void *data, Ecore_Thread *thread)
{
   Edi_Scm_Engine *e = data;

   e->push();

   ecore_thread_cancel(thread);
}

EAPI void
edi_scm_push(void)
{
   Edi_Scm_Engine *e = edi_scm_engine_get();

   ecore_thread_run(_edi_scm_push_thread_cb, NULL, NULL, e);
}

EAPI const char *
edi_scm_root_directory_get(void)
{
   Edi_Scm_Engine *e = edi_scm_engine_get();

   return e->root_directory;
}

static Edi_Scm_Engine *
_edi_scm_git_init(const char *rootdir)
{
   Edi_Scm_Engine *engine;

   if (!ecore_file_app_installed("git"))
     return NULL;

   _edi_scm_global_object = engine = calloc(1, sizeof(Edi_Scm_Engine));
   engine->name = "git";
   engine->directory = ".git";
   engine->file_stage = _edi_scm_git_file_stage;
   engine->file_mod = _edi_scm_git_file_mod;
   engine->file_del = _edi_scm_git_file_del;
   engine->file_unstage = _edi_scm_git_file_unstage;
   engine->move = _edi_scm_git_file_move;
   engine->status = _edi_scm_git_status;
   engine->diff = _edi_scm_git_diff;
   engine->commit = _edi_scm_git_commit;
   engine->pull = _edi_scm_git_pull;
   engine->push = _edi_scm_git_push;
   engine->stash = _edi_scm_git_stash;
   engine->file_status = _edi_scm_git_file_status;

   engine->remote_add = _edi_scm_git_remote_add;
   engine->remote_name_get = _edi_scm_git_remote_name_get;
   engine->remote_email_get = _edi_scm_git_remote_email_get;
   engine->remote_url_get = _edi_scm_git_remote_url_get;
   engine->credentials_set = _edi_scm_git_credentials_set;
   engine->status_get = _edi_scm_git_status_get;

   engine->root_directory = strdup(rootdir);
   engine->initialized = EINA_TRUE;

   return engine;
}

static char *
_edi_scm_root_find(const char *dir, const char *scmdir)
{
   char *directory, *engine_root, *path, *tmp;

   engine_root = NULL;
   directory = strdup(dir);
   while (directory && strlen(directory) > 1)
     {
        path = edi_path_append(directory, scmdir);
        if (ecore_file_exists(path) && ecore_file_is_dir(path))
          {
             engine_root = strdup(directory);

             free(directory);
             free(path);
             break;
          }

        tmp = ecore_file_dir_get(directory);
        free(directory);
        directory = tmp;
        free(path);
     }

   return engine_root;
}

EAPI Edi_Scm_Engine *
edi_scm_init_path(const char *path)
{
   char *location;
   Edi_Scm_Engine *engine;

   engine = NULL;
   location = _edi_scm_root_find(path, ".git");
   if (location)
     {
        engine = _edi_scm_git_init(location);
        free(location);
     }

   return engine;
}

EAPI Edi_Scm_Engine *
edi_scm_init(void)
{
   char *cwd;
   Edi_Scm_Engine *engine;

   if (edi_project_get())
     return edi_scm_init_path(edi_project_get());

   cwd = getcwd(NULL, PATH_MAX);
   engine = edi_scm_init_path(cwd);
   free(cwd);
   return engine;
}

EAPI const char *
edi_scm_avatar_url_get(const char *email)
{
   int n;
   char *id;
   const char *url;
   MD5_CTX ctx;
   char md5out[(2 * MD5_HASHBYTES) + 1];
   unsigned char hash[MD5_HASHBYTES];
   static const char hex[] = "0123456789abcdef";

   if (!email || strlen(email) == 0)
     return NULL;

   id = strdup(email);
   eina_str_tolower(&id);

   MD5Init(&ctx);
   MD5Update(&ctx, (unsigned char const*)id, (unsigned)strlen(id));
   MD5Final(hash, &ctx);

   for (n = 0; n < MD5_HASHBYTES; n++)
     {
        md5out[2 * n] = hex[hash[n] >> 4];
        md5out[2 * n + 1] = hex[hash[n] & 0x0f];
     }
   md5out[2 * MD5_HASHBYTES] = '\0';

   url = eina_stringshare_printf("http://www.gravatar.com/avatar/%s", md5out);

   free(id);
   return url;
}

