#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eo.h>
#include <Eina.h>
#include <Elementary.h>

#include <string.h>
#include "edi_file.h"
#include "edi_searchpanel.h"
#include "edi_config.h"
#include "mainview/edi_mainview.h"

#include "edi_private.h"

static Evas_Object *_info_widget, *_tasks_widget;
static Elm_Code *_elm_code, *_tasks_code;

static Ecore_Thread *_search_thread = NULL;
static Eina_Bool _searching = EINA_FALSE;
static char *_search_text = NULL;

static void
_edi_searchpanel_line_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   Elm_Code_Line *line;

   line = (Elm_Code_Line *)event->info;

   if (line->data)
     line->status = ELM_CODE_STATUS_TYPE_ERROR;
}

static Eina_Bool
_edi_searchpanel_config_changed_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   elm_code_widget_font_set(_info_widget, _edi_project_config->font.name, _edi_project_config->font.size);

   return ECORE_CALLBACK_RENEW;
}

static void
_edi_searchpanel_line_clicked_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   Elm_Code_Line *line;
   const char *content;
   unsigned int length;
   char *filename, *filename_end;
   char *line_start, *line_end;
   Eina_Bool success = EINA_FALSE;

   filename_end = line_start = line_end = NULL;

   line = (Elm_Code_Line *)event->info;
   if (!line) return;
   content = elm_code_line_text_get(line, &length);
   if (!content) return;

   filename = strdup(content);
   if (!filename) return;

   filename_end = strchr(filename, ':');
   if (filename_end)
     line_start = filename_end + 1;
   if (line_start)
     line_end = strchr(line_start, ' ');
   if (line_end)
     {
        *filename_end = '\0';
        *line_end = '\0';
        success = EINA_TRUE;
     }

   if (success)
     {
        edi_mainview_open_path(filename);
        edi_mainview_goto(atoi(line_start));
     }
   
   free(filename);
}

void _edi_searchpanel_search_project_file(const char *filename, const char *search_term)
{
   Elm_Code *code;
   Elm_Code_File *code_file; 
   Eina_List *item;
   Elm_Code_Line *line;
   unsigned int len;
   const char *text;
   char *tmp;
   static char buf[1024];
   static char data[1024];

   code = elm_code_create();
   code_file = elm_code_file_open(code, filename);

   EINA_LIST_FOREACH(code->file->lines, item, line)
     {
        int found = elm_code_line_text_strpos(line, search_term, 0);
        if (found != ELM_CODE_TEXT_NOT_FOUND) 
          {
             text = elm_code_line_text_get(line, &len);
             if (text)
               {
                  if (line->length >= sizeof(data))
                    len = sizeof(data);
                  else
                    len = line->length + 1;
                  snprintf(data, len, "%s", text);
                  snprintf(buf, sizeof(buf), "%s:%d -> %s", filename, line->number, data);
                  tmp = strdup(buf);
                  ecore_thread_feedback(_search_thread, tmp);
               }
          }
     }

    elm_code_file_close(code_file);
}

Eina_Bool
_file_ignore(const char *filename)
{
   if ((eina_str_has_extension(filename, ".png")   ||
        eina_str_has_extension(filename, ".PNG")   ||
        eina_str_has_extension(filename, ".jpg")   ||
        eina_str_has_extension(filename, ".jpeg")  ||
        eina_str_has_extension(filename, ".JPG")   ||
        eina_str_has_extension(filename, ".JPEG" ) ||
        eina_str_has_extension(filename, ".bmp")   ||
        eina_str_has_extension(filename, ".tgv")   ||
        eina_str_has_extension(filename, ".eet")   ||
        eina_str_has_extension(filename, ".edj")
       ))
     return EINA_TRUE;

   return EINA_FALSE;
}

void
_edi_searchpanel_search_project(const char *directory, const char *search_term)
{
   Eina_List *files, *item;
   char *file;
   char *path;

   files = ecore_file_ls(directory);

   EINA_LIST_FOREACH(files, item, file)
     {
        if (_file_ignore(file)) continue;

        path = edi_path_append(directory, file);
        if (!edi_file_path_hidden(path))
          {
             if (ecore_file_is_dir(path))
               _edi_searchpanel_search_project(path, search_term);
             else
               _edi_searchpanel_search_project_file(path, search_term);
          }

        free (path);
        if (ecore_thread_check(_search_thread)) return;
     }
}

static void
_search_feedback_cb(void *data EINA_UNUSED, Ecore_Thread *thread EINA_UNUSED, void *msg)
{
   char *text = msg;

   elm_code_file_line_append(_elm_code->file, text, strlen(text), NULL);
   free(text);
}

static void
_search_end_cb(void *data EINA_UNUSED, Ecore_Thread *thread EINA_UNUSED)
{
   _search_thread = NULL;
   _searching = EINA_FALSE;
}

static void
_search_cancel_cb(void *data EINA_UNUSED, Ecore_Thread *thread EINA_UNUSED)
{
   while ((ecore_thread_wait(_search_thread, 0.1)) != EINA_TRUE);
   _searching = EINA_FALSE;
}

static void
_search_begin_cb(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   const char *path = data;

   _searching = EINA_TRUE;

   _edi_searchpanel_search_project(path, _search_text);

   if (ecore_thread_check(_search_thread)) return;
}

void edi_searchpanel_find(const char *text)
{
   const char *path;

   if (!text || strlen(text) == 0) return;

   if (_searching) _search_cancel_cb(NULL, _search_thread);

   if (_search_text) free(_search_text);
   _search_text = strdup(text);

   path = edi_project_get();

   elm_code_file_clear(_elm_code->file);

   _search_thread = ecore_thread_feedback_run(_search_begin_cb, _search_feedback_cb,
                                      _search_end_cb, _search_cancel_cb,
                                      path, EINA_FALSE);
}

void edi_searchpanel_add(Evas_Object *parent)
{
   Elm_Code_Widget *widget;
   Elm_Code *code;
   code = elm_code_create();
   widget = elm_code_widget_add(parent, code);
   elm_obj_code_widget_font_set(widget, _edi_project_config->font.name, _edi_project_config->font.size);
   elm_obj_code_widget_gravity_set(widget, 0.0, 1.0);
   efl_event_callback_add(widget, &ELM_CODE_EVENT_LINE_LOAD_DONE, _edi_searchpanel_line_cb, NULL);
   efl_event_callback_add(widget, ELM_OBJ_CODE_WIDGET_EVENT_LINE_CLICKED, _edi_searchpanel_line_clicked_cb, NULL);
   evas_object_size_hint_weight_set(widget, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget);

   _elm_code = code;
   _info_widget = widget;

   elm_box_pack_end(parent, widget);
   ecore_event_handler_add(EDI_EVENT_CONFIG_CHANGED, _edi_searchpanel_config_changed_cb, NULL);
}

static void
_edi_taskspanel_line_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   Elm_Code_Line *line;

   line = (Elm_Code_Line *)event->info;

   if (line->data)
     line->status = ELM_CODE_STATUS_TYPE_ERROR;
}

static Eina_Bool
_edi_taskspanel_config_changed_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   elm_code_widget_font_set(_tasks_widget, _edi_project_config->font.name, _edi_project_config->font.size);

   return ECORE_CALLBACK_RENEW;
}

#define _edi_taskspanel_line_clicked_cb _edi_searchpanel_line_clicked_cb

static void
_tasks_feedback_cb(void *data EINA_UNUSED, Ecore_Thread *thread EINA_UNUSED, void *msg)
{
   char *text = msg;

   elm_code_file_line_append(_tasks_code->file, text, strlen(text), NULL);
   free(text);
}

static void
_tasks_begin_cb(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   const char *path = data;

   _edi_searchpanel_search_project(path, "TODO");
   if (ecore_thread_check(_search_thread)) return;
   _edi_searchpanel_search_project(path, "FIXME");
   if (ecore_thread_check(_search_thread)) return;
   _edi_searchpanel_search_project(path, "XXX");
   if (ecore_thread_check(_search_thread)) return;
}

void edi_taskspanel_find(void)
{
   const char *path;
   if (_searching) return;

   elm_code_file_clear(_tasks_code->file);

   path = edi_project_get();

   _search_thread = ecore_thread_feedback_run(_tasks_begin_cb, _tasks_feedback_cb,
                                             _search_end_cb, _search_cancel_cb,
                                             path, EINA_FALSE);
}

void edi_taskspanel_add(Evas_Object *parent)
{
   Elm_Code_Widget *widget;
   Elm_Code *code;
   code = elm_code_create();
   widget = elm_code_widget_add(parent, code);
   elm_obj_code_widget_font_set(widget, _edi_project_config->font.name, _edi_project_config->font.size);
   elm_obj_code_widget_gravity_set(widget, 0.0, 1.0);
   efl_event_callback_add(widget, &ELM_CODE_EVENT_LINE_LOAD_DONE, _edi_taskspanel_line_cb, NULL);
   efl_event_callback_add(widget, ELM_OBJ_CODE_WIDGET_EVENT_LINE_CLICKED, _edi_taskspanel_line_clicked_cb, NULL);
   evas_object_size_hint_weight_set(widget, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget);

   _tasks_code = code;
   _tasks_widget = widget;

   elm_box_pack_end(parent, widget);
   ecore_event_handler_add(EDI_EVENT_CONFIG_CHANGED, _edi_taskspanel_config_changed_cb, NULL);

   edi_taskspanel_find();
}

