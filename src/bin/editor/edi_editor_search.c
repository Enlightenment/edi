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

static Evas_Object *_search_win, *_search_entry, *_target_entry;
static Evas_Textblock_Cursor *_current_search;

static Eina_Bool
_search_in_entry(Evas_Object *entry, const char *text)
{
   Eina_Bool try_next = EINA_FALSE;
   const char *found;
   char *utf8;
   const Evas_Object *tb = elm_entry_textblock_get(entry);
   Evas_Textblock_Cursor *end, *start, *mcur;
   size_t initial_pos;

   if (!text || !*text)
      return EINA_FALSE;

   mcur = (Evas_Textblock_Cursor *) evas_object_textblock_cursor_get(tb);
   if (!_current_search)
     {
        _current_search = evas_object_textblock_cursor_new(tb);
     }
   else if (!evas_textblock_cursor_compare(_current_search, mcur))
     {
        try_next = EINA_TRUE;
     }

   evas_textblock_cursor_paragraph_last(_current_search);
   start = mcur;
   end = _current_search;

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

   if (found)
     {
        size_t pos = 0;
        int idx = 0;
        while ((utf8 + idx) < found)
          {
             pos++;
             eina_unicode_utf8_next_get(utf8, &idx);
          }

        elm_entry_select_none(entry);
        evas_textblock_cursor_pos_set(mcur, pos + initial_pos + strlen(text));
        elm_entry_cursor_selection_begin(entry);
        elm_entry_cursor_pos_set(entry, pos + initial_pos);
        elm_entry_cursor_selection_end(entry);
        evas_textblock_cursor_copy(mcur, _current_search);
     }

   free(utf8);

   return !!found;
}

static void
_search_clicked(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _search_in_entry(_target_entry, elm_object_text_get(_search_entry));
}

static void
_search_win_del(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   if (_current_search)
     {
        evas_textblock_cursor_free(_current_search);
        _current_search = NULL;
     }

   _search_win = NULL;
}

EAPI void
edi_editor_search(Evas_Object *entry)
{
   Evas_Object *bg, *bx, *lbl, *hbx, *btn;

   _target_entry = entry;
   if (_search_win)
     {
        evas_object_show(_search_win);
        return;
     }

   _search_win = elm_win_util_standard_add("search", "Search");
   elm_win_autodel_set(_search_win, EINA_TRUE);
   elm_win_title_set(_search_win, "Search & Replace");
   evas_object_smart_callback_add(_search_win, "delete,request", _search_win_del, entry);

   bg = elm_bg_add(_search_win);
   elm_win_resize_object_add(_search_win, bg);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   bx = elm_box_add(_search_win);
   elm_win_resize_object_add(_search_win, bx);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bx);

   hbx = elm_box_add(_search_win);
   elm_box_padding_set(hbx, 15, 0);
   elm_box_horizontal_set(hbx, EINA_TRUE);
   evas_object_size_hint_align_set(hbx, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(hbx, EVAS_HINT_EXPAND, 0.0);
   evas_object_show(hbx);
   elm_box_pack_end(bx, hbx);

   lbl = elm_label_add(_search_win);
   elm_object_text_set(lbl, "Search term:");
   evas_object_size_hint_align_set(lbl, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(lbl, 0.0, 0.0);
   elm_box_pack_end(hbx, lbl);
   evas_object_show(lbl);

   _search_entry = elm_entry_add(_search_win);
   elm_entry_scrollable_set(_search_entry, EINA_TRUE);
   elm_entry_single_line_set(_search_entry, EINA_TRUE);
   evas_object_size_hint_align_set(_search_entry, EVAS_HINT_FILL, 0.0);
   evas_object_size_hint_weight_set(_search_entry, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(hbx, _search_entry);
   evas_object_show(_search_entry);

   hbx = elm_box_add(_search_win);
   elm_box_homogeneous_set(hbx, EINA_FALSE);
   elm_box_horizontal_set(hbx, EINA_TRUE);
   evas_object_size_hint_align_set(hbx, 1.0, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(hbx, 0.0, 0.0);
   evas_object_show(hbx);
   elm_box_pack_end(bx, hbx);

   btn = elm_button_add(_search_win);
   elm_object_text_set(btn, "Search");
   evas_object_size_hint_align_set(btn, 1.0, 0.0);
   evas_object_size_hint_weight_set(btn, 0.0, 0.0);
   evas_object_show(btn);
   elm_box_pack_end(hbx, btn);
   evas_object_smart_callback_add(btn, "clicked", _search_clicked, entry);

   evas_object_resize(_search_win, 300, 50);
   evas_object_show(_search_win);

   _current_search = NULL;
}

