#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eo.h>
#include <Eina.h>
#include <Elementary.h>

#include "edi_debug.h"
#include "edi_theme.h"
#include "edi_debugpanel.h"
#include "edi_config.h"

#include "edi_private.h"

#if defined (__APPLE__)
 #define LIBTOOL_COMMAND "glibtool"
#else
 #define LIBTOOL_COMMAND "libtool"
#endif

static Evas_Object *_info_widget, *_entry_widget, *_button_start, *_button_quit;
static Evas_Object *_button_int, *_button_term;

static Eina_Bool
_edi_debugpanel_config_changed(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   elm_entry_text_style_user_pop(_info_widget);
   elm_entry_text_style_user_push(_info_widget, eina_slstr_printf("DEFAULT='font=\\'%s\\' font_size=\\'%d\\''", _edi_project_config->font.name, _edi_project_config->font.size));
   elm_entry_calc_force(_info_widget);

   return ECORE_CALLBACK_RENEW;
}

static void
chomp(char *line)
{
   char *s = line;

   while (*s)
     {
        if (*s == '\r' || *s == '\n')
          {
             *s = '\0';
             break;
          }
        s++;
     }
}

static Eina_Bool
_debugpanel_stdout_handler(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Exe_Event_Data *ev;
   Edi_Debug *debug;
   char *buf, *start, *end;

   ev = event;

   debug = edi_debug_get();

   if (ev->exe != debug->exe)
     return ECORE_CALLBACK_RENEW;

   if (ev && ev->size)
      {
         if (!ev->data) return ECORE_CALLBACK_DONE;

         buf = start = ev->data;

         ecore_thread_main_loop_begin();
         while (start < (buf + ev->size))
           {
              while (*start == '\n') start++;
              if (!start) break;

              end = strchr(start, '\n');
              if (!end) end = &buf[ev->size - 1];

              size_t len = 1 + (end - start);
              char *line = malloc(len);
              if (line)
                {
                   snprintf(line, len, "%s", start);
                   chomp(line);
                   char *markup = elm_entry_utf8_to_markup(line);
                   if (markup)
                     {
                        elm_entry_entry_append(_info_widget, eina_slstr_printf("%s<br>", markup));
                        free(markup);
                     }
                   free(line);
                }
              start = end + 1;
           }
         ecore_thread_main_loop_end();

         elm_entry_cursor_pos_set(_info_widget, strlen(elm_object_text_get(_info_widget)));
    }

    return ECORE_CALLBACK_DONE;
}

static void
_edi_debugpanel_keypress_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Key_Down *event;
   const char *text_markup;
   char *command, *text;
   Edi_Debug *debug = edi_debug_get();

   event = event_info;

   if (!event) return;

   if (!event->key) return;

   if (!strcmp(event->key, "Return"))
     {
        if (!debug->exe) return;

        text_markup = elm_object_part_text_get(_entry_widget, NULL);
        text = elm_entry_markup_to_utf8(text_markup);
        if (text)
          {
             size_t len = strlen(text);
             if (!len) return;
             command = malloc(len + 2);
             snprintf(command, len + 2, "%s\n", text);
             ecore_exe_send(debug->exe, command, strlen(command));

             free(command);
             free(text);
          }
        elm_object_part_text_set(_entry_widget, NULL, "");
     }
}

static void
_edi_debugpanel_icons_update(Edi_Debug_Process_State state)
{
   Evas_Object *ico_int;

   ico_int = elm_icon_add(_button_int);

   if (state == EDI_DEBUG_PROCESS_ACTIVE)
     {
        elm_icon_standard_set(ico_int, "media-playback-pause");
     }
   else
     {
        elm_icon_standard_set(ico_int, "media-playback-start");
     }

   elm_object_part_content_set(_button_int, "icon", ico_int);
}

static void
_edi_debugpanel_bt_sigterm_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   pid_t pid;
   Evas_Object *ico_int;
   Edi_Debug *debug;

   debug = edi_debug_get();
   if (!debug) return;

   pid = edi_debug_process_id(debug);
   if (pid <= 0) return;

   ico_int = elm_icon_add(_button_int);
   elm_icon_standard_set(ico_int, "media-playback-pause");
   elm_object_part_content_set(_button_int, "icon", ico_int);

   kill(pid, SIGTERM);
}

static void
_edi_debugpanel_bt_sigint_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   pid_t pid;
   Edi_Debug *debug;

   debug = edi_debug_get();
   if (!debug) return;

   pid = edi_debug_process_id(debug);
   if (pid <= 0)
     {
        if (debug->tool->command_start)
          ecore_exe_send(debug->exe, debug->tool->command_start, strlen(debug->tool->command_start));
        return;
     }

   if (debug->state == EDI_DEBUG_PROCESS_ACTIVE)
     kill(pid, SIGINT);
   else if (debug->tool->command_continue)
     ecore_exe_send(debug->exe, debug->tool->command_continue, strlen(debug->tool->command_continue));
}

static void
_edi_debugpanel_button_quit_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   edi_debugpanel_stop();
}

static void
_edi_debugpanel_button_start_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   edi_debugpanel_start(_edi_project_config_debug_command_get());
}

void
edi_debugpanel_active_check(void)
{
   Edi_Debug *debug;
   int pid;

   debug = edi_debug_get();
   if (!debug) return;

   pid = ecore_exe_pid_get(debug->exe);
   if (pid == -1)
     {
        if (debug->exe) ecore_exe_quit(debug->exe);
        debug->exe = NULL;
        elm_object_disabled_set(_button_quit, EINA_TRUE);
        elm_object_disabled_set(_button_start, EINA_FALSE);
        elm_object_disabled_set(_button_int, EINA_TRUE);
        elm_object_disabled_set(_button_term, EINA_TRUE);
        return;
     }

   pid = edi_debug_process_id(debug);
   _edi_debugpanel_icons_update(pid > 0 ? debug->state : 0);
}

void edi_debugpanel_stop(void)
{
   Edi_Debug *debug;
   int pid;

   debug = edi_debug_get();
   if (!debug)
     return;

   if (debug->exe)
     ecore_exe_terminate(debug->exe);

   pid = ecore_exe_pid_get(debug->exe);
   if (pid != -1)
     ecore_exe_quit(debug->exe);

   debug->exe = NULL;

   elm_object_disabled_set(_button_quit, EINA_TRUE);
   elm_object_disabled_set(_button_int, EINA_TRUE);
   elm_object_disabled_set(_button_term, EINA_TRUE);
}

static void
_edi_debugger_run(Edi_Debug *debug)
{
   const char *fmt;
   char *args;
   int len;

   debug->exe = ecore_exe_pipe_run(debug->cmd, ECORE_EXE_PIPE_WRITE |
                                               ECORE_EXE_PIPE_ERROR |
                                               ECORE_EXE_PIPE_READ, NULL);

   if (debug->tool->command_arguments && _edi_project_config->launch.args)
     {
        fmt = debug->tool->command_arguments;
        len = strlen(fmt) + strlen(_edi_project_config->launch.args) + 1;
        args = malloc(len);
        snprintf(args, len, fmt, _edi_project_config->launch.args);
        ecore_exe_send(debug->exe, args, strlen(args));
        free(args);
     }
   if (debug->tool->command_settings)
     ecore_exe_send(debug->exe, debug->tool->command_settings, strlen(debug->tool->command_settings));

   if (debug->tool->command_start)
     ecore_exe_send(debug->exe, debug->tool->command_start, strlen(debug->tool->command_start));
}

void edi_debugpanel_start(const char *name)
{
   const char *mime;
   Edi_Debug *debug;

   debug = edi_debug_get();
   if (!debug) return;

   if (debug->exe) return;

   if (!_edi_project_config->launch.path)
     {
        edi_launcher_config_missing();
        return;
     }

   debug->program_name = ecore_file_file_get(_edi_project_config->launch.path);

   debug->tool = edi_debug_tool_get(name);
   if (!debug->tool)
     {
        edi_debug_exe_missing();
        return;
     }

   mime = edi_mime_type_get(_edi_project_config->launch.path);
   if (mime && !strcmp(mime, "application/x-shellscript"))
     snprintf(debug->cmd, sizeof(debug->cmd), LIBTOOL_COMMAND " --mode execute %s %s", debug->tool->exec, _edi_project_config->launch.path);
   else if (debug->tool->arguments)
     snprintf(debug->cmd, sizeof(debug->cmd), "%s %s %s", debug->tool->exec, debug->tool->arguments, _edi_project_config->launch.path);
   else
     snprintf(debug->cmd, sizeof(debug->cmd), "%s %s", debug->tool->exec, _edi_project_config->launch.path);

   elm_object_disabled_set(_button_int, EINA_FALSE);
   elm_object_disabled_set(_button_term, EINA_FALSE);
   elm_object_disabled_set(_button_quit, EINA_FALSE);
   elm_object_disabled_set(_button_start, EINA_TRUE);

   elm_object_text_set(_info_widget, "");

   _edi_debugger_run(debug);
}

void edi_debugpanel_add(Evas_Object *parent)
{
   Evas_Object *hbox, *frame, *box, *entry, *bt_term, *bt_int, *bt_start, *bt_quit;
   Evas_Object *separator;
   Evas_Object *ico_int, *ico_term;
   Evas_Object *widget;

   frame = elm_frame_add(parent);
   elm_object_text_set(frame, _("Debug"));
   evas_object_size_hint_weight_set(frame, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(frame, EVAS_HINT_FILL, EVAS_HINT_FILL);

   box = elm_box_add(parent);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);

   widget = elm_entry_add(parent);
   elm_entry_single_line_set(widget, EINA_FALSE);
   elm_entry_editable_set(widget, EINA_FALSE);
   elm_entry_scrollable_set(widget, EINA_TRUE);
   elm_entry_text_style_user_push(widget, eina_slstr_printf("DEFAULT='font=\\'%s\\' font_size=\\'%d\\''", _edi_project_config->font.name, _edi_project_config->font.size));
   evas_object_size_hint_weight_set(widget, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget);

   hbox = elm_box_add(parent);
   evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(hbox, EVAS_HINT_FILL, 0);
   elm_box_horizontal_set(hbox, EINA_TRUE);
   evas_object_show(hbox);

   separator = elm_separator_add(parent);
   elm_separator_horizontal_set(separator, EINA_FALSE);
   evas_object_show(separator);

   _button_term = bt_term = elm_button_add(parent);
   ico_term = elm_icon_add(parent);
   elm_icon_standard_set(ico_term, "media-playback-stop");
   elm_object_part_content_set(bt_term, "icon", ico_term);
   elm_object_tooltip_text_set(bt_term, "Send SIGTERM");
   elm_object_disabled_set(bt_term, EINA_TRUE);
   evas_object_smart_callback_add(bt_term, "clicked", _edi_debugpanel_bt_sigterm_cb, NULL);
   evas_object_show(bt_term);

   _button_int = bt_int = elm_button_add(parent);
   ico_int = elm_icon_add(parent);
   elm_icon_standard_set(ico_int, "media-playback-pause");
   elm_object_part_content_set(bt_int, "icon", ico_int);
   elm_object_tooltip_text_set(bt_int, "Start/Stop Process");
   elm_object_disabled_set(bt_int, EINA_TRUE);
   evas_object_smart_callback_add(bt_int, "clicked", _edi_debugpanel_bt_sigint_cb, NULL);
   evas_object_show(bt_int);

   _button_start = bt_start = elm_button_add(parent);
   elm_object_tooltip_text_set(bt_start, "Start Debugging");
   evas_object_size_hint_weight_set(bt_start, 0.05, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt_start, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(bt_start, _("Start"));
   evas_object_smart_callback_add(bt_start, "clicked", _edi_debugpanel_button_start_cb, NULL);
   evas_object_show(bt_start);

   _button_quit = bt_quit = elm_button_add(parent);
   elm_object_tooltip_text_set(bt_quit, "Quit Debugging");
   elm_object_disabled_set(bt_quit, EINA_TRUE);
   evas_object_size_hint_weight_set(bt_quit, 0.05, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt_quit, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(bt_quit, _("Quit"));
   evas_object_smart_callback_add(bt_quit, "clicked", _edi_debugpanel_button_quit_cb, NULL);
   evas_object_show(bt_quit);

   entry = elm_entry_add(parent);
   elm_entry_single_line_set(entry, EINA_TRUE);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   elm_entry_editable_set(entry, EINA_TRUE);
   evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_event_callback_add(entry, EVAS_CALLBACK_KEY_DOWN, _edi_debugpanel_keypress_cb, NULL);
   evas_object_show(entry);

   elm_box_pack_end(hbox, bt_term);
   elm_box_pack_end(hbox, bt_int);
   elm_box_pack_end(hbox, entry);
   elm_box_pack_end(hbox, bt_start);
   elm_box_pack_end(hbox, bt_quit);

   _info_widget = widget;
   _entry_widget = entry;

   edi_debug_new();

   elm_box_pack_end(box, widget);
   elm_box_pack_end(box, hbox);
   evas_object_show(box);

   elm_object_content_set(frame, box);
   evas_object_show(frame);
   elm_box_pack_end(parent, frame);

   ecore_event_handler_add(ECORE_EXE_EVENT_DATA, _debugpanel_stdout_handler, NULL);
   ecore_event_handler_add(ECORE_EXE_EVENT_ERROR, _debugpanel_stdout_handler, NULL);
   ecore_event_handler_add(EDI_EVENT_CONFIG_CHANGED, _edi_debugpanel_config_changed, NULL);
}
