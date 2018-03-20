#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Elementary.h>
#include <Ecore.h>

#include "edi_private.h"
#include "edi_screens.h"

static Evas_Object *_edi_screens_popup = NULL;

static void
_edi_screens_popup_cancel_cb(void *data, Evas_Object *obj EINA_UNUSED,
                             void *event_info EINA_UNUSED)
{
   evas_object_del((Evas_Object *) data);
}

static void
_edi_screens_message_confirm_cb(void *data, Evas_Object *obj,
                                void *event_info EINA_UNUSED)
{
   void ((*confirm_fn)(void *)) = evas_object_data_get(obj, "callback");

   confirm_fn(data);

   if (_edi_screens_popup)
     {
        evas_object_del(_edi_screens_popup);
        _edi_screens_popup = NULL;
     }
}

void edi_screens_message_confirm(Evas_Object *parent, const char *message, void ((*confirm_cb)(void *)), void *data)
{
   Evas_Object *popup, *frame, *table, *label, *button, *icon, *box, *sep;

   _edi_screens_popup = popup = elm_popup_add(parent);
   elm_object_part_text_set(popup, "title,text", _("Confirmation required"));

   table = elm_table_add(popup);

   icon = elm_icon_add(table);
   elm_icon_standard_set(icon, "dialog-question");
   evas_object_size_hint_min_set(icon, 48 * elm_config_scale_get(), 48 * elm_config_scale_get());
   evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(icon);
   elm_table_pack(table, icon, 0, 0, 1, 1);

   label = elm_label_add(table);
   elm_object_text_set(label, message);
   evas_object_show(label);

   elm_table_pack(table, label, 1, 0, 1, 1);
   evas_object_show(table);

   box = elm_box_add(popup);
   elm_box_pack_end(box, table);
   sep = elm_separator_add(box);
   elm_separator_horizontal_set(sep, EINA_TRUE);
   evas_object_show(sep);
   elm_box_pack_end(box, sep);

   frame = elm_frame_add(popup);
   evas_object_show(frame);
   elm_object_content_set(frame, box);
   elm_object_content_set(popup, frame);

   button = elm_button_add(popup);
   elm_object_text_set(button, _("Yes"));
   elm_object_part_content_set(popup, "button1", button);
   evas_object_data_set(button, "callback", confirm_cb);
   evas_object_smart_callback_add(button, "clicked", _edi_screens_message_confirm_cb, data);

   button = elm_button_add(popup);
   elm_object_text_set(button, _("No"));
   elm_object_part_content_set(popup, "button2", button);
   evas_object_smart_callback_add(button, "clicked", _edi_screens_popup_cancel_cb, popup);

   evas_object_show(popup);
}

void edi_screens_message(Evas_Object *parent, const char *title, const char *message)
{
   Evas_Object *popup, *table, *box, *icon, *sep, *label, *button;

   popup = elm_popup_add(parent);
   elm_object_part_text_set(popup, "title,text", title);

   table = elm_table_add(popup);
   icon = elm_icon_add(table);
   elm_icon_standard_set(icon, "dialog-information");
   evas_object_size_hint_min_set(icon, 48 * elm_config_scale_get(), 48 * elm_config_scale_get());
   evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(icon);
   elm_table_pack(table, icon, 0, 0, 1, 1);

   label = elm_label_add(popup);
   elm_object_text_set(label, message);
   evas_object_show(label);
   elm_table_pack(table, label, 1, 0, 1, 1);
   evas_object_show(table);

   box = elm_box_add(popup);
   sep = elm_separator_add(box);
   elm_separator_horizontal_set(sep, EINA_TRUE);
   evas_object_show(sep);
   elm_box_pack_end(box, sep);
   elm_box_pack_end(box, table);
   sep = elm_separator_add(box);
   elm_separator_horizontal_set(sep, EINA_TRUE);
   evas_object_show(sep);
   elm_box_pack_end(box, sep);

   elm_object_content_set(popup, box);

   button = elm_button_add(popup);
   elm_object_text_set(button, _("OK"));
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked", _edi_screens_popup_cancel_cb, popup);

   evas_object_show(popup);
}

static void
_edi_screens_settings_display_cb(void *data, Evas_Object *obj,
                             void *event_info EINA_UNUSED)
{
   Evas_Object *parent = evas_object_data_get(obj, "parent");

   evas_object_del((Evas_Object *) data);

   edi_settings_show(parent);
}

void edi_screens_settings_message(Evas_Object *parent, const char *title, const char *message)
{
   Evas_Object *popup, *table, *box, *icon, *sep, *label, *button;

   popup = elm_popup_add(parent);
   elm_object_part_text_set(popup, "title,text", title);

   table = elm_table_add(popup);
   icon = elm_icon_add(table);
   elm_icon_standard_set(icon, "dialog-information");
   evas_object_size_hint_min_set(icon, 48 * elm_config_scale_get(), 48 * elm_config_scale_get());
   evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(icon);
   elm_table_pack(table, icon, 0, 0, 1, 1);

   label = elm_label_add(popup);
   elm_object_text_set(label, message);
   evas_object_show(label);
   elm_table_pack(table, label, 1, 0, 1, 1);
   evas_object_show(table);

   box = elm_box_add(popup);
   sep = elm_separator_add(box);
   elm_separator_horizontal_set(sep, EINA_TRUE);
   evas_object_show(sep);
   elm_box_pack_end(box, sep);
   elm_box_pack_end(box, table);
   sep = elm_separator_add(box);
   elm_separator_horizontal_set(sep, EINA_TRUE);
   evas_object_show(sep);
   elm_box_pack_end(box, sep);

   elm_object_content_set(popup, box);

   button = elm_button_add(popup);
   elm_object_text_set(button, _("OK"));
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked", _edi_screens_popup_cancel_cb, popup);

   button = elm_button_add(popup);
   elm_object_text_set(button, _("Settings"));
   elm_object_part_content_set(popup, "button2", button);
   evas_object_data_set(button, "parent", parent);
   evas_object_smart_callback_add(button, "clicked", _edi_screens_settings_display_cb, popup);

   evas_object_show(popup);
}

void edi_screens_desktop_notify(const char *title, const char *message)
{
   Eina_Strbuf *command;

   if (!ecore_file_app_installed("notify-send"))
     return;

   command = eina_strbuf_new();

   eina_strbuf_append_printf(command, "notify-send -t 10000 -i edi '%s' '%s'", title, message);

   ecore_exe_run(eina_strbuf_string_get(command), NULL);

   eina_strbuf_free(command);
}

void
edi_screens_scm_binary_missing(Evas_Object *parent, const char *binary)
{
   Evas_Object *popup, *label, *button;
   Eina_Strbuf *text = eina_strbuf_new();

   eina_strbuf_append_printf(text, _("No %s binary found, please install %s."), binary, binary);

   popup = elm_popup_add(parent);
   elm_object_part_text_set(popup, "title,text", _("Unable to launch SCM binary"));
   label = elm_label_add(popup);
   elm_object_text_set(label, eina_strbuf_string_get(text));
   evas_object_show(label);
   elm_object_content_set(popup, label);

   eina_strbuf_free(text);

   button = elm_button_add(popup);
   elm_object_text_set(button, _("OK"));
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked", _edi_screens_popup_cancel_cb, popup);

   evas_object_show(popup);
}

