#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <libgen.h>

#include <Eina.h>
#include <Elementary.h>

#include "edi_editor.h"

#include "edi_mainview.h"

#include "edi_private.h"

#define EDITOR_FONT "DEFAULT='font=Monospace font_size=12'"

static void
_update_lines(Edi_Editor *editor);

static void
_undo_do(Edi_Editor *editor, Elm_Entry_Change_Info *inf)
{
   if (inf->insert)
     {
        const Evas_Object *tb = elm_entry_textblock_get(editor->entry);
        Evas_Textblock_Cursor *mcur, *end;
        mcur = (Evas_Textblock_Cursor *) evas_object_textblock_cursor_get(tb);
        end = evas_object_textblock_cursor_new(tb);

        if (inf->insert)
          {
             elm_entry_cursor_pos_set(editor->entry, inf->change.insert.pos);
             evas_textblock_cursor_pos_set(end, inf->change.insert.pos +
                   inf->change.insert.plain_length);
          }
        else
          {
             elm_entry_cursor_pos_set(editor->entry, inf->change.del.start);
             evas_textblock_cursor_pos_set(end, inf->change.del.end);
          }

        evas_textblock_cursor_range_delete(mcur, end);
        evas_textblock_cursor_free(end);
        elm_entry_calc_force(editor->entry);
     }
   else
     {
        if (inf->insert)
          {
             elm_entry_cursor_pos_set(editor->entry, inf->change.insert.pos);
             elm_entry_entry_insert(editor->entry, inf->change.insert.content);
          }
        else
          {
             size_t start;
             start = (inf->change.del.start < inf->change.del.end) ?
                inf->change.del.start : inf->change.del.end;

             elm_entry_cursor_pos_set(editor->entry, start);
             elm_entry_entry_insert(editor->entry, inf->change.insert.content);
             elm_entry_cursor_pos_set(editor->entry, inf->change.del.end);
          }
     }
}

static void
_undo_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Elm_Entry_Change_Info *change;
   Edi_Editor *editor = data;

   if (!eina_list_next(editor->undo_stack))
      return;

   change = eina_list_data_get(editor->undo_stack);
   _undo_do(editor, change);
   editor->undo_stack = eina_list_next(editor->undo_stack);
}

static void
_changed_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Entry_Change_Info *change;
   Edi_Editor *editor = data;

   change = calloc(1, sizeof(*change));
   memcpy(change, event_info, sizeof(*change));
   if (change->insert)
     {
        eina_stringshare_ref(change->change.insert.content);
     }
   else
     {
        eina_stringshare_ref(change->change.del.content);
     }

   editor->undo_stack = eina_list_prepend(editor->undo_stack, change);
   _update_lines(editor);
}

static void
_smart_cb_key_down(void *data, Evas *e EINA_UNUSED,
                   Evas_Object *obj EINA_UNUSED, void *event)
{
   Eina_Bool ctrl, alt, shift;
   Evas_Event_Key_Down *ev = event;

   ctrl = evas_key_modifier_is_set(ev->modifiers, "Control");
   alt = evas_key_modifier_is_set(ev->modifiers, "Alt");
   shift = evas_key_modifier_is_set(ev->modifiers, "Shift");

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
             edi_mainview_save();
          }
        else if (!strcmp(ev->key, "f"))
          {
             edi_mainview_search();
          }
        else if (!strcmp(ev->key, "z"))
          {
             _undo_cb(data, obj, event);
          }
     }
}

static int
_get_lines_in_textblock(Evas_Object *textblock)
{
   int lines;
   Evas_Textblock_Cursor *cursor;

   cursor = evas_object_textblock_cursor_new(textblock);
   evas_textblock_cursor_paragraph_last(cursor);
   lines = evas_textblock_cursor_geometry_get(cursor, NULL, NULL, NULL, NULL, NULL, EVAS_TEXTBLOCK_CURSOR_BEFORE);

   evas_textblock_cursor_free(cursor);
   return lines + 1;
}

static void
_update_lines(Edi_Editor *editor)
{
   Eina_Strbuf *content;
   int lines, i;

   lines = _get_lines_in_textblock(elm_entry_textblock_get(editor->entry));

   content = eina_strbuf_new();
   eina_strbuf_append(content, "<align=right>");
   for (i = 1; i <= lines; i++)
     {
        eina_strbuf_append_printf(content, "%d<br>", i);
     }
   eina_strbuf_append(content, "<br><br></align>");
   elm_object_text_set(editor->lines, eina_strbuf_string_get(content));
   eina_strbuf_free(content);
}

static void
_scroll_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Edi_Editor *editor = data;
   Evas_Coord y, h;

   elm_scroller_region_get(editor->entry, NULL, &y, NULL, NULL);
   elm_scroller_region_get(editor->lines, NULL, NULL, NULL, &h);
   elm_scroller_region_show(editor->lines, 0, y, 10, h);
}

static void
_edit_cursor_moved(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   char buf[30];
   int line;
   int col;
   Evas_Object *tb;
   Evas_Textblock_Cursor *cur1, *cur2;

   tb = elm_entry_textblock_get(obj);

   cur1 = evas_object_textblock_cursor_get(tb);
   cur2 = evas_object_textblock_cursor_new(tb);
   line = evas_textblock_cursor_line_geometry_get(cur1, NULL, NULL, NULL, NULL) + 1;
   evas_textblock_cursor_copy(cur1, cur2);
   evas_textblock_cursor_line_char_first(cur2);
   col = evas_textblock_cursor_pos_get(cur1) -
      evas_textblock_cursor_pos_get(cur2) + 1;
   evas_textblock_cursor_free(cur2);

   snprintf(buf, sizeof(buf), "Line:%d, Column:%d", line, col);
   elm_object_text_set((Evas_Object *)data, buf);
}

static void
_edi_editor_statusbar_add(Evas_Object *panel, Edi_Editor *editor)
{
   Evas_Object *position;

   position = elm_label_add(panel);
   evas_object_size_hint_align_set(position, 1.0, 0.5);
   evas_object_size_hint_weight_set(position, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(panel, position);
   evas_object_show(position);
   elm_object_disabled_set(position, EINA_TRUE);

   _edit_cursor_moved(position, editor->entry, NULL);
   evas_object_smart_callback_add(editor->entry, "cursor,changed", _edit_cursor_moved, position);
}

EAPI Evas_Object *edi_editor_add(Evas_Object *parent, const char *path)
{
   Evas_Object *txt, *lines, *vbox, *box, *searchbar, *statusbar;
   Evas_Modifier_Mask ctrl, shift, alt;
   Evas *e;
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

   lines = elm_entry_add(box);
   elm_entry_scrollable_set(lines, EINA_TRUE);
   elm_scroller_policy_set(lines, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
   elm_entry_editable_set(lines, EINA_FALSE);

   elm_entry_text_style_user_push(lines, EDITOR_FONT);
   evas_object_color_set(lines, 127, 127, 127, 255);

   evas_object_size_hint_weight_set(lines, 0.052, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(lines, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, lines);
   evas_object_show(lines);

   txt = elm_entry_add(box);
   elm_entry_editable_set(txt, EINA_TRUE);
   elm_entry_scrollable_set(txt, EINA_TRUE);
   elm_entry_line_wrap_set(txt, EINA_FALSE);
   elm_entry_text_style_user_push(txt, EDITOR_FONT);
   elm_entry_file_set(txt, path, ELM_TEXT_FORMAT_PLAIN_UTF8);
   elm_entry_autosave_set(txt, EDI_CONTENT_AUTOSAVE);
   evas_object_size_hint_weight_set(txt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(txt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(txt);
   elm_box_pack_end(box, txt);

   editor = calloc(1, sizeof(*editor));
   editor->entry = txt;
   editor->lines = lines;
   evas_object_event_callback_add(txt, EVAS_CALLBACK_KEY_DOWN,
                                  _smart_cb_key_down, editor);
   evas_object_smart_callback_add(txt, "changed,user", _changed_cb, editor);
   evas_object_smart_callback_add(txt, "scroll", _scroll_cb, editor);
   evas_object_smart_callback_add(txt, "undo,request", _undo_cb, editor);

   edi_editor_search_add(searchbar, editor);
   _edi_editor_statusbar_add(statusbar, editor);

   e = evas_object_evas_get(txt);
   ctrl = evas_key_modifier_mask_get(e, "Control");
   alt = evas_key_modifier_mask_get(e, "Alt");
   shift = evas_key_modifier_mask_get(e, "Shift");

   (void)!evas_object_key_grab(txt, "Prior", ctrl, shift | alt, 1);
   (void)!evas_object_key_grab(txt, "Next", ctrl, shift | alt, 1);
   (void)!evas_object_key_grab(txt, "s", ctrl, shift | alt, 1);
   (void)!evas_object_key_grab(txt, "f", ctrl, shift | alt, 1);
   (void)!evas_object_key_grab(txt, "z", ctrl, shift | alt, 1);

   evas_object_data_set(vbox, "editor", editor);
   _update_lines(editor);
   return vbox;
}
