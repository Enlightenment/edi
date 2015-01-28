#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Elm_Code.h>

#include "edi_logpanel.h"
#include "edi_config.h"

#include "edi_private.h"

static Evas_Object *_info_widget;
static Elm_Code *_elm_code;

static void _print_cb(const Eina_Log_Domain *domain,
              Eina_Log_Level level,
              const char *file,
              const char *fnc,
              int line,
              const char *fmt,
              EINA_UNUSED void *data,
              va_list args)
{
   unsigned int printed, line_count, buffer_len = 512;
   char buffer [buffer_len];

   printed = snprintf(buffer, buffer_len, "%s:%s:%s (%d): ",
           domain->domain_str, file, fnc, line);
   vsnprintf(buffer + printed, buffer_len - printed, fmt, args);

   elm_code_file_line_append(_elm_code->file, buffer, strlen(buffer));
   if (level <= EINA_LOG_LEVEL_ERR)
     {
        line_count = elm_code_file_lines_get(_elm_code->file);

        elm_code_file_line_status_set(_elm_code->file, line_count, ELM_CODE_STATUS_TYPE_ERROR);
     }
}

void edi_logpanel_add(Evas_Object *parent)
{
   Elm_Code_Widget *widget;
   Elm_Code *code;

   code = elm_code_create();
   widget = eo_add(ELM_CODE_WIDGET_CLASS, parent);
   eo_do(widget,
         elm_code_widget_code_set(code),
         elm_code_widget_font_size_set(_edi_cfg->font.size),
         elm_code_widget_gravity_set(0.0, 1.0));
   evas_object_size_hint_weight_set(widget, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget);

   _elm_code = code;
   _info_widget = widget;

   eina_log_print_cb_set(_print_cb, NULL);
   eina_log_color_disable_set(EINA_TRUE);

   elm_box_pack_end(parent, widget);
}
