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

static void _set_offset(Evas_Object *lines, int offset, int height)
{
   elm_scroller_region_show(lines, 0, offset, 10, height);
}

static void _set_line(Evas_Object *lines, int line, int count)
{
   Eina_Strbuf *content;
   int i;

   content = eina_strbuf_new();
   eina_strbuf_append(content, "<align=right>");
   for (i = line; i < line + count; i++)
     {
        eina_strbuf_append_printf(content, "%d<br>", i);
     }
   eina_strbuf_append(content, "</align>");
   elm_object_text_set(lines, eina_strbuf_string_get(content));
   eina_strbuf_free(content);
}

static void
_scroll_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Edi_Editor *editor = data;
   Evas_Coord y, h;
   int line, lines, offset, line_height;

   line_height = (int) (15.0 * elm_config_scale_get());
   elm_scroller_region_get(editor->entry, NULL, &y, NULL, &h);
   offset = y % line_height;
   line = (y - offset) / line_height + 1;
   lines = h / line_height + 2;
   elm_scroller_region_get(editor->lines, NULL, NULL, NULL, &h);

   _set_offset(editor->lines, offset, h);
   _set_line(editor->lines, line, lines);
}

EAPI Evas_Object *edi_editor_add(Evas_Object *parent, const char *path)
{
   Evas_Object *txt, *lines, *box;
   Evas_Modifier_Mask ctrl, shift, alt;
   Evas *e;
   Edi_Editor *editor;

   box = elm_box_add(parent);
   elm_box_horizontal_set(box, EINA_TRUE);
   evas_object_show(box);

   lines = elm_entry_add(box);
   elm_entry_scrollable_set(lines, EINA_TRUE);
   elm_scroller_policy_set(lines, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
   elm_entry_editable_set(lines, EINA_FALSE);

   elm_entry_text_style_user_push(lines, EDITOR_FONT);
   evas_object_color_set(lines, 127, 127, 127, 255);
   _set_line(lines, 1, 200);

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

   e = evas_object_evas_get(txt);
   ctrl = evas_key_modifier_mask_get(e, "Control");
   alt = evas_key_modifier_mask_get(e, "Alt");
   shift = evas_key_modifier_mask_get(e, "Shift");

   (void)!evas_object_key_grab(txt, "Prior", ctrl, shift | alt, 1);
   (void)!evas_object_key_grab(txt, "Next", ctrl, shift | alt, 1);
   (void)!evas_object_key_grab(txt, "s", ctrl, shift | alt, 1);
   (void)!evas_object_key_grab(txt, "f", ctrl, shift | alt, 1);
   (void)!evas_object_key_grab(txt, "z", ctrl, shift | alt, 1);

   evas_object_data_set(box, "editor", editor);
   return box;
}
