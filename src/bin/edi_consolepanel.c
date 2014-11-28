#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define BUFFER_SIZE 1024

#include <Eina.h>
#include <Ecore.h>
#include <Elm_Code.h>
#include <Elementary.h>
#include <Elementary_Cursor.h>
#include <regex.h>

#include "edi_consolepanel.h"
#include "mainview/edi_mainview.h"

#include "edi_private.h"

static Evas_Object *_console_box;
static const char *_current_dir = NULL;

static int _edi_test_count;
static int _edi_test_pass;
static int _edi_test_fail;

static Elm_Code *_edi_test_code;
static void _edi_test_line_callback(const char *content);

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
                 Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
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

   pos = strstr(line, "Entering directory ");
   if (pos) 
     {
        if (_current_dir)
          eina_stringshare_del(_current_dir);

        _current_dir = eina_stringshare_add_length(pos + 20, strlen(pos) - 21);
     }
}

static void _edi_consolepanel_scroll_to_bottom()
{
   Evas_Object *scroller;
   Evas_Coord x, y, w, h;

   scroller = elm_object_parent_widget_get(_console_box);
   evas_object_geometry_get(_console_box, &x, &y, &w, &h);
   elm_scroller_region_show(scroller, x, h - 10, w, 10);
}

static void _edi_consolepanel_append_line_type(const char *line, Eina_Bool err)
{
   Evas_Object *txt, *icon, *box;
   char *buf, *path;
   const char *pos, *file, *type = NULL, *cursor = NULL;
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
   _edi_consolepanel_scroll_to_bottom();

   _edi_test_line_callback(line);
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

   elm_code_file_clear(_edi_test_code->file);
   _edi_test_count = 0;
   _edi_test_pass = 0;
   _edi_test_fail = 0;
}

static Eina_Bool
_exe_data(void *d EINA_UNUSED, int t EINA_UNUSED, void *event_info)
{
   Ecore_Exe_Event_Data *ev;
   Ecore_Exe_Event_Data_Line *el;

   ev = event_info;
   for (el = ev->lines; el && el->line; el++)
     edi_consolepanel_append_line(el->line);

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_exe_error(void *d EINA_UNUSED, int t EINA_UNUSED, void *event_info)
{
   Ecore_Exe_Event_Data *ev;
   Ecore_Exe_Event_Data_Line *el;

   ev = event_info;
   for (el = ev->lines; el && el->line; el++)
     edi_consolepanel_append_error_line(el->line);

   return ECORE_CALLBACK_RENEW;
}

static void _edi_test_append(const char *content, int length, Elm_Code_Status_Type type)
{
   elm_code_file_line_append(_edi_test_code->file, content, length);
   elm_code_file_line_status_set(_edi_test_code->file, elm_code_file_lines_get(_edi_test_code->file), type);
}

static  Eina_Bool _edi_test_line_contains(const char *start, unsigned int length, const char *needle)
{
   unsigned int needlelen, c;
   char *ptr;

   ptr = (char *) start;
   needlelen = strlen(needle);

   for (c = 0; c < length - strlen(needle); c++)
     {
        if (!strncmp(ptr, needle, needlelen))
           return EINA_TRUE;

        ptr++;
     }

   return EINA_FALSE;
}

static void _edi_test_line_parse_suite(const char *path)
{
   Eina_File *file;
   Eina_File_Line *line;
   Eina_Iterator *it;
   char logfile[PATH_MAX];
   int pathlength;
   Elm_Code_Status_Type status;

   pathlength = strlen(path);
   snprintf(logfile, pathlength + 4 + 1, "%s.log", path);

   file = eina_file_open(logfile, EINA_FALSE);

   it = eina_file_map_lines(file);
   EINA_ITERATOR_FOREACH(it, line)
     {
        status = ELM_CODE_STATUS_TYPE_DEFAULT;

        if (_edi_test_line_contains(line->start, line->length, ":P:"))
           status = ELM_CODE_STATUS_TYPE_PASSED;
        else if (_edi_test_line_contains(line->start, line->length, ":F:"))
           status = ELM_CODE_STATUS_TYPE_FAILED;

        _edi_test_append(line->start, line->length, status);
     }
   eina_iterator_free(it);
   eina_file_close(file);
}

static void _edi_test_line_parse_suite_pass_line(const char *line)
{
   _edi_test_line_parse_suite(line);
   _edi_test_append("Suite passed", 13, ELM_CODE_STATUS_TYPE_DEFAULT);
}

static void _edi_test_line_parse_suite_fail_line(const char *line)
{
   _edi_test_line_parse_suite(line);
   _edi_test_append("Suite failed", 13, ELM_CODE_STATUS_TYPE_DEFAULT);
}

static void _edi_test_line_parse_summary_line(const char *line)
{
   _edi_test_append(line, strlen(line), ELM_CODE_STATUS_TYPE_DEFAULT);
}

static void _edi_test_line_callback(const char *content)
{
   if (!content)
     return;

   if (content[0] == '#')
     {
        _edi_test_line_parse_summary_line(content + 2);
        return;
     }

   if (!strncmp(content, "PASS:", 5))
     {
        _edi_test_count++;
        _edi_test_pass++;
        _edi_test_line_parse_suite_pass_line(content + 6);
     }
   else if (!strncmp(content, "FAIL:", 5))
     {
        _edi_test_count++;
        _edi_test_fail++;
        _edi_test_line_parse_suite_fail_line(content + 6);
     }
}

void edi_consolepanel_add(Evas_Object *parent)
{
   Evas_Object *scroll, *vbx;

   scroll = elm_scroller_add(parent);
   evas_object_size_hint_weight_set(scroll, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(scroll, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(scroll);

   vbx = elm_box_add(parent);
   evas_object_size_hint_weight_set(vbx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(vbx);
   elm_object_content_set(scroll, vbx);

   _console_box = vbx;

   elm_box_pack_end(parent, scroll);

   ecore_event_handler_add(ECORE_EXE_EVENT_DATA, _exe_data, NULL);
   ecore_event_handler_add(ECORE_EXE_EVENT_ERROR, _exe_error, NULL);
}

void edi_testpanel_add(Evas_Object *parent)
{
   Elm_Code *code;
   Evas_Object *widget;

   code = elm_code_create();
   _edi_test_code = code;

   widget = elm_code_widget_add(parent, code);
   elm_code_widget_font_size_set(widget, 12);

   evas_object_size_hint_weight_set(widget, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget);

   elm_box_pack_end(parent, widget);
}

