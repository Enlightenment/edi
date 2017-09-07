#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/wait.h>

#include <Ecore.h>
#include <Ecore_Con.h>

#include "Edi.h"
#include "edi_private.h"

static Ecore_Event_Handler *_edi_exe_handler = NULL;
static Ecore_Event_Handler *_edi_exe_notify_handler = NULL;

typedef struct _Edi_Exe_Args {
   void ((*func)(int, void *));
   void *data;
} Edi_Exe_Args;

static Eina_Bool
_edi_exe_notify_data_cb(void *data, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   int *status;
   Edi_Exe_Args *args;
   Ecore_Con_Event_Client_Data *ev = event;

   status = ev->data;

   args = data;

   args->func(*status, args->data);

   ecore_event_handler_del(_edi_exe_notify_handler);
   _edi_exe_notify_handler = NULL;

   free(args);

   return EINA_FALSE;
}

EAPI Eina_Bool
edi_exe_notify_handle(const char *name, void ((*func)(int, void *)), void *data)
{
   Edi_Exe_Args *args;

   if (_edi_exe_notify_handler) return EINA_FALSE;

  /* These are UNIX domain sockets, no need to clean up */
   ecore_con_server_add(ECORE_CON_LOCAL_USER, name, 0, NULL);

   args = malloc(sizeof(Edi_Exe_Args));
   args->func = func;
   args->data = data;

   _edi_exe_notify_handler = ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_DATA, (Ecore_Event_Handler_Cb) _edi_exe_notify_data_cb, args);

   return EINA_TRUE;
}

static Eina_Bool
_edi_exe_event_done_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
  Ecore_Exe_Event_Del *ev;
  Ecore_Con_Server *srv;
  const char *name = data;

  ev = event;

  if (!ev->exe) return ECORE_CALLBACK_RENEW;

  /* These are UNIX domain sockets, no need to clean up */
  srv = ecore_con_server_connect(ECORE_CON_LOCAL_USER, name, 0, NULL);

  ecore_con_server_send(srv, &ev->exit_code, sizeof(int *));
  ecore_con_server_flush(srv);

  ecore_event_handler_del(_edi_exe_handler);
  _edi_exe_handler = NULL;

  return ECORE_CALLBACK_DONE;
}

EAPI void
edi_exe_notify(const char *name, const char *command)
{
   if (_edi_exe_handler) return;

   ecore_exe_pipe_run(command,
                      ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                      ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                      ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);

   _edi_exe_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _edi_exe_event_done_cb, name);
}

EAPI int
edi_exe_wait(const char *command)
{
   pid_t pid;
   Ecore_Exe *exe;
   int exit;

   ecore_thread_main_loop_begin();
   exe = ecore_exe_pipe_run(command,
                            ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                            ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR |
                            ECORE_EXE_PIPE_WRITE | ECORE_EXE_USE_SH, NULL);
   pid = ecore_exe_pid_get(exe);
   ecore_thread_main_loop_end();

   waitpid(pid, &exit, 0);
   return exit;
}

EAPI char *
edi_exe_response(const char *command)
{
   FILE *p;
   char buf[8192];
   Eina_Strbuf *lines;
   char *out;
   ssize_t len;

   p = popen(command, "r");
   if (!p)
     return NULL;

   lines = eina_strbuf_new();

   while ((fgets(buf, sizeof(buf), p)) != NULL)
     {
        eina_strbuf_append_printf(lines, "%s", buf);
     }

   pclose(p);

   len = eina_strbuf_length_get(lines);
   eina_strbuf_remove(lines, len - 1, len);

   out = strdup(eina_strbuf_string_get(lines));

   eina_strbuf_free(lines);

   return out;
}
