#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eo.h>
#include <Eina.h>
#include <Elementary.h>

#include "edi_logpanel.h"
#include "edi_theme.h"
#include "edi_config.h"

#include "edi_private.h"

#define _EDI_LOG_ERROR "err"

static Evas_Object *_info_widget;
static Elm_Code *_elm_code;

static Eina_Bool
_edi_logpanel_ignore(Eina_Log_Level level, const char *fnc)
{
   if (level <= EINA_LOG_LEVEL_DBG)
     return !strncmp(fnc, "_eo_", 4) || !strncmp(fnc, "_evas_", 6) ||
            !strncmp(fnc, "_ecore_", 7) || !strncmp(fnc, "_edje_", 6) ||
            !strncmp(fnc, "_elm_", 5) || !strncmp(fnc, "_drm_", 5) ||
            !strncmp(fnc, "_eina_", 6);

   return !strncmp(fnc, "_evas_object_smart_need_recalculate_set", strlen(fnc));
}

static void
_edi_logpanel_print_cb(const Eina_Log_Domain *domain, Eina_Log_Level level,
                       const char *file, const char *fnc, int line, const char *fmt,
                       void *data EINA_UNUSED, va_list args)
{
   unsigned int printed, buffer_len = 512;
   char buffer [buffer_len];

   if (_edi_log_dom == -1) return;

   if (_edi_logpanel_ignore(level, fnc))
     return;

   printed = snprintf(buffer, buffer_len, "%s:%s:%s (%d): ",
           domain->domain_str, file, fnc, line);
   vsnprintf(buffer + printed, buffer_len - printed, fmt, args);

   ecore_thread_main_loop_begin();

   elm_code_file_line_append(_elm_code->file, buffer, strlen(buffer),
                             (level <= EINA_LOG_LEVEL_ERR) ? _EDI_LOG_ERROR : NULL);
   ecore_thread_main_loop_end();
}

static void
_edi_logpanel_line_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   Elm_Code_Line *line;

   line = (Elm_Code_Line *)event->info;

   if (line->data)
     line->status = ELM_CODE_STATUS_TYPE_ERROR;
}

static Eina_Bool
_edi_logpanel_config_changed(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   elm_code_widget_font_set(_info_widget, _edi_project_config->font.name, _edi_project_config->font.size);
   edi_theme_elm_code_set(_info_widget, _edi_project_config->gui.theme);

   return ECORE_CALLBACK_RENEW;
}

void edi_logpanel_add(Evas_Object *parent)
{
   Evas_Object *frame;
   Elm_Code_Widget *widget;
   Elm_Code *code;

   frame = elm_frame_add(parent);
   elm_object_text_set(frame, _("Logs"));
   evas_object_size_hint_weight_set(frame, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(frame, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(frame);

   code = elm_code_create();
   widget = elm_code_widget_add(parent, code);
   edi_theme_elm_code_set(widget, _edi_project_config->gui.theme);
   elm_obj_code_widget_font_set(widget, _edi_project_config->font.name, _edi_project_config->font.size);
   elm_obj_code_widget_gravity_set(widget, 0.0, 1.0);
   efl_event_callback_add(widget, &ELM_CODE_EVENT_LINE_LOAD_DONE, _edi_logpanel_line_cb, NULL);
   evas_object_size_hint_weight_set(widget, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget);

   _elm_code = code;
   _info_widget = widget;

   eina_log_print_cb_set(_edi_logpanel_print_cb, NULL);
   eina_log_color_disable_set(EINA_TRUE);

   elm_object_content_set(frame, widget);
   elm_box_pack_end(parent, frame);
   ecore_event_handler_add(EDI_EVENT_CONFIG_CHANGED, _edi_logpanel_config_changed, NULL);
}
