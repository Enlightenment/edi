#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define BUFFER_SIZE 1024

#include <Eina.h>
#include <Ecore.h>
#include <Elementary.h>
#include <Elementary_Cursor.h>
#include <regex.h>

#include "edi_consolepanel.h"
#include "edi_mainview.h"

#include "edi_private.h"

static Evas_Object *_console_box;
static const char *_current_dir = NULL;

static const char *_edi_consolepanel_icon_for_line(const char *line)
{
   if (strstr(line, " error:") != NULL)
     return "error";
   if (strstr(line, " warning:") != NULL)
     return "important";
   if (strstr(line, " info:") != NULL || strstr(line, " note:") != NULL)
     return "info";

   return NULL;
}

static Eina_Bool _startswith_location(const char *line)
{
   regex_t regex;
   int ret;

   regcomp(&regex, "^[^/].*:[0-9]*:[0-9]* ", 0);
   ret = !regexec(&regex, line, 0, NULL, 0);

   regfree(&regex);
   return ret;
}

static void _edi_consolepanel_clicked_cb(void *data, Evas *e EINA_UNUSED,
                 Evas_Object *obj EINA_UNUSED, Evas_Event_Mouse_Down *ev)
{
   Edi_Path_Options *options;

   if (strstr(data, edi_project_get()) != data)
     return;

   options = edi_path_options_create(data);
   edi_mainview_open(options);
}

static void _edi_consolepanel_parse_directory(const char *line)
{
   const char *pos;

   pos = strstr(line, "Entering directory '");
   if (pos) 
     {
        if (_current_dir)
          eina_stringshare_del(_current_dir);

        _current_dir = eina_stringshare_add_length(pos + 20, strlen(pos) - 21);
     }
}

static void _edi_consolepanel_append_line_type(const char *line, Eina_Bool err)
{
   Evas_Object *txt, *icon, *box;
   const char *buf, *pos, *file, *path, *type = NULL, *cursor = NULL;
   int length;

   txt = elm_label_add(_console_box);

   if (err)
     evas_object_color_set(txt, 255, 63, 63, 255);
   else
     evas_object_color_set(txt, 255, 255, 255, 255);

   if (_startswith_location(line))
     {
        cursor = ELM_CURSOR_HAND1;
        elm_object_cursor_set(txt, cursor);
        elm_object_cursor_theme_search_enabled_set(txt, EINA_TRUE);

        pos = strstr(line, " ");
        length = strlen(line) - strlen(pos);
        file = eina_stringshare_add_length(line, length);

        buf = malloc(sizeof(char) * (strlen(line) + 8));
        snprintf(buf, strlen(line) + 8, "<b>%s</b>%s/n", file, pos);
        elm_object_text_set(txt, buf);

        length = strlen(_current_dir) + strlen(file) + 2;
        path = malloc(sizeof(char) * length);
        snprintf(path, length, "%s/%s\n", _current_dir, file);

        evas_object_event_callback_add(txt, EVAS_CALLBACK_MOUSE_DOWN,
                                       _edi_consolepanel_clicked_cb, eina_stringshare_add(path));

        free(path);
        free(buf);
     }
   else
     {
        _edi_consolepanel_parse_directory(line);
        elm_object_text_set(txt, line);
     }

   evas_object_size_hint_weight_set(txt, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(txt, 0.0, EVAS_HINT_FILL);
   evas_object_show(txt);

   if (err)
     type = _edi_consolepanel_icon_for_line(line);

   icon = elm_icon_add(_console_box);
   evas_object_size_hint_min_set(icon, 14, 14);
   evas_object_size_hint_max_set(icon, 14, 14);
   if (type)
     {
        elm_icon_standard_set(icon, type);
        if (cursor)
          {
             elm_object_cursor_set(icon, cursor);
             elm_object_cursor_theme_search_enabled_set(icon, EINA_TRUE);
          }
     }
   evas_object_show(icon);

   box = elm_box_add(_console_box);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_horizontal_set(box, EINA_TRUE);
   elm_box_pack_end(box, icon);
   elm_box_pack_end(box, txt);
   evas_object_show(box);

   elm_box_pack_end(_console_box, box);
}

void edi_consolepanel_append_line(const char *line)
{
   _edi_consolepanel_append_line_type(line, EINA_FALSE);
}

void edi_consolepanel_append_error_line(const char *line)
{
   _edi_consolepanel_append_line_type(line, EINA_TRUE);
}

void edi_consolepanel_clear()
{
   elm_box_clear(_console_box);
}

static Eina_Bool _stdin_handler_cb(void *data EINA_UNUSED, Ecore_Fd_Handler *fd_handler EINA_UNUSED)
{
   char message[BUFFER_SIZE];

   if (!fgets(message, BUFFER_SIZE, stdin))
     return ECORE_CALLBACK_RENEW;

   edi_consolepanel_append_line(message);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool _stderr_handler_cb(void *data EINA_UNUSED, Ecore_Fd_Handler *fd_handler EINA_UNUSED)
{
   char message[BUFFER_SIZE];

   if (!fgets(message, BUFFER_SIZE, stderr))
     return ECORE_CALLBACK_RENEW;

   edi_consolepanel_append_error_line(message);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
exe_data(void *d EINA_UNUSED, int t EINA_UNUSED, Ecore_Exe_Event_Data *ev)
{
   Ecore_Exe_Event_Data_Line *el;

   for (el = ev->lines; el && el->line; el++)
     edi_consolepanel_append_line(el->line);

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
exe_error(void *d EINA_UNUSED, int t EINA_UNUSED, Ecore_Exe_Event_Data *ev)
{
   Ecore_Exe_Event_Data_Line *el;

   for (el = ev->lines; el && el->line; el++)
     edi_consolepanel_append_error_line(el->line);

   return ECORE_CALLBACK_RENEW;
}

void edi_consolepanel_add(Evas_Object *parent)
{
   Evas_Object *scroll, *vbx;

   scroll = elm_scroller_add(parent);
   elm_scroller_gravity_set(scroll, 0.0, 0.0);
   evas_object_size_hint_weight_set(scroll, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(scroll, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(scroll);

   vbx = elm_box_add(parent);
   evas_object_size_hint_weight_set(vbx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(vbx);
   elm_object_content_set(scroll, vbx);

   _console_box = vbx;

   elm_object_content_set(parent, scroll);

   ecore_event_handler_add(ECORE_EXE_EVENT_DATA, exe_data, NULL);
   ecore_event_handler_add(ECORE_EXE_EVENT_ERROR, exe_error, NULL);
}
