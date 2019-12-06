#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "language/edi_language_provider.h"
#include "editor/edi_editor.h"
#include "edi_content.h"
#include "mainview/edi_mainview.h"

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

   edi_content_statusbar_add(statusbar, item);
   edi_content_statusbar_position_set(item->pos, 0, 0);

   return vbox;
}

void
edi_content_statusbar_position_set(Evas_Object *position, unsigned int line, unsigned int pos)
{
   char buf[64];
   char text[128];
   int i;

   if (!position) return;

   if (line && pos)
     {
        snprintf(buf, sizeof(buf), _("Line: %d, Position: %d"), line, pos);
     }
   else
     {
        buf[0] = 0x00;
     }

   text[0] = 0x00;

   for (i = strlen(buf); i < 24; i++)
      {
         strcat(text, " ");
      }

   strcat(text, buf);

   elm_object_text_set(position, text);
}

void
edi_content_statusbar_add(Evas_Object *panel, Edi_Mainview_Item *item)
{
   Edi_Language_Provider *provider;
   Evas_Object *table, *rect, *tb, *position, *mime;
   Elm_Code *code;
   char text[256];
   const char *format = "", *spaces = "        ";
   const char *mimename = NULL;

   elm_box_horizontal_set(panel, EINA_TRUE);

   table = elm_table_add(panel);
   evas_object_size_hint_align_set(table, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(table);
   elm_box_pack_end(panel, table);

   if (!strcmp(item->editortype, "code"))
     {
        code = elm_code_create();
        code->file = elm_code_file_open(code, item->path);

        if (elm_code_file_line_ending_get(code->file) == ELM_CODE_FILE_LINE_ENDING_WINDOWS)
          format = "WIN";
        else
          format = "UNIX";

        elm_code_free(code);
     }

   mime = elm_entry_add(panel);
   elm_entry_editable_set(mime, EINA_FALSE);
   elm_entry_scrollable_set(mime, EINA_FALSE);
   elm_entry_single_line_set(mime, EINA_TRUE);

   if (item->mimetype)
     {
        provider = edi_language_provider_for_mime_get(item->mimetype);
        if (provider)
          mimename = provider->mime_name(item->mimetype);

        if (mimename)
          snprintf(text, sizeof(text), "%s (%s)%s%s", mimename, item->mimetype, spaces, format);
        else
          snprintf(text, sizeof(text), "%s%s%s", item->mimetype, spaces, format);
     }
   else
     {
        snprintf(text, sizeof(text), "%s%s%s", item->editortype, spaces, format);
     }

   elm_object_text_set(mime, text);

   rect = evas_object_rectangle_add(panel);
   evas_object_size_hint_weight_set(rect, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(rect, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_move(rect, 0, 0);
   evas_object_resize(rect, 220, 1);
   evas_object_color_set(rect, 0, 0, 0, 0);
   evas_object_show(rect);

   tb = elm_entry_textblock_get(mime);
   evas_object_size_hint_weight_set(tb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_resize(tb, 220, 1);
   evas_object_show(tb);
   elm_table_pack(table, tb, 0, 0, 1, 1);
   elm_table_pack(table, rect, 0, 0, 1, 1);

   item->pos = position = elm_entry_add(panel);
   elm_entry_single_line_set(position, EINA_TRUE);
   elm_entry_editable_set(position, EINA_FALSE);
   evas_object_size_hint_align_set(position, 1.0, 0.5);
   evas_object_size_hint_weight_set(position, EVAS_HINT_EXPAND, 0.0);
   elm_table_pack(table, position, 1, 0, 1, 1);
   evas_object_show(position);
}

