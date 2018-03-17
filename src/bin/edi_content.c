#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "editor/edi_editor.h"
#include "edi_content.h"

#include "edi_config.h"
#include "edi_private.h"

static Eina_Bool
_edi_content_diff_config_changed(void *data, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   Evas_Object *diff;

   diff = (Evas_Object*) data;
   elm_code_diff_widget_font_set(diff, _edi_project_config->font.name, _edi_project_config->font.size);

   return ECORE_CALLBACK_RENEW;
}

Evas_Object *
edi_content_diff_add(Evas_Object *parent, Edi_Mainview_Item *item)
{
   Elm_Code *code;
   Evas_Object *diff;

   code = elm_code_create();
   elm_code_file_open(code, item->path);
   diff = elm_code_diff_widget_add(parent, code);
   elm_code_diff_widget_font_set(diff, _edi_project_config->font.name, _edi_project_config->font.size);

   ecore_event_handler_add(EDI_EVENT_CONFIG_CHANGED, _edi_content_diff_config_changed, diff);

   return diff;
}

Evas_Object *
edi_content_image_add(Evas_Object *parent, Edi_Mainview_Item *item)
{
   Evas_Object *vbox, *box, *searchbar, *statusbar, *scroll, *img;
   Edi_Editor *editor;

   vbox = elm_box_add(parent);
   evas_object_size_hint_weight_set(vbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(vbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(vbox);

   searchbar = elm_box_add(vbox);
   evas_object_size_hint_weight_set(searchbar, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(searchbar, EVAS_HINT_FILL, 0.0);
   elm_box_pack_end(vbox, searchbar);

   box = elm_box_add(vbox);
   elm_box_horizontal_set(box, EINA_TRUE);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(vbox, box);
   evas_object_show(box);

   statusbar = elm_box_add(vbox);
   evas_object_size_hint_weight_set(statusbar, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(statusbar, EVAS_HINT_FILL, 0.0);
   elm_box_pack_end(vbox, statusbar);
   evas_object_show(statusbar);

   scroll = elm_scroller_add(parent);
   evas_object_size_hint_weight_set(scroll, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(scroll, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(scroll);
   img = elm_image_add(vbox);
   elm_image_file_set(img, item->path, NULL);
   elm_image_no_scale_set(img, EINA_TRUE);
   elm_object_content_set(scroll, img);
   evas_object_show(img);

   elm_box_pack_end(box, scroll);
   editor = calloc(1, sizeof(*editor));
   editor->mimetype = item->mimetype;

   edi_editor_statusbar_add(statusbar, editor, item);

   return vbox;
}

