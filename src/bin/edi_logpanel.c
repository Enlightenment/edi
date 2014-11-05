#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Elm_Code.h>

#include "edi_logpanel.h"

#include "edi_private.h"

static Evas_Object *_info_widget;
static Elm_Code *_elm_code;

void print_cb(const Eina_Log_Domain *domain,
              Eina_Log_Level level,
              const char *file,
              const char *fnc,
              int line,
              const char *fmt,
              EINA_UNUSED void *data,
              va_list args)
{
   Elm_Code_Line *code_line;
   unsigned int printed, line_count, buffer_len = 512;
   char buffer [buffer_len];

   printed = snprintf(buffer, buffer_len, "%s:%s:%s (%d): ",
           domain->domain_str, file, fnc, line);
   vsnprintf(buffer + printed, buffer_len - printed, fmt, args);

   elm_code_file_line_append(_elm_code->file, buffer);
   if (level <= EINA_LOG_LEVEL_ERR)
     {
        line_count = elm_code_file_lines_get(_elm_code->file);
        code_line = elm_code_file_line_get(_elm_code->file, line_count);

        code_line->status = ELM_CODE_STATUS_TYPE_ERROR;
     }
}

void edi_logpanel_add(Evas_Object *parent)
{
   Evas_Object *widget;
   Elm_Code *code;

   code = elm_code_create(elm_code_file_new());
   widget = elm_code_widget_add(parent, code);
   evas_object_size_hint_weight_set(widget, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget);

   _elm_code = code;
   _info_widget = widget;

   eina_log_print_cb_set(print_cb, NULL);
   eina_log_color_disable_set(EINA_TRUE);

   elm_object_content_set(parent, widget);
}
