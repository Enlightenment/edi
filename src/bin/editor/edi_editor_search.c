#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/*
 * Search for, and possibly replace, text within an entry based on what is entered into
 * the search and replace entry boxes.
 *
 * Based largely on code in Ecrire by Tom Hacohen <tom@stosb.com>
 * Replace added by Kelly Wilson
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
   unsigned int current_search_line; /**< The current search cursor line for this session */
   unsigned int current_search_col; /**< The current search cursor column for this session */

   Eina_Bool term_found;
   Evas_Object *replace_entry; /**< The replace text widget */
   Evas_Object *replace_btn; /**< The replace button for our search */

   /* Add new members here. */
};

static Eina_Bool
_edi_search_in_entry(Evas_Object *entry, Edi_Editor_Search *search)
{
   Eina_Bool try_next = EINA_FALSE;
   Eina_List *item;
   Elm_Code *code;
   Elm_Code_Line *line;
   const char *text;
   unsigned int offset, pos_line, pos_col;
   int found;

   text = elm_object_text_get(search->entry);
   if (!text || !*text)
     {
        search->term_found = EINA_FALSE;
        return EINA_FALSE;
     }

   code = elm_code_widget_code_get(entry);
   elm_code_widget_cursor_position_get(entry, &pos_line, &pos_col);
   if (search->current_search_line == pos_line &&
       search->current_search_col == pos_col)
     {
        try_next = EINA_TRUE;
     }

   found = 0;
   EINA_LIST_FOREACH(code->file->lines, item, line)
     {
        if (line->number < pos_line)
          continue;

        offset = 0;
        if (line->number == pos_line)
          offset = elm_code_widget_line_text_position_for_column_get(entry, line, pos_col) + (try_next ? 1 : 0);

        found = elm_code_line_text_strpos(line, text, offset);
        if (found == ELM_CODE_TEXT_NOT_FOUND)
          continue;

        search->current_search_line = line->number;
        search->current_search_col =
          elm_code_widget_line_text_column_width_to_position(entry, line, found);
        break;
     }

   search->term_found = found != ELM_CODE_TEXT_NOT_FOUND;
   elm_code_widget_selection_clear(entry);
   if (!search->term_found)
     return EINA_FALSE;

   elm_code_widget_selection_start(entry, search->current_search_line,
                                        search->current_search_col);
   elm_code_widget_selection_end(entry, search->current_search_line,
                                 elm_code_widget_line_text_column_width_to_position(entry, line, found + strlen(text)) - 1);

   return EINA_TRUE;
}

static void
_edi_replace_entry_changed(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Edi_Editor *editor;
   Edi_Editor_Search *replace;

   editor = (Edi_Editor *)data;
   replace = editor->search;

   if (!elm_entry_is_empty(replace->replace_entry))
     elm_object_disabled_set(replace->replace_btn, EINA_FALSE);
   else
     elm_object_disabled_set(replace->replace_btn, EINA_TRUE);

   replace->current_search_line = 0;
   replace->term_found = EINA_FALSE;
}

static void
_edi_replace_in_entry(void *data, Edi_Editor_Search *search)
{
   Edi_Editor *editor;
   Elm_Code *code;
   Elm_Code_Line *line;
   const char *replace;

   editor = (Edi_Editor *)data;
   // If there is no search term found to replace, then do a new search first.
   if (search && !search->term_found)
     _edi_search_in_entry(editor->entry, search);

   // If we found a matching searched term the go ahead and replace it if appropriate.
   if (search->term_found)
     {
        if (search->current_search_line)
          {
             code = elm_code_widget_code_get(editor->entry);

             elm_code_widget_selection_delete(editor->entry);
             replace = elm_object_text_get(search->replace_entry);

             line = elm_code_file_line_get(code->file, search->current_search_line);
             elm_code_line_text_insert(line, search->current_search_col, replace, strlen(replace));
             search->current_search_line = 0;
          }
        _edi_search_in_entry(editor->entry, search);
     }

   return;
}

static void
_edi_editor_search_hide(Edi_Editor *editor)
{
   Edi_Editor_Search *search;

   search = editor->search;
   if (!search)
     return;

   search->term_found = EINA_FALSE;
   if (eina_list_data_find(elm_box_children_get(search->parent), search->widget))
     {
        evas_object_hide(search->widget);
        elm_box_unpack(search->parent, search->widget);
     }

   search->current_search_line = 0;
   elm_code_widget_selection_clear(editor->entry);

   // Re-focus in the editor proper since we are closing the search bar here
   elm_object_focus_set(editor->entry, EINA_TRUE);
}

void
edi_editor_search(Edi_Editor *editor)
{
   Edi_Editor_Search *search;

   search = editor->search;
   search->term_found = EINA_FALSE;
   if (search && !eina_list_data_find(elm_box_children_get(search->parent), search->widget))
     {
        evas_object_show(search->widget);
        elm_box_pack_end(search->parent, search->widget);

        // Check if there is already data in the replace entry. Enable the replace
        // button as appropriate
        if (!elm_entry_is_empty(search->replace_entry))
          elm_object_disabled_set(search->replace_btn, EINA_FALSE);
        else
          elm_object_disabled_set(search->replace_btn, EINA_TRUE);
     }

   elm_object_text_set(search->entry, "");
   elm_object_focus_set(search->entry, EINA_TRUE);
}

static void
_edi_search_clicked(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Edi_Editor *editor;
   Edi_Editor_Search *search;

   editor = (Edi_Editor *)data;
   search = editor->search;

   if (search)
     _edi_search_in_entry(editor->entry, search);
}

static void
_edi_replace_clicked(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Edi_Editor *editor;
   Edi_Editor_Search *replace;

   editor = (Edi_Editor *)data;
   replace = editor->search;

   if (replace)
     _edi_replace_in_entry(data, replace);
}

static void
_edi_cancel_clicked(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _edi_editor_search_hide((Edi_Editor *)data);
}

static void
_edi_search_key_up_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
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
     _edi_search_clicked(data, NULL, NULL);
   else if (!strcmp(ev->key, "Escape"))
     _edi_cancel_clicked(data, NULL, NULL);
   else
     {
        search->current_search_line = 0;
        search->term_found = EINA_FALSE;
     }
}

void
edi_editor_search_add(Evas_Object *parent, Edi_Editor *editor)
{
   Evas_Object *entry, *lbl, *btn, *box, *big_box;
   Evas_Object *replace_entry, *replace_lbl, *replace_btn, *replace_box;
   Edi_Editor_Search *search;

   big_box = elm_box_add(parent);
   evas_object_size_hint_align_set(big_box, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(big_box, 0.0, 0.0);

   box = elm_box_add(parent);
   elm_box_homogeneous_set(box, EINA_FALSE);
   elm_box_padding_set(box, 15, 0);
   elm_box_horizontal_set(box, EINA_TRUE);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, 0.0);
   evas_object_show(box);
   elm_box_pack_end(big_box, box);

   lbl = elm_label_add(parent);
   elm_object_text_set(lbl, "Search  term:");
   evas_object_size_hint_align_set(lbl, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(lbl, 0.0, 0.0);
   elm_box_pack_end(box, lbl);
   evas_object_show(lbl);

   entry = elm_entry_add(parent);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   elm_entry_single_line_set(entry, EINA_TRUE);
   evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, 0.0);
   evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(box, entry);
   evas_object_show(entry);

   replace_box = elm_box_add(parent);
   elm_box_padding_set(replace_box, 15, 0);
   elm_box_horizontal_set(replace_box, EINA_TRUE);
   evas_object_size_hint_align_set(replace_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(replace_box, EVAS_HINT_EXPAND, 0.0);
   evas_object_show(replace_box);
   elm_box_pack_end(big_box, replace_box);

   replace_lbl = elm_label_add(parent);
   elm_object_text_set(replace_lbl, "Replace term:");
   evas_object_size_hint_align_set(replace_lbl, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(replace_lbl, 0.0, 0.0);
   elm_box_pack_end(replace_box, replace_lbl);
   evas_object_show(replace_lbl);

   replace_entry = elm_entry_add(parent);
   elm_entry_scrollable_set(replace_entry, EINA_TRUE);
   elm_entry_single_line_set(replace_entry, EINA_TRUE);
   evas_object_size_hint_align_set(replace_entry, EVAS_HINT_FILL, 0.0);
   evas_object_size_hint_weight_set(replace_entry, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(replace_box, replace_entry);
   evas_object_show(replace_entry);
   evas_object_smart_callback_add(replace_entry, "changed", _edi_replace_entry_changed, editor);

   box = elm_box_add(parent);
   elm_box_homogeneous_set(box, EINA_FALSE);
   elm_box_padding_set(box, 15, 0);
   elm_box_horizontal_set(box, EINA_TRUE);
   evas_object_size_hint_align_set(box, 1.0, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(box, 0.0, 0.0);
   evas_object_show(box);
   elm_box_pack_end(big_box, box);

   evas_object_event_callback_add(entry, EVAS_CALLBACK_KEY_UP, _edi_search_key_up_cb, editor);

   btn = elm_button_add(parent);
   elm_object_text_set(btn, "Search");
   evas_object_size_hint_align_set(btn, 1.0, 0.0);
   evas_object_size_hint_weight_set(btn, 0.0, 0.0);
   evas_object_show(btn);
   elm_box_pack_end(box, btn);
   evas_object_smart_callback_add(btn, "clicked", _edi_search_clicked, editor);

   replace_btn = elm_button_add(parent);
   elm_object_text_set(replace_btn, "Replace");
   evas_object_size_hint_align_set(replace_btn, 1.0, 0.0);
   evas_object_size_hint_weight_set(replace_btn, 0.0, 0.0);
   evas_object_show(replace_btn);
   elm_box_pack_end(box, replace_btn);
   evas_object_smart_callback_add(replace_btn, "clicked", _edi_replace_clicked, editor);

   btn = elm_button_add(parent);
   elm_object_text_set(btn, "Cancel");
   evas_object_size_hint_align_set(btn, 1.0, 0.0);
   evas_object_size_hint_weight_set(btn, 0.0, 0.0);
   evas_object_show(btn);
   elm_box_pack_end(box, btn);
   evas_object_smart_callback_add(btn, "clicked", _edi_cancel_clicked, editor);

   search = calloc(1, sizeof(*search));
   search->entry = entry;
   search->replace_entry = replace_entry;
   search->replace_btn = replace_btn;
   search->parent = parent;
   search->widget = big_box;
   editor->search = search;
   evas_object_show(parent);
}
