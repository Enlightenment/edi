#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/*
 * Search for text within an entry based on what is entered into a search dialog
 *
 * Based largely on code in Ecrire by Tom Hacohen <tom@stosb.com>
 */

#include <Elementary.h>
#include <Evas.h>

#include "edi_editor.h"
#include "edi_private.h"

/**
 * @struct _Edi_Editor_Search
 * An instance of an editor view search session.
 */
struct _Edi_Editor_Search
{
   Evas_Object *entry; /**< The search text widget */
   Evas_Object *widget; /**< The search UI panel we wish to show and hide */
   Evas_Object *parent; /**< The parent panel we will insert into */
   Evas_Textblock_Cursor *current_search; /**< The current search cursor for this session */

   /* Add new members here. */
};

static Eina_Bool
_search_in_entry(Evas_Object *entry, Edi_Editor_Search *search)
{
   Eina_Bool try_next = EINA_FALSE;
   const char *found, *text;
   char *utf8;
   const Evas_Object *tb = elm_entry_textblock_get(entry);
   Evas_Textblock_Cursor *end, *start, *mcur;
   size_t initial_pos;
   Evas_Coord x, y, w, h;

   text = elm_object_text_get(search->entry);
   if (!text || !*text)
      return EINA_FALSE;

   mcur = (Evas_Textblock_Cursor *) evas_object_textblock_cursor_get(tb);
   if (!search->current_search)
     {
        search->current_search = evas_object_textblock_cursor_new(tb);
     }
   else if (!evas_textblock_cursor_compare(search->current_search, mcur))
     {
        try_next = EINA_TRUE;
     }

   evas_textblock_cursor_paragraph_last(search->current_search);
   start = mcur;
   end = search->current_search;

   initial_pos = evas_textblock_cursor_pos_get(start);

   utf8 = evas_textblock_cursor_range_text_get(start, end,
         EVAS_TEXTBLOCK_TEXT_PLAIN);

   if (!utf8)
      return EINA_FALSE;

   if (try_next)
     {
        found = strstr(utf8 + 1, text);
        if (!found)
          {
             found = utf8;
          }
     }
   else
     {
        found = strstr(utf8, text);
     }

   elm_entry_select_none(entry);
   if (found)
     {
        size_t pos = 0;
        int idx = 0;
        while ((utf8 + idx) < found)
          {
             pos++;
             eina_unicode_utf8_next_get(utf8, &idx);
          }

        evas_textblock_cursor_pos_set(mcur, pos + initial_pos + strlen(text));
        elm_entry_cursor_geometry_get(entry, &w, &h, NULL, NULL);
        elm_entry_cursor_selection_begin(entry);
        elm_entry_cursor_pos_set(entry, pos + initial_pos);
        elm_entry_cursor_selection_end(entry);

        elm_entry_cursor_geometry_get(entry, &x, &y, NULL, NULL);
        w -= x;
        h -= y;
        elm_scroller_region_show(entry, x, y, w, h);
        evas_textblock_cursor_copy(mcur, search->current_search);
     }

   free(utf8);

   return !!found;
}

static void
_edi_editor_search_hide(Edi_Editor *editor)
{
   Edi_Editor_Search *search;

   search = editor->search;
   if (!search)
     return;

   if (eina_list_data_find(elm_box_children_get(search->parent), search->widget))
     {
        evas_object_hide(search->widget);
        elm_box_unpack(search->parent, search->widget);
     }

   if (search->current_search)
     evas_textblock_cursor_free(search->current_search);
   search->current_search = NULL;
   elm_entry_select_none(editor->entry);
}

EAPI void
edi_editor_search(Edi_Editor *editor)
{
   Edi_Editor_Search *search;

   search = editor->search;
   if (search && !eina_list_data_find(elm_box_children_get(search->parent), search->widget))
     {
        evas_object_show(search->widget);
        elm_box_pack_end(search->parent, search->widget);
     }

   elm_object_focus_set(search->entry, EINA_TRUE);
}

static void
_search_clicked(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Edi_Editor *editor;
   Edi_Editor_Search *search;

   editor = (Edi_Editor *)data;
   search = editor->search;

   if (search)
      _search_in_entry(editor->entry, search);
}

static void
_cancel_clicked(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _edi_editor_search_hide((Edi_Editor *)data);
}

static void
_search_key_up_cb(void *data , Evas *e EINA_UNUSED, Evas_Object *obj,
                  void *event_info)
{
   Edi_Editor *editor;
   Edi_Editor_Search *search;
   Evas_Event_Key_Up *ev = (Evas_Event_Key_Up *)event_info;
   const char *str;

   editor = (Edi_Editor *)data;
   search = editor->search;

   str = elm_object_text_get(obj);

   if (strlen(str) && (!strcmp(ev->key, "KP_Enter") || !strcmp(ev->key, "Return")))
     _search_clicked(data, NULL, NULL);
   else if (!strcmp(ev->key, "Escape"))
     _cancel_clicked(data, NULL, NULL);
   else
     {
        if (search->current_search)
          evas_textblock_cursor_free(search->current_search);
        search->current_search = NULL;
     }
}


EAPI void
edi_editor_search_add(Evas_Object *parent, Edi_Editor *editor)
{
   Evas_Object *entry, *lbl, *btn, *box;
   Edi_Editor_Search *search;

   box = elm_box_add(parent);
   elm_box_horizontal_set(box, EINA_TRUE);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(box, 0.0, 0.0);
   elm_box_padding_set(box, 15, 0);

   lbl = elm_label_add(box);
   elm_object_text_set(lbl, "Search term:");
   evas_object_size_hint_align_set(lbl, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(lbl, 0.0, 0.0);
   elm_box_pack_end(box, lbl);
   evas_object_show(lbl);

   entry = elm_entry_add(box);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   elm_entry_single_line_set(entry, EINA_TRUE);
   evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, 0.0);
   evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(box, entry);
   evas_object_show(entry);

   evas_object_event_callback_add(entry, EVAS_CALLBACK_KEY_UP, _search_key_up_cb, editor);

   btn = elm_button_add(box);
   elm_object_text_set(btn, "Search");
   evas_object_size_hint_align_set(btn, 1.0, 0.0);
   evas_object_size_hint_weight_set(btn, 0.0, 0.0);
   evas_object_show(btn);
   elm_box_pack_end(box, btn);
   evas_object_smart_callback_add(btn, "clicked", _search_clicked, editor);

   btn = elm_button_add(box);
   elm_object_text_set(btn, "Cancel");
   evas_object_size_hint_align_set(btn, 1.0, 0.0);
   evas_object_size_hint_weight_set(btn, 0.0, 0.0);
   evas_object_show(btn);
   elm_box_pack_end(box, btn);
   evas_object_smart_callback_add(btn, "clicked", _cancel_clicked, editor);

   search = calloc(1, sizeof(*search));
   search->entry = entry;
   search->parent = parent;
   search->widget = box;
   editor->search = search;
   evas_object_show(parent);
}
