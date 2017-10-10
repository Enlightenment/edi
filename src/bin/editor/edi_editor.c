#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <libgen.h>

#include <Eina.h>
#include <Elementary.h>

#include "edi_editor.h"

#include "mainview/edi_mainview.h"
#include "edi_filepanel.h"
#include "edi_config.h"

#include "language/edi_language_provider.h"

#include "edi_private.h"

static Evas_Object *_suggest_hint;

static void _suggest_popup_show(Edi_Editor *editor);

typedef struct
{
   unsigned int line;
   unsigned int col;
} Edi_Location;

typedef struct
{
   Edi_Location start;
   Edi_Location end;
} Edi_Range;

static void
_edi_editor_file_change_reload_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Edi_Editor *editor;

   editor = (Edi_Editor *)data;
   if (!editor) return;

   edi_editor_reload(editor);

   evas_object_del(editor->popup);
   editor->popup = NULL;
}

static void
_edi_editor_file_change_ignore_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Edi_Editor *editor;

   editor = (Edi_Editor *)data;
   if (!editor) return;

   edi_editor_save(editor);
   editor->save_time = time(NULL);
   evas_object_del(editor->popup);
   editor->popup = NULL;
}

static void
_edi_editor_file_change_popup(Evas_Object *parent, Edi_Editor *editor)
{
   Evas_Object *table, *frame, *box, *label, *sep, *icon, *button;

   if (editor->popup)
     return;

   editor->popup = elm_popup_add(parent);
   elm_popup_orient_set(editor->popup, ELM_POPUP_ORIENT_CENTER);
   elm_popup_scrollable_set(editor->popup, EINA_TRUE);
   elm_object_part_text_set(editor->popup, "title,text", _("Confirmation"));
   evas_object_size_hint_align_set(editor->popup, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(editor->popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   table = elm_table_add(editor->popup);
   icon = elm_icon_add(table);
   elm_icon_standard_set(icon, "dialog-warning");
   evas_object_size_hint_min_set(icon, 48 * elm_config_scale_get(), 48 * elm_config_scale_get());
   evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(icon);
   elm_table_pack(table, icon, 0, 0, 1, 1);

   frame = elm_frame_add(editor->popup);
   elm_object_content_set(frame, table);
   evas_object_show(frame);

   box = elm_box_add(editor->popup);
   label = elm_label_add(editor->popup);
   elm_object_text_set(label, _("File contents have changed. Would you like to reload <br> the contents of this file?"));
   evas_object_show(label);
   elm_box_pack_end(box, label);

   sep = elm_separator_add(box);
   elm_separator_horizontal_set(sep, EINA_TRUE);
   evas_object_show(sep);
   elm_box_pack_end(box, sep);
   evas_object_show(box);
   elm_table_pack(table, box, 1, 0, 1, 1);

   elm_object_content_set(editor->popup, frame);
   evas_object_show(table);

   button = elm_button_add(editor->popup);
   elm_object_text_set(button, _("Reload"));
   elm_object_part_content_set(editor->popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked", _edi_editor_file_change_reload_cb, editor);

   button = elm_button_add(editor->popup);
   elm_object_text_set(button, _("No, continue editing"));
   elm_object_part_content_set(editor->popup, "button2", button);
   evas_object_smart_callback_add(button, "clicked", _edi_editor_file_change_ignore_cb, editor);

   evas_object_show(editor->popup);
}

void
edi_editor_save(Edi_Editor *editor)
{
   Elm_Code *code;
   const char *filename;

   if (!editor->modified)
     return;

   code = elm_code_widget_code_get(editor->entry);

   filename = elm_code_file_path_get(code->file);

   elm_code_file_save(code->file);

   editor->save_time = ecore_file_mod_time(filename);

   editor->modified = EINA_FALSE;

   if (editor->save_timer)
     {
        ecore_timer_del(editor->save_timer);
        editor->save_timer = NULL;
     }

   if (edi_language_provider_has(editor))
     edi_language_provider_get(editor)->refresh(editor);

   ecore_event_add(EDI_EVENT_FILE_SAVED, NULL, NULL, NULL);
}

static Eina_Bool
_edi_editor_autosave_cb(void *data)
{
   Edi_Editor *editor;

   editor = (Edi_Editor *)data;
   if (!editor)
     return ECORE_CALLBACK_CANCEL;

   edi_editor_save(editor);

   return ECORE_CALLBACK_CANCEL;
}

static void
_changed_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Edi_Editor *editor = data;

   editor->modified = EINA_TRUE;

   if (editor->save_timer)
     ecore_timer_reset(editor->save_timer);
   else if (_edi_config->autosave)
     editor->save_timer = ecore_timer_add(EDI_CONTENT_SAVE_TIMEOUT, _edi_editor_autosave_cb, editor);
}

static char *
_edi_editor_word_at_position_get(Edi_Editor *editor, unsigned int row, unsigned int col)
{
   Elm_Code *code;
   Elm_Code_Line *line;
   char *ptr, *curword, *curtext;
   unsigned int curlen, wordlen;

   code = elm_code_widget_code_get(editor->entry);
   line = elm_code_file_line_get(code->file, row);

   if (!line)
     return strdup("");

   curtext = (char *)elm_code_line_text_get(line, &curlen);
   ptr = curtext + col - 1;

   while (ptr != curtext &&
         ((*(ptr - 1) >= 'a' && *(ptr - 1) <= 'z') ||
          (*(ptr - 1) >= 'A' && *(ptr - 1) <= 'Z') ||
          (*(ptr - 1) >= '0' && *(ptr - 1) <= '9') ||
          *(ptr - 1) == '_'))
     ptr--;

   wordlen = col - (ptr - curtext);
   curword = malloc(sizeof(char) * (wordlen + 1));
   strncpy(curword, ptr, wordlen);
   curword[wordlen - 1] = '\0';

   return curword;
}

static char *
_edi_editor_current_word_get(Edi_Editor *editor)
{
   unsigned int row, col;

   elm_code_widget_cursor_position_get(editor->entry, &row, &col);

   return _edi_editor_word_at_position_get(editor, row, col);
}

static Evas_Object *
_suggest_list_content_get(void *data, Evas_Object *obj, const char *part)
{
   Edi_Editor *editor;
   Edi_Mainview_Item *item;
   Edi_Language_Suggest_Item *suggest_it = data;
   char *format, *display;
   const char *font;
   int font_size, displen;

   if (strcmp(part, "elm.swallow.content"))
     return NULL;

   item = edi_mainview_item_current_get();

   if (!item)
     return NULL;

   editor = (Edi_Editor *)evas_object_data_get(item->view, "editor");
   elm_code_widget_font_get(editor->entry, &font, &font_size);

   format = "<align=left><font='%s'><font_size=%d> %s</font_size></font></align>";
   displen = strlen(suggest_it->summary) + strlen(format) + strlen(font);
   display = malloc(sizeof(char) * displen);
   snprintf(display, displen, format, font, font_size, suggest_it->summary);

   Evas_Object *label = elm_label_add(obj);
   elm_label_ellipsis_set(label, EINA_TRUE);
   elm_object_text_set(label, display);
   evas_object_color_set(label, 255, 255, 255, 255);
   evas_object_show(label);
   free(display);

   return label;
}

static void
_suggest_list_cb_selected(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Edi_Language_Suggest_Item *suggest_it;
   Evas_Object *label = data;

   suggest_it = elm_object_item_data_get(event_info);

   elm_object_text_set(label, suggest_it->detail);
}

static void
_suggest_list_update(Edi_Editor *editor, char *word)
{
   Edi_Language_Suggest_Item *suggest_it;
   Eina_List *l;
   Elm_Genlist_Item_Class *ic;
   Elm_Object_Item *item;

   elm_genlist_clear(editor->suggest_genlist);

   ic = elm_genlist_item_class_new();
   ic->item_style = "full";
   ic->func.content_get = _suggest_list_content_get;

   EINA_LIST_FOREACH(editor->suggest_list, l, suggest_it)
     {
        if (eina_str_has_prefix(suggest_it->summary, word))
          {
             elm_genlist_item_append(editor->suggest_genlist,
                                     ic,
                                     suggest_it,
                                     NULL,
                                     ELM_GENLIST_ITEM_NONE,
                                     NULL,
                                     NULL);
          }
     }
   elm_genlist_item_class_free(ic);

   item = elm_genlist_first_item_get(editor->suggest_genlist);
   if (item)
     {
        elm_genlist_item_selected_set(item, EINA_TRUE);
        elm_genlist_item_show(item, ELM_GENLIST_ITEM_SCROLLTO_TOP);
        _suggest_popup_show(editor);
     }
   else
     evas_object_hide(editor->suggest_bg);
}

static void
_suggest_list_load(Edi_Editor *editor)
{
   Edi_Language_Provider *provider;
   char *curword;
   unsigned int row, col;

   if (evas_object_visible_get(editor->suggest_bg))
     return;

   provider = edi_language_provider_get(editor);
   if (!provider || !provider->lookup)
     return;

   if (editor->suggest_list)
     {
        Edi_Language_Suggest_Item *suggest_it;

        EINA_LIST_FREE(editor->suggest_list, suggest_it)
          edi_language_suggest_item_free(suggest_it);

        editor->suggest_list = NULL;
     }

   elm_code_widget_cursor_position_get(editor->entry, &row, &col);

   curword = _edi_editor_word_at_position_get(editor, row, col);
   editor->suggest_list = provider->lookup(editor, row, col - strlen(curword));
   free(curword);
}

static void
_suggest_list_selection_insert(Edi_Editor *editor, const char *selection)
{
   char *word;
   unsigned int wordlen, col, row;

   elm_code_widget_cursor_position_get(editor->entry, &row, &col);

   word = _edi_editor_word_at_position_get(editor, row, col);
   wordlen = strlen(word);
   free(word);

   elm_code_widget_selection_start(editor->entry, row, col - wordlen);
   elm_code_widget_selection_end(editor->entry, row, col);
   elm_code_widget_text_at_cursor_insert(editor->entry, selection);
}

static void
_suggest_bg_cb_hide(void *data, Evas *e EINA_UNUSED,
                    Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Edi_Editor *editor = (Edi_Editor *)data;

   evas_object_key_ungrab(editor->suggest_genlist, "Return", 0, 0);
   evas_object_key_ungrab(editor->suggest_genlist, "Up", 0, 0);
   evas_object_key_ungrab(editor->suggest_genlist, "Down", 0, 0);
}

static void
_suggest_list_cb_key_down(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                          void *event_info)
{
   Edi_Editor *editor = (Edi_Editor *)data;
   Edi_Language_Suggest_Item *suggest_it;
   Elm_Object_Item *it;
   Evas_Object *genlist = obj;
   Evas_Event_Key_Down *ev = event_info;

   if (!strcmp(ev->key, "Return"))
     {
        it = elm_genlist_selected_item_get(genlist);
        suggest_it = elm_object_item_data_get(it);

        _suggest_list_selection_insert(editor, suggest_it->summary);
        evas_object_hide(editor->suggest_bg);
     }
   else if (!strcmp(ev->key, "Up"))
     {
        it = elm_genlist_item_prev_get(elm_genlist_selected_item_get(genlist));
        if(!it) it = elm_genlist_last_item_get(genlist);

        elm_genlist_item_selected_set(it, EINA_TRUE);
        elm_genlist_item_show(it, ELM_GENLIST_ITEM_SCROLLTO_IN);
     }
   else if (!strcmp(ev->key, "Down"))
     {
        it = elm_genlist_item_next_get(elm_genlist_selected_item_get(genlist));
        if(!it) it = elm_genlist_first_item_get(genlist);

        elm_genlist_item_selected_set(it, EINA_TRUE);
        elm_genlist_item_show(it, ELM_GENLIST_ITEM_SCROLLTO_IN);
     }
}

static void
_suggest_list_cb_clicked_double(void *data, Evas_Object *obj EINA_UNUSED,
                                void *event_info)
{
   Elm_Object_Item *it = event_info;
   Edi_Language_Suggest_Item *suggest_it;
   Edi_Editor *editor = (Edi_Editor *)data;

   suggest_it = elm_object_item_data_get(it);
   _suggest_list_selection_insert(editor, suggest_it->summary);

   evas_object_hide(editor->suggest_bg);
}

static void
_suggest_popup_show(Edi_Editor *editor)
{
   unsigned int col, row;
   Evas_Coord cx, cy, cw, ch, sh, eh, bg_x, bg_y;
   char *word;

   if (!editor->suggest_genlist)
     return;

   if (elm_genlist_items_count(editor->suggest_genlist) <= 0)
     return;

   if (evas_object_visible_get(editor->suggest_bg))
     return;

   elm_code_widget_cursor_position_get(editor->entry, &row, &col);
   elm_code_widget_geometry_for_position_get(editor->entry, row, col,
                                             &cx, &cy, &cw, &ch);
   evas_object_geometry_get(editor->suggest_bg, NULL, NULL, NULL, &sh);
   evas_object_geometry_get(elm_object_top_widget_get(editor->entry),
                            NULL, NULL, NULL, &eh);

   word = _edi_editor_word_at_position_get(editor, row, col);

   bg_x = cx - (strlen(word) + 1) * cw;
   bg_y = cy + ch;
   if (bg_y + sh > eh)
     bg_y = cy - sh;

   evas_object_move(editor->suggest_bg, bg_x, bg_y);
   evas_object_show(editor->suggest_bg);

   if (!evas_object_key_grab(editor->suggest_genlist, "Return", 0, 0, EINA_TRUE))
     ERR("Failed to grab key - %s", "Return");
   if (!evas_object_key_grab(editor->suggest_genlist, "Up", 0, 0, EINA_TRUE))
     ERR("Failed to grab key - %s", "Up");
   if (!evas_object_key_grab(editor->suggest_genlist, "Down", 0, 0, EINA_TRUE))
     ERR("Failed to grab key - %s", "Down");

   free(word);
}

static void
_suggest_popup_key_down_cb(Edi_Editor *editor, const char *key, const char *string)
{
   Elm_Code *code;
   Elm_Code_Line *line;
   unsigned int col, row;
   char *word = NULL;

   if (!evas_object_visible_get(editor->suggest_bg))
     return;

   elm_code_widget_cursor_position_get(editor->entry, &row, &col);

   code = elm_code_widget_code_get(editor->entry);
   line = elm_code_file_line_get(code->file, row);

   if (!strcmp(key, "Left"))
     {
        if (col - 1 <= 0)
          {
             evas_object_hide(editor->suggest_bg);
             return;
          }

        word = _edi_editor_word_at_position_get(editor, row, col - 1);
        if (!strcmp(word, ""))
          evas_object_hide(editor->suggest_bg);
        else
          _suggest_list_update(editor, word);
     }
   else if (!strcmp(key, "Right"))
     {
        if (line->length < col)
          {
             evas_object_hide(editor->suggest_bg);
             return;
          }

        word = _edi_editor_word_at_position_get(editor, row, col + 1);
        if (!strcmp(word, ""))
          evas_object_hide(editor->suggest_bg);
        else
          _suggest_list_update(editor, word);
     }
   else if (!strcmp(key, "BackSpace"))
     {
        if (col - 1 <= 0)
          {
             evas_object_hide(editor->suggest_bg);
             return;
          }

        word = _edi_editor_word_at_position_get(editor, row, col - 1);
        if (!strcmp(word, ""))
          evas_object_hide(editor->suggest_bg);
        else
          _suggest_list_update(editor, word);
     }
   else if (!strcmp(key, "Escape"))
     {
        evas_object_hide(editor->suggest_bg);
     }
   else if (!strcmp(key, "Delete"))
     {
        //Do nothing
     }
   else if (string && strlen(string) == 1)
     {
        word = _edi_editor_word_at_position_get(editor, row, col);
        strncat(word, string, 1);
        _suggest_list_update(editor, word);
     }
   if (word)
     free(word);
}

static void
_suggest_popup_setup(Edi_Editor *editor)
{
   //Popup bg
   Evas_Object *bg = elm_bg_add(editor->entry);
   editor->suggest_bg = bg;
   elm_bg_color_set(bg, 30, 30, 30);
   evas_object_event_callback_add(bg, EVAS_CALLBACK_HIDE,
                                  _suggest_bg_cb_hide, editor);
   evas_object_resize(bg, 400 * elm_config_scale_get(), 300 * elm_config_scale_get());

   //Box
   Evas_Object *box = elm_box_add(bg);
   elm_box_padding_set(box, 5, 5);
   elm_object_content_set(bg, box);
   evas_object_show(box);

   //Genlist
   Evas_Object *genlist = elm_genlist_add(box);
   editor->suggest_genlist = genlist;
   evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_focus_allow_set(genlist, EINA_FALSE);
   elm_genlist_homogeneous_set(genlist, EINA_TRUE);
   evas_object_event_callback_add(genlist, EVAS_CALLBACK_KEY_DOWN,
                                  _suggest_list_cb_key_down, editor);
   evas_object_smart_callback_add(genlist, "clicked,double",
                                  _suggest_list_cb_clicked_double, editor);
   evas_object_show(genlist);
   elm_box_pack_end(box, genlist);

   //Label
   Evas_Object *label = elm_label_add(box);
   elm_label_line_wrap_set(label, ELM_WRAP_MIXED);
   evas_object_size_hint_align_set(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(label);
   elm_box_pack_end(box, label);

   evas_object_smart_callback_add(genlist, "selected",
                                  _suggest_list_cb_selected, label);
}

static void
_edi_editor_snippet_insert(Edi_Editor *editor)
{
   char *key, *leading;
   unsigned int line_len;
   int nl_pos;
   short nl_len;
   const char *snippet, *ptr;
   unsigned int row, col;
   Edi_Language_Provider *provider;
   Elm_Code_Line *line;

   provider = edi_language_provider_get(editor);
   elm_code_widget_cursor_position_get(editor->entry, &row, &col);
   key = _edi_editor_word_at_position_get(editor, row, col);
   snippet = provider->snippet_get(key);

   if (!snippet)
     return;

   line = elm_code_file_line_get(elm_code_widget_code_get(editor->entry)->file, row);
   leading = elm_code_line_indent_get(line);

   elm_code_widget_selection_select_word(editor->entry, row, col);
   elm_code_widget_selection_delete(editor->entry);

   ptr = snippet;
   nl_pos = elm_code_text_newlinenpos(ptr, strlen(ptr), &nl_len);

   while (nl_pos != ELM_CODE_TEXT_NOT_FOUND)
     {
        char *insert;
        line_len = nl_pos + nl_len;

        insert = strndup(ptr, line_len);
        elm_code_widget_text_at_cursor_insert(editor->entry, insert);
        elm_code_widget_text_at_cursor_insert(editor->entry, leading);
        free(insert);

        ptr += line_len;
        nl_pos = elm_code_text_newlinenpos(ptr, strlen(ptr), &nl_len);
     }
   elm_code_widget_text_at_cursor_insert(editor->entry, ptr);

   free(key);
   return;
}

static void
_suggest_hint_hide(Edi_Editor *editor EINA_UNUSED)
{
   if (!_suggest_hint)
     return;

   evas_object_hide(_suggest_hint);
}

static Evas_Object *
_suggest_hint_popup_add(Edi_Editor *editor, const char *content, Evas_Smart_Cb fn)
{
   Evas_Object *pop, *btn, *text;
   unsigned int row, col;
   Evas_Coord cx, cy, cw, ch;

   elm_code_widget_cursor_position_get(editor->entry, &row, &col);

   elm_code_widget_cursor_position_get(editor->entry, &row, &col);
   elm_code_widget_geometry_for_position_get(editor->entry, row, col,
                                             &cx, &cy, &cw, &ch);

   pop = elm_box_add(editor->entry);
   evas_object_size_hint_weight_set(pop, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(pop, EVAS_HINT_FILL, EVAS_HINT_FILL);

   evas_object_size_hint_weight_set(pop, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(pop, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_move(pop, cx, cy - ch);

   btn = elm_button_add(pop);
   evas_object_smart_callback_add(btn, "clicked", fn, editor);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(pop, btn);
   evas_object_show(btn);

   text = elm_label_add(btn);
   evas_object_size_hint_weight_set(text, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(text, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(text, content);

   elm_layout_content_set(btn, "elm.swallow.content", text);
   evas_object_show(text);

   return pop;
}

static void
_suggest_hint_click_snippet(void *data, Evas_Object *obj EINA_UNUSED,
                            void *event_info EINA_UNUSED)
{
   Edi_Editor *editor = data;

   _suggest_hint_hide(editor);
   _edi_editor_snippet_insert(editor);
}

static void
_suggest_hint_show_snippet(Edi_Editor *editor, const char *word)
{
   unsigned int row, col;
   const char *snippet;

   if (!word || strlen(word) == 0)
     return;

   elm_code_widget_cursor_position_get(editor->entry, &row, &col);

   snippet = edi_language_provider_get(editor)->snippet_get(word);
   if (!snippet)
     return;

   _suggest_hint = _suggest_hint_popup_add(editor,
      eina_slstr_printf("Press tab to insert snippet <hilight>%s</hilight>", word),
      _suggest_hint_click_snippet);
   evas_object_show(_suggest_hint);
}

Edi_Language_Suggest_Item *
_suggest_match_get(Edi_Editor *editor, const char *word)
{
   Edi_Language_Suggest_Item *suggest_it;
   unsigned int wordlen;

   Eina_List *l, *list = editor->suggest_list;

   wordlen = strlen(word);
   EINA_LIST_FOREACH(list, l, suggest_it)
     {
        if (strlen(suggest_it->summary) <= wordlen)
          continue;
        if (eina_str_has_prefix(suggest_it->summary, word))
          return suggest_it;
     }

   return NULL;
}

static void
_suggest_hint_click_suggest(void *data, Evas_Object *obj EINA_UNUSED,
                            void *event_info EINA_UNUSED)
{
   Edi_Editor *editor = data;
   Edi_Language_Suggest_Item *match;
   char *word;

   _suggest_hint_hide(editor);

   word = _edi_editor_current_word_get(editor);
   match = _suggest_match_get(editor, word);
   if (match)
     _suggest_list_selection_insert(editor, match->summary);

   free(word);
}

static void
_suggest_hint_show_match(Edi_Editor *editor, const char *word)
{
   Edi_Language_Suggest_Item *match;

   match = _suggest_match_get(editor, word);
   if (!match)
     return;

   _suggest_hint = _suggest_hint_popup_add(editor,
      eina_slstr_printf("Press tab to insert suggestion <hilight>%s</hilight>", match->summary),
      _suggest_hint_click_suggest);

   evas_object_show(_suggest_hint);
}

static void
_smart_cb_key_down(void *data EINA_UNUSED, Evas *e EINA_UNUSED,
                   Evas_Object *obj EINA_UNUSED, void *event)
{
   Edi_Mainview_Item *item;
   Edi_Editor *editor;
   Edi_Language_Provider *provider;
   Eina_Bool ctrl, alt, shift;
   Evas_Event_Key_Down *ev = event;

   ctrl = evas_key_modifier_is_set(ev->modifiers, "Control");
   alt = evas_key_modifier_is_set(ev->modifiers, "Alt");
   shift = evas_key_modifier_is_set(ev->modifiers, "Shift");

   item = edi_mainview_item_current_get();

   if (!item)
     return;

   editor = (Edi_Editor *)evas_object_data_get(item->view, "editor");
   _suggest_hint_hide(editor);

   if ((!alt) && (ctrl) && (!shift))
     {
        if (!strcmp(ev->key, "Prior"))
          {
             edi_mainview_item_prev();
          }
        else if (!strcmp(ev->key, "Next"))
          {
             edi_mainview_item_next();
          }
        else if (!strcmp(ev->key, "s"))
          {
             edi_editor_save(editor);
          }
        else if (!strcmp(ev->key, "f"))
          {
             edi_editor_search(editor);
          }
        else if (!strcmp(ev->key, "g"))
          {
             edi_mainview_goto_popup_show();
          }
        else if (edi_language_provider_has(editor) && !strcmp(ev->key, "space"))
          {
             _suggest_list_load(editor);
             _suggest_list_update(editor, _edi_editor_current_word_get(editor));
          }
     }
   else if ((!alt) && (ctrl) && (shift))
     {
        if (edi_language_provider_has(editor) && !strcmp(ev->key, "space"))
          {
             edi_editor_doc_open(editor);
          }
     }

   if (alt || ctrl)
     return;

   provider = edi_language_provider_get(editor);
   if (!provider)
     return;

   if (evas_object_visible_get(editor->suggest_bg))
     {
        _suggest_popup_key_down_cb(editor, ev->key, ev->string);
        return;
     }

   if (!strcmp(ev->key, "Tab"))
     {
        char *word;
        const char *snippet;
        Edi_Language_Suggest_Item *suggest;

        word = _edi_editor_current_word_get(editor);
        snippet = provider->snippet_get(word);

        if (snippet)
          {
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
             _edi_editor_snippet_insert(editor);
          }
        else if (strlen(word) >= 3)
          {
             suggest = _suggest_match_get(editor, word);
             if (suggest)
               {
                  ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
                  _suggest_list_selection_insert(editor, suggest->summary);
               }
          }

        free(word);
     }
}

static void
_edit_cursor_moved(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Elm_Code_Widget *widget;
   char buf[30];
   unsigned int line;
   unsigned int col;

   widget = (Elm_Code_Widget *)obj;
   elm_code_widget_cursor_position_get(widget, &line, &col);

   snprintf(buf, sizeof(buf), _("Line:%d, Column:%d"), line, col);
   elm_object_text_set((Evas_Object *)data, buf);
}

static void
_edit_file_changed(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Edi_Editor *editor;
   Edi_Language_Provider *provider;
   char *word;
   const char *snippet;

   ecore_event_add(EDI_EVENT_FILE_CHANGED, NULL, NULL, NULL);

   editor = (Edi_Editor *)data;

   _suggest_hint_hide(editor);
   if (evas_object_visible_get(editor->suggest_bg))
     return;

   provider = edi_language_provider_get(editor);
   if (!provider)
     return;

   word = _edi_editor_current_word_get(editor);

   if (word && strlen(word) > 1)
     {
        snippet = provider->snippet_get(word);
        if (snippet)
          _suggest_hint_show_snippet(editor, word);
        else if (strlen(word) >= 3)
          _suggest_hint_show_match(editor, word);
     }

   free(word);
}

static void
_edi_editor_statusbar_add(Evas_Object *panel, Edi_Editor *editor, Edi_Mainview_Item *item)
{
   Edi_Language_Provider *provider;
   Evas_Object *position, *mime, *lines;
   Elm_Code *code;

   elm_box_horizontal_set(panel, EINA_TRUE);

   mime = elm_label_add(panel);
   if (item->mimetype)
     {
        provider = edi_language_provider_get(editor);
        if (provider && provider->mime_name(item->mimetype))
          {
             char summary[1024];
             sprintf(summary, "%s (%s)", provider->mime_name(item->mimetype), item->mimetype);
             elm_object_text_set(mime, summary);
          }
        else
          elm_object_text_set(mime, item->mimetype);
     }
   else
     elm_object_text_set(mime, item->editortype);
   evas_object_size_hint_align_set(mime, 0.0, 0.5);
   evas_object_size_hint_weight_set(mime, 0.1, 0.0);
   elm_box_pack_end(panel, mime);
   evas_object_show(mime);
   elm_object_disabled_set(mime, EINA_TRUE);

   lines = elm_label_add(panel);
   code = elm_code_widget_code_get(editor->entry);
   if (elm_code_file_line_ending_get(code->file) == ELM_CODE_FILE_LINE_ENDING_WINDOWS)
     elm_object_text_set(lines, _("WIN"));
   else
     elm_object_text_set(lines, _("UNIX"));
   evas_object_size_hint_align_set(lines, 0.0, 0.5);
   evas_object_size_hint_weight_set(lines, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(panel, lines);
   evas_object_show(lines);
   elm_object_disabled_set(lines, EINA_TRUE);

   position = elm_label_add(panel);
   evas_object_size_hint_align_set(position, 1.0, 0.5);
   evas_object_size_hint_weight_set(position, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(panel, position);
   evas_object_show(position);
   elm_object_disabled_set(position, EINA_TRUE);

   _edit_cursor_moved(position, editor->entry, NULL);
   evas_object_smart_callback_add(editor->entry, "cursor,changed", _edit_cursor_moved, position);
   evas_object_smart_callback_add(editor->entry, "changed,user", _edit_file_changed, editor);
}

#if HAVE_LIBCLANG
static void
_edi_range_color_set(Edi_Editor *editor, Edi_Range range, Elm_Code_Token_Type type)
{
   Elm_Code *code;
   Elm_Code_Line *line, *extra_line;
   unsigned int number;

   ecore_thread_main_loop_begin();

   code = elm_code_widget_code_get(editor->entry);
   line = elm_code_file_line_get(code->file, range.start.line);

   elm_code_line_token_add(line, range.start.col - 1, range.end.col - 2,
                           range.end.line - range.start.line + 1, type);

   elm_code_widget_line_refresh(editor->entry, line);
   for (number = line->number + 1; number <= range.end.line; number++)
     {
        extra_line = elm_code_file_line_get(code->file, number);
        elm_code_widget_line_refresh(editor->entry, extra_line);
     }

   ecore_thread_main_loop_end();
}

static void
_edi_line_status_set(Edi_Editor *editor, unsigned int number, Elm_Code_Status_Type status,
                     const char *text)
{
   Elm_Code *code;
   Elm_Code_Line *line;

   ecore_thread_main_loop_begin();

   code = elm_code_widget_code_get(editor->entry);
   line = elm_code_file_line_get(code->file, number);
   if (!line)
     {
        if (text)
          ERR("Status on invalid line %d (\"%s\")", number, text);

        ecore_thread_main_loop_end();
        return;
     }

   elm_code_line_status_set(line, status);
   if (text)
     elm_code_line_status_text_set(line, text);

   elm_code_widget_line_refresh(editor->entry, line);

   ecore_thread_main_loop_end();
}

static void
_clang_load_highlighting(const char *path, Edi_Editor *editor)
{
        CXFile cfile = clang_getFile(editor->clang_unit, path);

        CXSourceRange range = clang_getRange(
              clang_getLocationForOffset(editor->clang_unit, cfile, 0),
              clang_getLocationForOffset(editor->clang_unit, cfile, ecore_file_size(path)));

        clang_tokenize(editor->clang_unit, range, &editor->tokens, &editor->token_count);
        editor->cursors = (CXCursor *) malloc(editor->token_count * sizeof(CXCursor));
        clang_annotateTokens(editor->clang_unit, editor->tokens, editor->token_count, editor->cursors);
}

static void
_clang_show_highlighting(Edi_Editor *editor)
{
   unsigned int i = 0;

   for (i = 0 ; i < editor->token_count ; i++)
     {
        Edi_Range range;
        Elm_Code_Token_Type type = ELM_CODE_TOKEN_TYPE_DEFAULT;

        CXSourceRange tkrange = clang_getTokenExtent(editor->clang_unit, editor->tokens[i]);
        clang_getSpellingLocation(clang_getRangeStart(tkrange), NULL,
              &range.start.line, &range.start.col, NULL);
        clang_getSpellingLocation(clang_getRangeEnd(tkrange), NULL,
              &range.end.line, &range.end.col, NULL);
        /* FIXME: Should probably do something fancier, this is only a limited
         * number of types. */
        switch (clang_getTokenKind(editor->tokens[i]))
          {
             case CXToken_Punctuation:
                break;
             case CXToken_Identifier:
                if (editor->cursors[i].kind < CXCursor_FirstRef)
                  {
                      type = ELM_CODE_TOKEN_TYPE_CLASS;
                      break;
                  }
                switch (editor->cursors[i].kind)
                  {
                   case CXCursor_DeclRefExpr:
                      /* Handle different ref kinds */
                      type = ELM_CODE_TOKEN_TYPE_FUNCTION;
                      break;
                   case CXCursor_MacroDefinition:
                   case CXCursor_InclusionDirective:
                   case CXCursor_PreprocessingDirective:
                   case CXCursor_MacroExpansion:
                      type = ELM_CODE_TOKEN_TYPE_PREPROCESSOR;
                      break;
                   case CXCursor_TypeRef:
                      type = ELM_CODE_TOKEN_TYPE_TYPE;
                      break;
                   default:
                      break;
                  }
                break;
             case CXToken_Keyword:
             case CXToken_Literal:
             case CXToken_Comment:
                break;
          }

        if (editor->highlight_cancel)
          break;
        if (type != ELM_CODE_TOKEN_TYPE_DEFAULT)
          _edi_range_color_set(editor, range, type);
     }
}

static void
_clang_free_highlighting(Edi_Editor *editor)
{
   free(editor->cursors);
   clang_disposeTokens(editor->clang_unit, editor->tokens, editor->token_count);
}

static void
_clang_load_errors(Edi_Editor *editor)
{
   Elm_Code *code;
   const char *filename;
   unsigned n = clang_getNumDiagnostics(editor->clang_unit);
   unsigned i = 0;

   ecore_thread_main_loop_begin();
   code = elm_code_widget_code_get(editor->entry);
   filename = elm_code_file_path_get(code->file);
   ecore_thread_main_loop_end();

   for(i = 0, n = clang_getNumDiagnostics(editor->clang_unit); i != n; ++i)
     {
        CXDiagnostic diag = clang_getDiagnostic(editor->clang_unit, i);
        CXFile file;
        unsigned int line;
        CXString path;

        // the parameter after line would be a caret position but we're just highlighting for now
        clang_getSpellingLocation(clang_getDiagnosticLocation(diag), &file, &line, NULL, NULL);

        path = clang_getFileName(file);
        if (!clang_getCString(path) || strcmp(filename, clang_getCString(path)))
          continue;

        /* FIXME: Also handle ranges and fix suggestions. */
        Elm_Code_Status_Type status = ELM_CODE_STATUS_TYPE_DEFAULT;

        switch (clang_getDiagnosticSeverity(diag))
          {
           case CXDiagnostic_Ignored:
              status = ELM_CODE_STATUS_TYPE_IGNORED;
              break;
           case CXDiagnostic_Note:
              status = ELM_CODE_STATUS_TYPE_NOTE;
              break;
           case CXDiagnostic_Warning:
              status = ELM_CODE_STATUS_TYPE_WARNING;
              break;
           case CXDiagnostic_Error:
              status = ELM_CODE_STATUS_TYPE_ERROR;
              break;
           case CXDiagnostic_Fatal:
              status = ELM_CODE_STATUS_TYPE_FATAL;
              break;
          }
        CXString str = clang_getDiagnosticSpelling(diag);
        if (status != ELM_CODE_STATUS_TYPE_DEFAULT)
          _edi_line_status_set(editor, line, status, clang_getCString(str));
        clang_disposeString(str);

        clang_disposeDiagnostic(diag);
        if (editor->highlight_cancel)
          break;
     }
}

static void
_edi_clang_setup(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Edi_Editor *editor;
   Elm_Code *code;
   const char *path;

   ecore_thread_main_loop_begin();

   editor = (Edi_Editor *)data;
   code = elm_code_widget_code_get(editor->entry);
   path = elm_code_file_path_get(code->file);

   ecore_thread_main_loop_end();

   _clang_load_errors(editor);
   _clang_load_highlighting(path, editor);
   _clang_show_highlighting(editor);
}

static void
_edi_clang_dispose(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Edi_Editor *editor = (Edi_Editor *)data;

   _clang_free_highlighting(editor);

   editor->highlight_thread = NULL;
   editor->highlight_cancel = EINA_FALSE;
}
#endif

static void
_focused_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Edi_Mainview_Panel *panel;
   Edi_Mainview_Item *item;
   Elm_Code *code;
   Edi_Editor *editor;
   const char *filename;
   time_t mtime;

   item = (Edi_Mainview_Item *)data;
   panel = edi_mainview_panel_for_item_get(item);

   edi_mainview_panel_focus(panel);

   editor = evas_object_data_get(obj, "editor");

   code = elm_code_widget_code_get(editor->entry);
   filename = elm_code_file_path_get(code->file);

   edi_filepanel_select_path(filename);

   mtime = ecore_file_mod_time(filename);

   if ((editor->save_time) && (editor->save_time < mtime))
     {
        ecore_timer_del(editor->save_timer);
        editor->save_timer = NULL;
        _edi_editor_file_change_popup(evas_object_smart_parent_get(obj), editor);
        editor->modified = EINA_FALSE;
        return;
     }
}

static void
_unfocused_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Edi_Editor *editor;

   editor = (Edi_Editor *)data;

   if (_edi_config->autosave)
     edi_editor_save(editor);

   _suggest_hint_hide(editor);
   if (editor->suggest_bg)
     evas_object_hide(editor->suggest_bg);

   if (editor->doc_popup && evas_object_visible_get(editor->doc_popup))
     evas_object_del(editor->doc_popup);
}

static void
_mouse_up_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
             void *event_info)
{
   Edi_Editor *editor;
   Evas_Event_Mouse_Up *event;
   Eina_Bool ctrl;
   unsigned int row;
   int col;
   const char *word;

   editor = (Edi_Editor *)data;
   event = (Evas_Event_Mouse_Up *)event_info;

   _suggest_hint_hide(editor);
   if (editor->suggest_bg)
     evas_object_hide(editor->suggest_bg);

   ctrl = evas_key_modifier_is_set(event->modifiers, "Control");
   if (event->button != 3 || !ctrl || !edi_language_provider_has(editor))
     return;

   elm_code_widget_position_at_coordinates_get(editor->entry, event->canvas.x, event->canvas.y, &row, &col);
   elm_code_widget_selection_select_word(editor->entry, row, col);
   word = elm_code_widget_selection_text_get(editor->entry);
   if (!word || !strlen(word))
     return;

   edi_editor_doc_open(editor);
}

static void
_edi_editor_parse_line_cb(Elm_Code_Line *line EINA_UNUSED, void *data)
{
   Edi_Editor *editor = (Edi_Editor *)data;

   // We have caused a reset in the file parser, if it is active
   if (!editor->highlight_thread)
     return;

   editor->highlight_cancel = EINA_TRUE;
}

static void
_edi_editor_parse_file_cb(Elm_Code_File *file EINA_UNUSED, void *data)
{
   Edi_Editor *editor;

   editor = (Edi_Editor *)data;
   if (editor->highlight_thread)
     return;

#if HAVE_LIBCLANG
   editor->highlight_cancel = EINA_FALSE;
   editor->highlight_thread = ecore_thread_run(_edi_clang_setup, _edi_clang_dispose, NULL, editor);
#endif

   if (edi_language_provider_has(editor))
     _suggest_list_load(editor);
}

static Eina_Bool
_edi_editor_config_changed(void *data, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   Elm_Code_Widget *widget;
   Elm_Code *code;

   widget = (Elm_Code_Widget *) data;
   code = elm_code_widget_code_get(widget);

   code->config.trim_whitespace = _edi_config->trim_whitespace;

   elm_obj_code_widget_font_set(widget, _edi_project_config->font.name, _edi_project_config->font.size);
   elm_obj_code_widget_show_whitespace_set(widget, _edi_project_config->gui.show_whitespace);
   elm_obj_code_widget_tab_inserts_spaces_set(widget, _edi_project_config->gui.tab_inserts_spaces);
   elm_obj_code_widget_line_width_marker_set(widget, _edi_project_config->gui.width_marker);
   elm_obj_code_widget_tabstop_set(widget, _edi_project_config->gui.tabstop);

   return ECORE_CALLBACK_RENEW;
}

void
edi_editor_widget_config_get(Elm_Code_Widget *widget)
{
   _edi_editor_config_changed(widget, 0, NULL);
}

static void
_editor_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *o, void *event_info EINA_UNUSED)
{
   Edi_Editor *editor = (Edi_Editor *)evas_object_data_get(o, "editor");
   Ecore_Event_Handler *ev_handler = data;

   ecore_event_handler_del(ev_handler);

   if (edi_language_provider_has(editor))
     edi_language_provider_get(editor)->del(editor);
}

void
edi_editor_reload(Edi_Editor *editor)
{
   Elm_Code *code;
   char *path;

   ecore_thread_main_loop_begin();

   code = elm_code_widget_code_get(editor->entry);
   path = strdup(elm_code_file_path_get(code->file));
   elm_code_file_clear(code->file);
   code->file = elm_code_file_open(code, path);
   editor->modified = EINA_FALSE;
   editor->save_time = ecore_file_mod_time(path);

   if (editor->save_timer)
     {
        ecore_timer_del(editor->save_timer);
        editor->save_timer = NULL;
     }

   free(path);
   ecore_thread_main_loop_end();
}

Evas_Object *
edi_editor_add(Evas_Object *parent, Edi_Mainview_Item *item)
{
   Evas_Object *vbox, *box, *searchbar, *statusbar;
   Evas_Modifier_Mask ctrl, shift, alt;
   Ecore_Event_Handler *ev_handler;
   Evas *e;

   Elm_Code *code;
   Elm_Code_Widget *widget;
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

   code = elm_code_create();
   widget = elm_code_widget_add(vbox, code);
   elm_code_widget_editable_set(widget, EINA_TRUE);
   elm_code_widget_line_numbers_set(widget, EINA_TRUE);
   _edi_editor_config_changed(widget, 0, NULL);

   editor = calloc(1, sizeof(*editor));
   editor->entry = widget;
   editor->mimetype = item->mimetype;
   evas_object_data_set(widget, "editor", editor);
   evas_object_event_callback_add(widget, EVAS_CALLBACK_KEY_DOWN,
                                  _smart_cb_key_down, editor);
   evas_object_smart_callback_add(widget, "changed,user", _changed_cb, editor);
   evas_object_event_callback_add(widget, EVAS_CALLBACK_MOUSE_UP, _mouse_up_cb, editor);
   evas_object_smart_callback_add(widget, "focused", _focused_cb, item);
   evas_object_smart_callback_add(widget, "unfocused", _unfocused_cb, editor);

   elm_code_parser_standard_add(code, ELM_CODE_PARSER_STANDARD_TODO);
   if (!strcmp(item->editortype, "code"))
     {
        elm_code_parser_add(code, _edi_editor_parse_line_cb,
                            _edi_editor_parse_file_cb, editor);
        elm_code_widget_syntax_enabled_set(widget, EINA_TRUE);
     }
   elm_code_file_open(code, item->path);
   if (eina_str_has_extension(item->path, ".eo"))
     {
        code->file->mime = "text/x-eolian";
        elm_code_widget_syntax_enabled_set(widget, EINA_TRUE);
     }

   editor->save_time = ecore_file_mod_time(item->path);

   evas_object_size_hint_weight_set(widget, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget);
   elm_box_pack_end(box, widget);

   edi_editor_search_add(searchbar, editor);
   _edi_editor_statusbar_add(statusbar, editor, item);

   e = evas_object_evas_get(widget);
   ctrl = evas_key_modifier_mask_get(e, "Control");
   alt = evas_key_modifier_mask_get(e, "Alt");
   shift = evas_key_modifier_mask_get(e, "Shift");

   (void)!evas_object_key_grab(widget, "Prior", ctrl, shift | alt, 1);
   (void)!evas_object_key_grab(widget, "Next", ctrl, shift | alt, 1);
   (void)!evas_object_key_grab(widget, "s", ctrl, shift | alt, 1);
   (void)!evas_object_key_grab(widget, "f", ctrl, shift | alt, 1);
   (void)!evas_object_key_grab(widget, "g", ctrl, shift | alt, 1);
   (void)!evas_object_key_grab(widget, "space", ctrl, shift | alt, 1);

   evas_object_data_set(item->view, "editor", editor);
   ev_handler = ecore_event_handler_add(EDI_EVENT_CONFIG_CHANGED, _edi_editor_config_changed, widget);
   evas_object_event_callback_add(item->view, EVAS_CALLBACK_DEL, _editor_del_cb, ev_handler);

   if (edi_language_provider_has(editor))
     {
        edi_language_provider_get(editor)->add(editor);
        _suggest_popup_setup(editor);
     }

   return vbox;
}
