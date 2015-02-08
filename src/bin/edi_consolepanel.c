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
#include "edi_config.h"

#include "edi_private.h"

static const char *_current_dir = NULL;
static const char *_current_test_dir = NULL;

static unsigned int _edi_strlen_current_dir;
static int _edi_test_count;
static int _edi_test_pass;
static int _edi_test_fail;

static Elm_Code *_edi_test_code, *_edi_console_code;
static void _edi_test_line_callback(const char *content);

static Eina_Bool
_edi_consolepanel_startswith_location(const char *line)
{
   regex_t regex;
   int ret;

   regcomp(&regex, "^[^/].*:[0-9]*:[0-9]*[ :]", 0);
   ret = !regexec(&regex, line, 0, NULL, 0);

   regfree(&regex);
   return ret;
}

static char *
_edi_consolepanel_extract_location(const char *line, const char *dir, int dirlen)
{
   char *path;
   const char *pos;
   int file_len, length;

   pos = strstr(line, ":F:");
   if (!pos)
     pos = strstr(line, ":P:");
   if (!pos)
     pos = strstr(line, " ");
   file_len = strlen(line) - strlen(pos);

   length = dirlen + file_len + 2;
   path = malloc(sizeof(char) * length);
   snprintf(path, dirlen + 2, "%s/", dir);
   snprintf(path + dirlen + 1, file_len + 1, "%s", line);

   return path;
}

static void _edi_consolepanel_parse_directory(const char *line)
{
   const char *pos;

   pos = strstr(line, "Entering directory ");
   if (pos) 
     {
        if (_current_dir)
          eina_stringshare_del(_current_dir);

        _edi_strlen_current_dir = strlen(pos) - 21;
        _current_dir = eina_stringshare_add_length(pos + 20, _edi_strlen_current_dir);
     }
}

static Eina_Bool
_edi_consolepanel_clicked_cb(void *data, Eo *obj EINA_UNUSED,
                             const Eo_Event_Description *desc EINA_UNUSED, void *event_info)
{
   Edi_Path_Options *options;
   Elm_Code *code;
   Elm_Code_Line *line;
   const char *content;
   char *path, *terminated;
   int length;

   code = (Elm_Code *)data;
   line = (Elm_Code_Line *)event_info;

   content = elm_code_file_line_content_get(code->file, line->number, &length);

   terminated = malloc(sizeof(char) * (length + 1));
   snprintf(terminated, length, "%s", content);

   if (_edi_consolepanel_startswith_location(terminated))
     {
        if (code == _edi_console_code)
          path = _edi_consolepanel_extract_location(terminated, _current_dir, _edi_strlen_current_dir);
        else
          path = _edi_consolepanel_extract_location(terminated, _current_test_dir, strlen(_current_test_dir));

        if (strstr(path, edi_project_get()) == path)
          {
             options = edi_path_options_create(path);
             edi_mainview_open(options);
          }
     }

   free(terminated);
   return EINA_TRUE;
}

static void _edi_consolepanel_append_line_type(const char *line, Eina_Bool err)
{
   _edi_consolepanel_parse_directory(line);

   elm_code_file_line_append(_edi_console_code->file, line, strlen(line));
   if (err)
     elm_code_file_line_status_set(_edi_console_code->file, elm_code_file_lines_get(_edi_console_code->file),
                                   ELM_CODE_STATUS_TYPE_ERROR);

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
   elm_code_file_clear(_edi_console_code->file);
   elm_code_file_clear(_edi_test_code->file);

   _edi_test_count = _edi_test_pass = _edi_test_fail = 0;
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

static void
_edi_test_append(const char *content, int length, Elm_Code_Status_Type type)
{
   elm_code_file_line_append(_edi_test_code->file, content, length);
   elm_code_file_line_status_set(_edi_test_code->file, elm_code_file_lines_get(_edi_test_code->file), type);
}

static void
_edi_test_output_suite(int count, int pass, int fail)
{
   char *line;
   const char *format = "Total pass %d (%d%%), fail %d";
   int linemax, percent;

   linemax = strlen(format) - 6 + 30;
   line = malloc(sizeof(char) * linemax);

   percent = 0;
   if (count > 0)
     percent = (int) ((pass / (double) count) * 100);

   snprintf(line, linemax, format, pass, percent, fail);
   _edi_test_append(line, strlen(line), (fail > 0) ? ELM_CODE_STATUS_TYPE_FAILED : ELM_CODE_STATUS_TYPE_PASSED);
   free(line);
}

static Eina_Bool
_edi_test_line_contains(const char *start, unsigned int length, const char *needle)
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

static void
_edi_test_line_parse_suite(const char *path)
{
   Eina_File *file;
   Eina_File_Line *line;
   Eina_Iterator *it;
   char logfile[PATH_MAX], logpath[PATH_MAX];
   int pathlength;

   pathlength = strlen(path);
   snprintf(logfile, pathlength + 4 + 1, "%s.log", path);
   realpath(logfile, logpath);
   if (_current_test_dir)
     eina_stringshare_del(_current_test_dir);
   _current_test_dir = eina_stringshare_add(dirname(logpath));

   file = eina_file_open(logfile, EINA_FALSE);

   it = eina_file_map_lines(file);
   EINA_ITERATOR_FOREACH(it, line)
     {
        if (_edi_test_line_contains(line->start, line->length, ":P:"))
          {
             _edi_test_count++;
             _edi_test_pass++;
             continue;
          }
        else if (_edi_test_line_contains(line->start, line->length, ":F:"))
          {
             _edi_test_count++;
             _edi_test_fail++;
             _edi_test_append(line->start, line->length, ELM_CODE_STATUS_TYPE_FAILED);
          }
        else if (_edi_test_line_contains(line->start, line->length, "Running"))
          {
             _edi_test_count = _edi_test_pass = _edi_test_fail = 0;
             _edi_test_append(line->start, line->length, ELM_CODE_STATUS_TYPE_DEFAULT);
          }
     }
   eina_iterator_free(it);
   eina_file_close(file);

   if (_edi_test_count > 0)
     {
        _edi_test_output_suite(_edi_test_count, _edi_test_pass, _edi_test_fail);
     }
}

static void _edi_test_line_callback(const char *content)
{
   if (!content)
     return;

   if (content[0] == '#')
     {
        return;
     }

   if (!strncmp(content, "PASS:", 5))
     {
        _edi_test_line_parse_suite(content + 6);
     }
   else if (!strncmp(content, "FAIL:", 5))
     {
        _edi_test_line_parse_suite(content + 6);
     }
}

void edi_consolepanel_add(Evas_Object *parent)
{
   Elm_Code *code;
   Elm_Code_Widget *widget;

   code = elm_code_create();
   _edi_console_code = code;

   widget = eo_add(ELM_CODE_WIDGET_CLASS, parent);
   eo_do(widget,
         elm_code_widget_code_set(code),
         elm_code_widget_font_size_set(_edi_cfg->font.size),
         elm_code_widget_gravity_set(0.0, 1.0),
         eo_event_callback_add(ELM_CODE_WIDGET_EVENT_LINE_CLICKED, _edi_consolepanel_clicked_cb, code));

   evas_object_size_hint_weight_set(widget, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget);

   elm_box_pack_end(parent, widget);

   ecore_event_handler_add(ECORE_EXE_EVENT_DATA, _exe_data, NULL);
   ecore_event_handler_add(ECORE_EXE_EVENT_ERROR, _exe_error, NULL);
}

void edi_testpanel_add(Evas_Object *parent)
{
   Elm_Code *code;
   Elm_Code_Widget *widget;

   code = elm_code_create();
   _edi_test_code = code;

   widget = eo_add(ELM_CODE_WIDGET_CLASS, parent);
   eo_do(widget,
         elm_code_widget_code_set(code),
         elm_code_widget_font_size_set(_edi_cfg->font.size),
         elm_code_widget_gravity_set(0.0, 1.0),
         eo_event_callback_add(ELM_CODE_WIDGET_EVENT_LINE_CLICKED, _edi_consolepanel_clicked_cb, code));

   evas_object_size_hint_weight_set(widget, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget);

   elm_box_pack_end(parent, widget);
}

