#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define BUFFER_SIZE 1024

#include <Eina.h>
#include <Ecore.h>

#include "edi_consolepanel.h"

#include "edi_private.h"

static Evas_Object *_console_box;

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

static void _edi_consolepanel_append_line_type(const char *line, Eina_Bool err)
{
   Evas_Object *txt, *icon, *box;
   const char *type = NULL;

   txt = elm_label_add(_console_box);
   if (err)
     evas_object_color_set(txt, 255, 63, 63, 255);
   else
     evas_object_color_set(txt, 255, 255, 255, 255);

   elm_object_text_set(txt, line);
   evas_object_size_hint_weight_set(txt, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(txt, 0.0, EVAS_HINT_FILL);
   evas_object_show(txt);

   if (err)
     type = _edi_consolepanel_icon_for_line(line);

   icon = elm_icon_add(_console_box);
   evas_object_size_hint_min_set(icon, 14, 14);
   evas_object_size_hint_max_set(icon, 14, 14);
   if (type)
     elm_icon_standard_set(icon, type);
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
