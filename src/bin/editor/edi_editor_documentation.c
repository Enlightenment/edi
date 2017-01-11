#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Elementary.h>
#include <Evas.h>

#include "edi_editor.h"

#include "language/edi_language_provider.h"

#include "edi_private.h"

static void
_edi_doc_font_set(Edi_Language_Document *doc, const char *font, int font_size)
{
   char *format = "<align=left><font=\'%s\'><font_size=%d>";
   char *font_head;
   char *font_tail = "</font_size></font></align>";
   int displen;

   displen = strlen(format) + strlen(font);
   font_head = malloc(sizeof(char) * displen);
   snprintf(font_head, displen, format, font, font_size);

   if (strlen(eina_strbuf_string_get(doc->title)) > 0)
     {
        eina_strbuf_prepend(doc->title, font_head);
        eina_strbuf_append(doc->title, font_tail);
     }

   if (strlen(eina_strbuf_string_get(doc->detail)) > 0)
     {
        eina_strbuf_prepend(doc->detail, font_head);
        eina_strbuf_append(doc->detail, font_tail);
     }

   if (strlen(eina_strbuf_string_get(doc->param)) > 0)
     {
        eina_strbuf_prepend(doc->param, font_head);
        eina_strbuf_append(doc->param, font_tail);
     }

   if (strlen(eina_strbuf_string_get(doc->ret)) > 0)
     {
        eina_strbuf_prepend(doc->ret, font_head);
        eina_strbuf_append(doc->ret, font_tail);
     }

   if (strlen(eina_strbuf_string_get(doc->see)) > 0)
     {
        eina_strbuf_prepend(doc->see, font_head);
        eina_strbuf_append(doc->see, font_tail);
     }

   free(font_head);
}

static void
_edi_doc_tag_name_set(Edi_Language_Document *doc)
{
   if (strlen(eina_strbuf_string_get(doc->param)) > 0)
     eina_strbuf_prepend(doc->param, "<b><br><br> Parameters<br></b>");

   if (strlen(eina_strbuf_string_get(doc->ret)) > 0)
     eina_strbuf_prepend(doc->ret, "<b><br><br> Returns<br></b>   ");

   if (strlen(eina_strbuf_string_get(doc->see)) > 0)
     eina_strbuf_prepend(doc->see, "<b><br><br> See also<br></b>");
}

static void
_edi_doc_popup_cb_block_clicked(void *data EINA_UNUSED, Evas_Object *obj,
                                void *event_info EINA_UNUSED)
{
   evas_object_del(obj);
}

static void
_edi_doc_popup_cb_btn_clicked(void *data, Evas_Object *obj EINA_UNUSED,
                              void *event_info EINA_UNUSED)
{
   evas_object_del(data);
}

static void
_edi_doc_popup_cb_key_down(void *data EINA_UNUSED, Evas *e EINA_UNUSED,
                           Evas_Object *obj, void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;

   if (!strcmp(ev->key, "Escape"))
     evas_object_del(obj);
}

static void
_edi_doc_popup_cb_del(void *data, Evas *e EINA_UNUSED,
                     Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Edi_Language_Document *doc = data;

   edi_language_doc_free(doc);
}

void
edi_editor_doc_open(Edi_Editor *editor)
{
   Edi_Language_Document *doc = NULL;
   const char *detail, *param, *ret, *see;
   char *display;
   int displen;
   Evas_Coord w, h;
   const char *font;
   int font_size;

   if (edi_language_provider_has(editor))
     {
        unsigned int row, col;

        elm_code_widget_cursor_position_get(editor->entry, &row, &col);
        doc = edi_language_provider_get(editor)->lookup_doc(editor, row, col);
     }

   //Popup
   editor->doc_popup = elm_popup_add(editor->entry);
   evas_object_smart_callback_add(editor->doc_popup, "block,clicked",
                                  _edi_doc_popup_cb_block_clicked, NULL);
   evas_object_event_callback_add(editor->doc_popup, EVAS_CALLBACK_KEY_DOWN,
                                  _edi_doc_popup_cb_key_down, NULL);
   evas_object_event_callback_add(editor->doc_popup, EVAS_CALLBACK_DEL,
                                  _edi_doc_popup_cb_del, doc);
   if (!doc)
     {
        elm_popup_timeout_set(editor->doc_popup, 1.5);
        elm_object_style_set(editor->doc_popup, "transparent");
        elm_object_text_set(editor->doc_popup, "No help available for this term");
        evas_object_show(editor->doc_popup);
        return;
     }

   _edi_doc_tag_name_set(doc);
   elm_code_widget_font_get(editor->entry, &font, &font_size);
   _edi_doc_font_set(doc, font, font_size);

   detail = eina_strbuf_string_get(doc->detail);
   param = eina_strbuf_string_get(doc->param);
   ret = eina_strbuf_string_get(doc->ret);
   see = eina_strbuf_string_get(doc->see);

   displen = strlen(detail) + strlen(param) + strlen(ret) + strlen(see);
   display = malloc(sizeof(char) * (displen + 1));
   snprintf(display, displen, "%s%s%s%s", detail, param, ret, see);

   //Close button
   Evas_Object *btn = elm_button_add(editor->doc_popup);
   elm_object_text_set(btn, "Close");
   evas_object_smart_callback_add(btn, "clicked", _edi_doc_popup_cb_btn_clicked,
                                  editor->doc_popup);
   elm_object_part_content_set(editor->doc_popup, "button1", btn);
   evas_object_show(btn);

   //Grid
   Evas_Object *grid = elm_grid_add(editor->doc_popup);
   elm_grid_size_set(grid, 100, 100);
   evas_object_size_hint_weight_set(grid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_geometry_get(editor->entry, NULL, NULL, &w, &h);
   evas_object_size_hint_min_set(grid, w * 0.8, h * 0.8);
   elm_object_content_set(editor->doc_popup, grid);
   evas_object_show(grid);

   //Background
   Evas_Object *bg = elm_bg_add(grid);
   elm_bg_color_set(bg, 53, 53, 53);
   elm_grid_pack(grid, bg, 1, 1, 98, 98);
   evas_object_show(bg);

   //Box
   Evas_Object *box = elm_box_add(grid);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_grid_pack(grid, box, 1, 1, 98, 98);
   evas_object_show(box);

   //Title
   Evas_Object *title = elm_entry_add(box);
   elm_entry_editable_set(title, EINA_FALSE);
   elm_object_text_set(title, eina_strbuf_string_get(doc->title));
   evas_object_size_hint_weight_set(title, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(title, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(title);
   elm_box_pack_end(box, title);

   //Entry
   Evas_Object *entry = elm_entry_add(box);
   elm_object_text_set(entry, display);
   elm_entry_editable_set(entry, EINA_FALSE);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   elm_scroller_policy_set(entry, ELM_SCROLLER_POLICY_OFF,
                           ELM_SCROLLER_POLICY_AUTO);
   evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(entry);
   elm_box_pack_end(box, entry);

   evas_object_show(editor->doc_popup);

   free(display);
}
