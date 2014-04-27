#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>

#include "edi_logpanel.h"

#include "edi_private.h"

static Evas_Object *_info_box;

void print_cb(const Eina_Log_Domain *domain,
              Eina_Log_Level level,
              const char *file,
              const char *fnc,
              int line,
              const char *fmt,
              EINA_UNUSED void *data,
              va_list args)
{
   Evas_Object *txt;
   unsigned int printed, buffer_len = 512;
   char buffer [buffer_len];

   printed = snprintf(buffer, buffer_len, "%s:%s:%s (%d): ",
           domain->domain_str, file, fnc, line);
   vsnprintf(buffer + printed, buffer_len - printed, fmt, args);

   txt = elm_label_add(_info_box);
   if (level <= EINA_LOG_LEVEL_ERR)
     evas_object_color_set(txt, 255, 63, 63, 255);
   else
     evas_object_color_set(txt, 255, 255, 255, 255);

   elm_object_text_set(txt, buffer);
   evas_object_size_hint_weight_set(txt, EVAS_HINT_EXPAND, 0.1);
   evas_object_size_hint_align_set(txt, 0.0, EVAS_HINT_FILL);
   evas_object_show(txt);

   elm_box_pack_end(_info_box, txt);
}

void edi_logpanel_add(Evas_Object *parent)
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

   _info_box = vbx;

   eina_log_print_cb_set(print_cb, NULL);
   eina_log_color_disable_set(EINA_TRUE);

   elm_object_content_set(parent, scroll);
}
