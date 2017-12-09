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
 * @struct _Edi_Search_Result
 * An instance of a single search.
 */
struct _Edi_Search_Result
{
   int found;
   Elm_Code_Line *line;
   unsigned int line_number;
   unsigned int col;
   char *text;
   Eina_Bool success;
};

/**
 * @struct _Edi_Editor_Search
 * An instance of an editor view search session.
 */
struct _Edi_Editor_Search
{
   Evas_Object *entry; /**< The search text widget */
   Evas_Object *widget; /**< The search UI panel we wish to show and hide */
   Evas_Object *parent; /**< The parent panel we will insert into */
   Evas_Object *checkbox; /**< The checkbox for wrapping search */
   Evas_Object *wrapped_text; /**< A display that shows the user that the search wrapped */
   unsigned int current_search_line; /**< The current search cursor line for this session */
   unsigned int current_search_col; /**< The current search cursor column for this session */
   Eina_Bool term_found;
   Eina_Bool wrap;
   Evas_Object *replace_entry; /**< The replace text widget */
   Evas_Object *replace_btn; /**< The replace button for our search */
   struct _Edi_Search_Result cache; /**< The first found search instance */
   /* Add new members here. */
   Eina_Bool wrapped;
};

static void
_edi_search_cache_reset(Edi_Editor_Search *search)
{
   if (search->cache.text)
     {
        free(search->cache.text);
        search->cache.text = NULL;
     }
   search->cache.found = ELM_CODE_TEXT_NOT_FOUND;
   search->cache.success = EINA_FALSE;
}

static Eina_Bool
_edi_search_cache_exists(Edi_Editor_Search *search)
{
   return search->cache.success;
}

static void
_edi_search_cache_store(Edi_Editor_Search *search, int found, const char *text, Elm_Code_Line *line, unsigned int line_col)
{
   search->cache.found = found;
   search->cache.text = strdup(text);
   search->cache.line = line;
   search->cache.line_number = line->number;
   search->cache.col = line_col;
   search->cache.success = EINA_TRUE;
}

Eina_Bool
_edi_search_term_changed(Edi_Editor_Search *search, const char *text)
{
   if (!strcmp(search->cache.text, text))
     return EINA_FALSE;

   return EINA_TRUE;
}

static void
_edi_search_cache_use(Edi_Editor_Search *search, char **text, Elm_Code_Line **line, int *found)
{
   *text = search->cache.text;
   *line = search->cache.line;
   *found = search->cache.found;
   search->current_search_line = search->cache.line_number;
   search->current_search_col = search->cache.col;
}


static Eina_List *
_edi_search_clear_highlights(Eina_List *tokens)
{
   Elm_Code_Token *token;
   Eina_List *ret, *item, *item_next;

   ret = tokens;

   EINA_LIST_FOREACH_SAFE(tokens, item, item_next, token)
     {
        if (token->type == ELM_CODE_TOKEN_TYPE_MATCH)
          ret = eina_list_remove(ret, token);
     }

   return ret;
}

static void
_edi_search_show_highlights(Elm_Code_Line *line, const char *text)
{
   int match;

   match = elm_code_line_text_strpos(line, text, 0);
   while (match != ELM_CODE_TEXT_NOT_FOUND)
     {
        elm_code_line_token_add(line, match, match + strlen(text) - 1, 1, ELM_CODE_TOKEN_TYPE_MATCH);

        match = elm_code_line_text_strpos(line, text, match + 1);
     }
}

static Eina_Bool
_edi_search_in_entry(Evas_Object *entry, Edi_Editor_Search *search)
{
   Eina_Bool try_next = EINA_FALSE;
   Eina_List *item;
   Elm_Code *code;
   Elm_Code_Line *line;
   const char *text_markup;
   char *text;
   unsigned int offset, pos, pos_line, pos_col;
   int found, match;
   search->wrap = elm_check_state_get(search->checkbox);

   text_markup = elm_object_text_get(search->entry);
   if (!text_markup || !text_markup[0])
     {
        search->term_found = EINA_FALSE;
        return EINA_FALSE;
     }

   text = elm_entry_markup_to_utf8(text_markup);

   code = elm_code_widget_code_get(entry);
   elm_code_widget_cursor_position_get(entry, &pos_line, &pos_col);
   if (search->current_search_line == pos_line &&
       search->current_search_col == pos_col)
     {
        try_next = EINA_TRUE;
     }

   if (_edi_search_cache_exists(search) &&
       _edi_search_term_changed(search, text))
     {
        _edi_search_cache_reset(search);
        line = elm_code_file_line_get(elm_code_widget_code_get(entry)->file, 1);
        elm_code_widget_cursor_position_set(entry, 1, 1);
        _edi_search_cache_store(search, 0, text, line, 1);
        _edi_search_in_entry(entry, search);
        return EINA_TRUE;
     }

   found = ELM_CODE_TEXT_NOT_FOUND;
   EINA_LIST_FOREACH(code->file->lines, item, line)
     {
        line->tokens = _edi_search_clear_highlights(line->tokens);
        _edi_search_show_highlights(line, text);

        offset = 0;
        match = elm_code_line_text_strpos(line, text, offset);
        if (match == ELM_CODE_TEXT_NOT_FOUND)
          continue;

        pos = elm_code_widget_line_text_column_width_to_position(entry, line, match);
        if (!_edi_search_cache_exists(search))
          _edi_search_cache_store(search, match, text, line, pos);

        if (line->number < pos_line)
          continue;

        if (line->number == pos_line)
          {
             offset = elm_code_widget_line_text_position_for_column_get(entry, line, pos_col) + (try_next ? 1 : 0);

             match = elm_code_line_text_strpos(line, text, offset);
             if (match == ELM_CODE_TEXT_NOT_FOUND)
               continue;

             pos = elm_code_widget_line_text_column_width_to_position(entry, line, match);
          }

        if (found == ELM_CODE_TEXT_NOT_FOUND)
          {
             // store first occurence of search from cursor position
             search->current_search_line = line->number;
             search->current_search_col = pos;

             found = match;
          }
     }

   search->term_found = found != ELM_CODE_TEXT_NOT_FOUND;
   elm_code_widget_selection_clear(entry);

   // nothing found and wrap is disabled
   if (!search->term_found && !search->wrap)
     return EINA_FALSE;

   // No history nor term found so return
   if (search->wrap && !search->term_found && !_edi_search_cache_exists(search))
     return EINA_FALSE;

   // EOF reached so go to search found before cursor (first inst).
   if (search->wrap && !search->term_found && _edi_search_cache_exists(search))
     {
        elm_code_widget_cursor_position_set(entry, 0, 0);
        elm_code_widget_selection_clear(entry);
        line = elm_code_file_line_get(elm_code_widget_code_get(entry)->file, 1);
        _edi_search_cache_reset(search);
        _edi_search_cache_use(search, &text, &line, &found);
        search->wrapped = EINA_TRUE;
        _edi_search_in_entry(entry, search);
        free(text);
        return EINA_TRUE;
     }
   else
     {
       if (search->wrapped)
         {
            evas_object_show(search->wrapped_text);
            search->wrapped = EINA_FALSE;
         }
       else
         {
            evas_object_hide(search->wrapped_text);
         }
     }

   line = elm_code_file_line_get(elm_code_widget_code_get(entry)->file, search->current_search_line);
   elm_code_widget_cursor_position_set(entry, search->current_search_line,
                                              search->current_search_col);
   elm_code_widget_selection_start(entry, search->current_search_line,
                                        search->current_search_col);
   elm_code_widget_selection_end(entry, search->current_search_line,
                                 elm_code_widget_line_text_column_width_to_position(entry, line, found + strlen(text)) - 1);

   free(text);

   return EINA_TRUE;
}

static void
_edi_replace_entry_changed(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Edi_Editor *editor;
   Edi_Editor_Search *search;
   const char *text;

   editor = (Edi_Editor *)data;
   search = editor->search;

   text = elm_object_text_get(search->replace_entry);
   if (!text || !text[0])
     elm_object_disabled_set(search->replace_btn, EINA_TRUE);
   else
     elm_object_disabled_set(search->replace_btn, EINA_FALSE);

   search->current_search_line = 0;
   search->current_search_col = 0;
   search->term_found = EINA_FALSE;
}

static void
_edi_replace_in_entry(void *data, Edi_Editor_Search *search)
{
   Edi_Editor *editor;
   const char *text_markup;
   unsigned int line, col;
   char *text = NULL;

   editor = (Edi_Editor *)data;

   if (!_edi_search_in_entry(editor->entry, search)) return;

   if (!search->term_found)
     return;

   text_markup = elm_object_text_get(search->replace_entry);
   if (text_markup && text_markup[0])
     {
        elm_code_widget_cursor_position_get(editor->entry, &line, &col);
        text = elm_entry_markup_to_utf8(text_markup);
        if ((text[0]) && search->current_search_line == line && search->current_search_col == col)
          {
             elm_code_widget_selection_delete(editor->entry);
             elm_code_widget_text_at_cursor_insert(editor->entry, text);
          }
     }

  if (text)
     free(text);

   return;
}

static void
_edi_editor_search_hide(Edi_Editor *editor)
{
   Edi_Editor_Search *search;
   Elm_Code *code;
   Elm_Code_Line *line;
   Eina_List *item;

   search = editor->search;
   if (!search)
     return;

   search->term_found = EINA_FALSE;
   if (eina_list_data_find(elm_box_children_get(search->parent), search->widget))
     {
        evas_object_hide(search->widget);
        elm_box_unpack(search->parent, search->widget);
     }

   code = elm_code_widget_code_get(editor->entry);
   EINA_LIST_FOREACH(code->file->lines, item, line)
     {
        line->tokens = _edi_search_clear_highlights(line->tokens);
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
   Evas_Object *entry, *wrapped_text, *lbl, *btn, *box, *big_box, *table;
   Evas_Object *replace_entry, *replace_lbl, *replace_btn;
   Evas_Object *checkbox;
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

   table = elm_table_add(parent);
   evas_object_size_hint_align_set(table, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_table_padding_set(table, 5, 0);
   evas_object_show(table);
   elm_box_pack_end(box, table);

   lbl = elm_label_add(parent);
   elm_object_text_set(lbl, _("Search term"));
   evas_object_size_hint_align_set(lbl, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(lbl, 0.0, 0.0);
   elm_table_pack(table, lbl, 0, 0, 1, 1);
   evas_object_show(lbl);

   entry = elm_entry_add(parent);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   elm_entry_single_line_set(entry, EINA_TRUE);
   evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, 0.0);
   evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, 0.0);
   elm_table_pack(table, entry, 1, 0, 1, 1);
   evas_object_show(entry);

   replace_lbl = elm_label_add(parent);
   elm_object_text_set(replace_lbl, _("Replace term"));
   evas_object_size_hint_align_set(replace_lbl, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(replace_lbl, 0.0, 0.0);
   elm_table_pack(table, replace_lbl, 0, 1, 1, 1);
   evas_object_show(replace_lbl);

   replace_entry = elm_entry_add(parent);
   elm_entry_scrollable_set(replace_entry, EINA_TRUE);
   elm_entry_single_line_set(replace_entry, EINA_TRUE);
   evas_object_size_hint_align_set(replace_entry, EVAS_HINT_FILL, 0.0);
   evas_object_size_hint_weight_set(replace_entry, EVAS_HINT_EXPAND, 0.0);
   elm_table_pack(table, replace_entry, 1, 1, 1, 1);
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

   wrapped_text = elm_label_add(parent);
   elm_object_text_set(wrapped_text, _("Reached end of file, starting from beginning"));
   elm_box_pack_end(box, wrapped_text);

   checkbox = elm_check_add(parent);
   elm_object_text_set(checkbox, _("Wrap search?"));
   elm_check_state_set(checkbox, EINA_TRUE);
   evas_object_show(checkbox);
   elm_box_pack_end(box, checkbox);

   btn = elm_button_add(parent);
   elm_object_text_set(btn, _("Search"));
   evas_object_size_hint_align_set(btn, 1.0, 0.0);
   evas_object_size_hint_weight_set(btn, 0.0, 0.0);
   evas_object_show(btn);
   elm_box_pack_end(box, btn);
   evas_object_smart_callback_add(btn, "clicked", _edi_search_clicked, editor);

   replace_btn = elm_button_add(parent);
   elm_object_text_set(replace_btn, _("Replace"));
   evas_object_size_hint_align_set(replace_btn, 1.0, 0.0);
   evas_object_size_hint_weight_set(replace_btn, 0.0, 0.0);
   evas_object_show(replace_btn);
   elm_box_pack_end(box, replace_btn);
   evas_object_smart_callback_add(replace_btn, "clicked", _edi_replace_clicked, editor);

   btn = elm_button_add(parent);
   elm_object_text_set(btn, _("Cancel"));
   evas_object_size_hint_align_set(btn, 1.0, 0.0);
   evas_object_size_hint_weight_set(btn, 0.0, 0.0);
   evas_object_show(btn);
   elm_box_pack_end(box, btn);
   evas_object_smart_callback_add(btn, "clicked", _edi_cancel_clicked, editor);

   search = calloc(1, sizeof(*search));
   search->entry = entry;
   search->wrapped_text = wrapped_text;
   search->replace_entry = replace_entry;
   search->replace_btn = replace_btn;
   search->parent = parent;
   search->widget = big_box;
   search->checkbox = checkbox;
   editor->search = search;
   evas_object_show(parent);
}
