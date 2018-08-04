#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <libgen.h>

#include <Eina.h>
#include <Eio.h>
#include <Elementary.h>

#include "mainview/edi_mainview_item.h"
#include "mainview/edi_mainview_panel.h"
#include "mainview/edi_mainview.h"

#include "edi_filepanel.h"
#include "editor/edi_editor.h"
#include "edi_content_provider.h"

#include "edi_private.h"
#include "edi_config.h"

typedef struct _Edi_Mainview_State {
   Edi_Mainview_Panel *panel;
   char *path;
}  Edi_Mainview_State;

static Edi_Mainview_State *_cached = NULL;

static Evas_Object *_main_win;
static Evas_Object *_edi_mainview_goto_popup;

static void
dummy()
{}

static void
_edi_mainview_panel_cache_set(void)
{
   Edi_Mainview_Item *item;

   if (!_cached)
     _cached = calloc(1, sizeof(Edi_Mainview_State));

   if (_cached->path)
     {
        free(_cached->path);
        _cached->path = NULL;
     }

   _cached->panel = edi_mainview_panel_current_get();
   item = edi_mainview_item_current_get();
   if (item)
     _cached->path = strdup(item->path);
}

static Edi_Mainview_State *
_edi_mainview_panel_cache_get(void)
{
   if (!_cached || !_cached->panel || !_cached->path)
     return NULL;

   return _cached;
}

static Edi_Mainview_Item *
_get_item_for_path(Edi_Mainview_Panel *panel, const char *path)
{
   Eina_List *item;
   Edi_Mainview_Item *it;

   if (!panel || !path) return NULL;

   EINA_LIST_FOREACH(panel->items, item, it)
     {
        if (it && !strcmp(it->path, path))
          return it;
     }
   return NULL;
}

unsigned int
edi_mainview_panel_item_count(Edi_Mainview_Panel *panel)
{
   return eina_list_count(panel->items);
}

Edi_Mainview_Item *
edi_mainview_panel_item_current_get(Edi_Mainview_Panel *panel)
{
   if (!panel) return NULL;
   return panel->current;
}

unsigned int
edi_mainview_panel_item_current_tab_get(Edi_Mainview_Panel *panel)
{
   Eina_List *item;
   Edi_Mainview_Item *it;
   unsigned int i = 0;

   EINA_LIST_FOREACH(panel->items, item, it)
     {
        if (!it->win)
          i++;
        if (it && it == panel->current)
          break;
     }

   return i;
}

static void
_edi_mainview_panel_current_tab_hide(Edi_Mainview_Panel *panel)
{
   Edi_Editor *editor;

   edi_mainview_panel_focus(panel);

   if (!panel || !panel->current)
     return;

   editor = (Edi_Editor *)evas_object_data_get(panel->current->view, "editor");
   if (editor)
     {
        elm_object_focus_set(editor->entry, EINA_FALSE);
        evas_object_hide(editor->entry);
     }
}

static void
_edi_mainview_panel_current_tab_show(Edi_Mainview_Panel *panel)
{
   Edi_Editor *editor;

   edi_mainview_panel_focus(panel);

   if (!panel || !panel->current)
     return;

   editor = (Edi_Editor *)evas_object_data_get(panel->current->view, "editor");
   if (editor)
     {
        evas_object_show(editor->entry);
        elm_object_focus_set(editor->entry, EINA_TRUE);
     }
}

Edi_Mainview_Item *
_edi_mainview_panel_item_for_view_get(Edi_Mainview_Panel *panel, Evas_Object *view)
{
   Eina_List *item;
   Edi_Mainview_Item *it;

   EINA_LIST_FOREACH(panel->items, item, it)
     {
        if (it->view == view)
          return it;
     }

   return NULL;
}

Eina_Bool
edi_mainview_panel_item_contains(Edi_Mainview_Panel *panel, Edi_Mainview_Item *item)
{
   Eina_List *it;
   Edi_Mainview_Item *panel_item;

   EINA_LIST_FOREACH(panel->items, it, panel_item)
     {
        if (panel_item == item)
          return EINA_TRUE;
     }

   return EINA_FALSE;
}

void
edi_mainview_panel_item_prev(Edi_Mainview_Panel *panel)
{
   Eina_List *item;
   Edi_Mainview_Item *it, *first, *prev = NULL;

   first = (Edi_Mainview_Item *)eina_list_nth(panel->items, 0);
   if (first == panel->current)
     {
        prev = eina_list_nth(panel->items, eina_list_count(panel->items)-1);
        if (prev)
          edi_mainview_panel_item_select(panel, prev);
        return;
     }

   EINA_LIST_FOREACH(panel->items, item, it)
     {
        if (it && it == panel->current)
          {
             if (prev)
               edi_mainview_panel_item_select(panel, prev);
             return;
          }

        prev = it;
     }
}

void
edi_mainview_panel_item_next(Edi_Mainview_Panel *panel)
{
   Eina_List *item;
   Edi_Mainview_Item *it, *last, *next;
   Eina_Bool open_next = EINA_FALSE;

   last = eina_list_nth(panel->items, eina_list_count(panel->items)-1);

   if (last == panel->current)
     {
        next = eina_list_nth(panel->items, 0);
        if (next)
          edi_mainview_panel_item_select(panel, next);
        return;
     }

   EINA_LIST_FOREACH(panel->items, item, it)
     {
        if (it && open_next)
          {
             edi_mainview_panel_item_select(panel, it);
             return;
          }

        if (it && it == panel->current)
          open_next = EINA_TRUE;
     }
}

void
edi_mainview_panel_tab_select(Edi_Mainview_Panel *panel, unsigned int id)
{
   Eina_List *item;
   Edi_Mainview_Item *it;
   unsigned int i = 0;

   EINA_LIST_FOREACH(panel->items, item, it)
     {
        if (!it->win)
          i++;
        if (i == id)
          edi_mainview_panel_item_select(panel, it);
     }
}

static void
_edi_mainview_panel_show(Edi_Mainview_Panel *panel, Evas_Object *view)
{
   if (panel->current)
     {
        elm_box_unpack(panel->content, panel->current->view);
        evas_object_hide(panel->current->view);
     }
   else
     {
        elm_box_unpack(panel->content, panel->welcome);
        evas_object_hide(panel->welcome);
     }

   panel->current = _edi_mainview_panel_item_for_view_get(panel, view);
   elm_box_pack_end(panel->content, view);
   evas_object_show(view);
}

static void
_content_load(Edi_Mainview_Item *item)
{
   Edi_Content_Provider *provider;
   Evas_Object *child;

   provider = edi_content_provider_for_id_get(item->editortype);
   if (!provider)
     {
        ERR("No content provider found for type %s", item->editortype);
        return;
     }
   child = provider->content_ui_add(item->container, item);

   evas_object_size_hint_weight_set(child, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(child, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(item->container, child);
   evas_object_show(child);

   item->loaded = EINA_TRUE;
}

void
edi_mainview_panel_item_close(Edi_Mainview_Panel *panel, Edi_Mainview_Item *item)
{
   int item_index;
   Eina_Bool current;

   if (!item)
     return;

   current = (item == panel->current);
   item_index = eina_list_data_idx(panel->items, item);

   if (item->view)
     evas_object_del(item->view);
   elm_box_unpack(panel->tabs, item->tab);
   evas_object_del(item->tab);
   panel->items = eina_list_remove(panel->items, item);

   _edi_project_config_tab_remove(item->path, EINA_FALSE,
                                  edi_mainview_panel_index_get(panel));
   eina_stringshare_del(item->path);
   free(item);

   if (!current)
     return;

   if (eina_list_count(panel->items) == 0)
     {
        _edi_mainview_panel_show(panel, panel->welcome);
        return;
     }

   if (item_index)
     item = eina_list_nth(panel->items, item_index - 1);
   else
     item = eina_list_nth(panel->items, item_index);

   edi_mainview_panel_item_select(panel, item);
   _edi_mainview_panel_current_tab_show(panel);
}

void
edi_mainview_panel_item_select(Edi_Mainview_Panel *panel, Edi_Mainview_Item *item)
{
   Eina_List *list;
   Edi_Mainview_Item *it;
   Evas_Coord tabw, region_x = 0, w, total_w = 0;

   if (item->win)
     {
        elm_win_raise(item->win);
     }
   else
     {
        EINA_LIST_FOREACH(panel->items, list, it)
          {
             elm_object_signal_emit(it->tab, "mouse,up,1", "base");
             evas_object_geometry_get(it->tab, NULL, NULL, &w, NULL);
             if (item == it) region_x = total_w;
             total_w += w;
          }

        if (!item->loaded)
          _content_load(item);

        _edi_mainview_panel_show(panel, item->view);
        elm_object_signal_emit(item->tab, "mouse,down,1", "base");

        evas_object_geometry_get(item->tab, NULL, NULL, &tabw, NULL);
        elm_scroller_region_bring_in(efl_parent_get(panel->tabs), region_x, 0, tabw, 0);

        _edi_project_config_tab_current_set(edi_mainview_panel_index_get(panel),
                                            edi_mainview_panel_item_current_tab_get(panel));
        _edi_project_config_save_no_notify();
     }

   edi_mainview_panel_focus(panel);
   ecore_event_add(EDI_EVENT_TAB_CHANGED, NULL, NULL, NULL);
}

static void
_promote(void *data, Evas_Object *obj EINA_UNUSED,
         const char *emission EINA_UNUSED, const char *source)
{
   Edi_Mainview_Panel *panel;
   Edi_Mainview_Item *item = (Edi_Mainview_Item *) data;

   // ignore if we clicked the delete part of the button
   if (!strcmp(source, "del"))
     return;

   panel = edi_mainview_panel_for_item_get(item);

   _edi_mainview_panel_cache_set();

   _edi_mainview_panel_current_tab_hide(panel);

   edi_mainview_panel_item_select(panel, item);

   _edi_mainview_panel_current_tab_show(panel);

   edi_filepanel_select_path(item->path);
}

static void
_closetab(void *data, Evas_Object *obj EINA_UNUSED,
          const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Edi_Mainview_Panel *panel;
   Edi_Mainview_Item *item;
   Edi_Mainview_State *last;
   Edi_Editor *editor;
   int index;

   item = (Edi_Mainview_Item *) data;
   panel = edi_mainview_panel_for_item_get(item);

   editor = (Edi_Editor *) evas_object_data_get(panel->current->view, "editor");
   if (editor && eina_list_count(editor->split_views))
     {
        Elm_Code *code;
        const char *path;
        Elm_Code_Widget *widget = eina_list_nth(editor->split_views, 0);
        elm_box_unpack(panel->current->container, widget);
        code = elm_code_widget_code_get(editor->entry);
        path = elm_code_file_path_get(code->file);

        editor->split_views = eina_list_remove(editor->split_views, widget);
         _edi_project_config_tab_split_view_count_set(path, edi_mainview_panel_id(panel), eina_list_count(editor->split_views));

        evas_object_del(widget);

        return;
     }

   edi_mainview_panel_item_close(panel, item);
   if (eina_list_count(panel->items)== 0 && edi_mainview_panel_count() > 1)
     {
        edi_mainview_panel_remove(panel);
        index = edi_mainview_panel_count() - 1;
        panel = edi_mainview_panel_by_index(index);
     }

   edi_mainview_panel_focus(panel);

   /* When closing tabs keep current tab */
   last = _edi_mainview_panel_cache_get();
   if (last && last->panel == panel && last->path)
     {
        item  = _get_item_for_path(panel, last->path);
        if (item)
          {
             _edi_mainview_panel_current_tab_hide(panel);
             edi_mainview_panel_item_select(panel, item);
          }
     }

   if (eina_list_count(panel->items))
     _edi_mainview_panel_current_tab_show(panel);
}

static Evas_Object *
_edi_mainview_panel_content_create(Edi_Mainview_Item *item, Evas_Object *parent)
{
   Evas_Object *container;

   container = elm_box_add(parent);
   evas_object_size_hint_weight_set(container, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(container, EVAS_HINT_FILL, EVAS_HINT_FILL);

   item->loaded = EINA_FALSE;
   item->container = container;

   return container;
}

static int
_font_width_get(Evas_Object *parent, const char *text)
{
   int w = 0;

   Evas_Object *textblock = evas_object_text_add(parent);
   evas_object_size_hint_weight_set(textblock, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(textblock, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(textblock);

   evas_object_text_font_set(textblock, "sans", 12);
   evas_object_text_text_set(textblock, text);

   evas_object_geometry_get(textblock, NULL, NULL, &w, NULL);

   evas_object_del(textblock);

   if (w < 120)
     w = 120;

   return w;
}

static void
_edi_mainview_panel_item_tab_add(Edi_Mainview_Panel *panel, Edi_Path_Options *options, const char *mime)
{
   Evas_Object *content, *tab;//, *icon;
   Edi_Mainview_Item *item;
   Edi_Editor *editor;
   Elm_Code *code;
   int h, width;
   const char *path;

   if (!panel) return;

   int id = edi_mainview_panel_id(panel);

   if (panel == edi_mainview_panel_current_get())
     {
        if (eina_list_count(panel->items))
          _edi_mainview_panel_current_tab_hide(panel);
     }

   item = edi_mainview_item_add(options, mime, NULL, NULL);
   content = _edi_mainview_panel_content_create(item, panel->content);
   item->view = content;
   panel->items = eina_list_append(panel->items, item);
   _edi_mainview_panel_show(panel, content);

   evas_object_geometry_get(panel->tabs, NULL, NULL, NULL, &h);

   tab = elm_button_add(panel->tabs);
   evas_object_size_hint_weight_set(tab, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(tab, 0.0, EVAS_HINT_FILL);
   elm_object_focus_allow_set(tab, EINA_FALSE);

   path = strstr(item->path, edi_project_get());
   if (path)
     path += 1 + strlen(edi_project_get());
   else
     path = item->path;

   elm_object_tooltip_text_set(tab, path);
   elm_object_tooltip_window_mode_set(tab, EINA_TRUE);

   elm_layout_theme_set(tab, "multibuttonentry", "btn", "default");
   elm_object_part_text_set(tab, "elm.btn.text", eina_slstr_printf("<style align=left> %s</>", ecore_file_file_get(options->path)));
/*
   icon = elm_icon_add(tab);
   elm_icon_standard_set(icon, provider->icon);
   elm_object_part_content_set(tab, "icon", icon);
*/
   width = _font_width_get(tab, ecore_file_file_get(options->path));

   elm_layout_signal_callback_add(tab, "mouse,clicked,1", "*", _promote, item);
   elm_layout_signal_callback_add(tab, "elm,deleted", "elm", _closetab, item);
   evas_object_size_hint_min_set(tab, width * elm_config_scale_get(), h);

   elm_box_pack_end(panel->tabs, tab);
   evas_object_show(tab);
   elm_box_recalculate(panel->tabs);
   item->tab = tab;

   if (!options->background)
     edi_mainview_panel_item_select(panel, item);

   // Set focus on the newly opening window so that one can just start typing
   editor = (Edi_Editor *)evas_object_data_get(content, "editor");
   if (editor)
     {
        evas_object_show(editor->entry);
        elm_object_focus_set(editor->entry, EINA_TRUE);
        code = elm_code_widget_code_get(editor->entry);
        editor->save_time = ecore_file_mod_time(elm_code_file_path_get(code->file));
        editor->modified = EINA_FALSE;
     }

   if (options->line)
     {
        if (options->character > 1)
          edi_mainview_panel_goto_position(panel, options->line, options->character);
        else
          edi_mainview_panel_goto(panel, options->line);
     }

   _edi_project_config_tab_add(options->path, mime?mime:options->type, EINA_FALSE, id);
}

static void
_edi_popup_cancel_cb(void *data, Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   evas_object_del((Evas_Object *) data);
}

static void
_edi_mainview_panel_mime_content_safe_popup(void)
{
   Evas_Object *popup, *box, *table, *label, *button, *icon, *sep;

   popup = elm_popup_add(_main_win);
   elm_object_part_text_set(popup, "title,text",
                                   _("Unrecognized file type"));

   table = elm_table_add(popup);
   icon = elm_icon_add(table);
   elm_icon_standard_set(icon, "dialog-warning");
   evas_object_size_hint_min_set(icon, 48 * elm_config_scale_get(), 48 * elm_config_scale_get());
   evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(icon);
   elm_table_pack(table, icon, 0, 0, 1, 1);

   box = elm_box_add(popup);
   sep = elm_separator_add(box);
   elm_separator_horizontal_set(sep, EINA_TRUE);
   evas_object_show(sep);
   elm_box_pack_end(box, sep);
   label = elm_label_add(popup);
   elm_object_text_set(label, _("To force open, select this file in the file browser, <br>and use \"open as\" menu options."));
   evas_object_show(label);
   elm_table_pack(table, label, 1, 0, 1, 1);
   evas_object_show(table);
   elm_box_pack_end(box, table);

   sep = elm_separator_add(box);
   elm_separator_horizontal_set(sep, EINA_TRUE);
   evas_object_show(sep);
   elm_box_pack_end(box, sep);
   evas_object_show(box);

   elm_object_content_set(popup, box);

   button = elm_button_add(popup);
   elm_object_text_set(button, _("OK"));
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked", _edi_popup_cancel_cb, popup);

   _edi_mainview_panel_current_tab_show(edi_mainview_panel_current_get());
   evas_object_show(popup);
}

void
edi_mainview_panel_close(Edi_Mainview_Panel *panel)
{
   Edi_Mainview_Item *item;

   item = edi_mainview_panel_item_current_get(panel);

   edi_mainview_panel_item_close(panel, item);
}

void
edi_mainview_panel_save(Edi_Mainview_Panel *panel)
{
   Edi_Editor *editor;

   if (!panel || !panel->current)
     return;

   editor = (Edi_Editor *)evas_object_data_get(panel->current->view, "editor");
   if (!editor)
     return;

   edi_editor_save(editor);
}

void
edi_mainview_panel_undo(Edi_Mainview_Panel *panel)
{
   Edi_Editor *editor;

   if (!panel || !panel->current)
     return;

   editor = (Edi_Editor *)evas_object_data_get(panel->current->view, "editor");

   if (editor)
     elm_code_widget_undo(editor->entry);
}

Eina_Bool
edi_mainview_panel_can_undo(Edi_Mainview_Panel *panel)
{
   Edi_Editor *editor;

   if (!panel || !panel->current)
     return EINA_FALSE;

   editor = (Edi_Editor *)evas_object_data_get(panel->current->view, "editor");

   if (!editor)
     return EINA_FALSE;

   return elm_code_widget_can_undo_get(editor->entry);
}

void
edi_mainview_panel_redo(Edi_Mainview_Panel *panel)
{
   Edi_Editor *editor;

   if (!panel || !panel->current)
     return;

   editor = (Edi_Editor *)evas_object_data_get(panel->current->view, "editor");

   if (editor)
     elm_code_widget_redo(editor->entry);
}

Eina_Bool
edi_mainview_panel_can_redo(Edi_Mainview_Panel *panel)
{
   Edi_Editor *editor;

   if (!panel || !panel->current)
     return EINA_FALSE;

   editor = (Edi_Editor *)evas_object_data_get(panel->current->view, "editor");

   if (!editor)
     return EINA_FALSE;

   return elm_code_widget_can_redo_get(editor->entry);
}

Eina_Bool
edi_mainview_panel_modified(Edi_Mainview_Panel *panel)
{
   Edi_Editor *editor;

   if (!panel || !panel->current)
     return EINA_FALSE;

   editor = (Edi_Editor *)evas_object_data_get(panel->current->view, "editor");
   if (!editor)
     return EINA_FALSE;

   return editor->modified;
}

void
edi_mainview_panel_cut(Edi_Mainview_Panel *panel)
{
   Edi_Editor *editor;

   if (!panel || !panel->current)
     return;

   editor = (Edi_Editor *)evas_object_data_get(panel->current->view, "editor");

   if (editor)
     elm_code_widget_selection_cut(editor->entry);
}

void
edi_mainview_panel_copy(Edi_Mainview_Panel *panel)
{
   Edi_Editor *editor;

   if (!panel || !panel->current)
     return;

   editor = (Edi_Editor *)evas_object_data_get(panel->current->view, "editor");

   if (editor)
     elm_code_widget_selection_copy(editor->entry);
}

void
edi_mainview_panel_paste(Edi_Mainview_Panel *panel)
{
   Edi_Editor *editor;

   if (!panel || !panel->current)
     return;

   editor = (Edi_Editor *)evas_object_data_get(panel->current->view, "editor");

   if (editor)
     elm_code_widget_selection_paste(editor->entry);
}

void
edi_mainview_panel_search(Edi_Mainview_Panel *panel)
{
   Edi_Editor *editor;

   if (edi_mainview_is_empty()) return;

   if (!panel || !panel->current)
     return;

   editor = (Edi_Editor *)evas_object_data_get(panel->current->view, "editor");

   if (editor)
     edi_editor_search(editor);
}

void
edi_mainview_panel_goto(Edi_Mainview_Panel *panel, unsigned int number)
{
   if (edi_mainview_is_empty()) return;

   if (!panel || !panel->current)
     return;

   edi_mainview_panel_goto_position(panel, number, 1);
}

void
edi_mainview_panel_goto_position(Edi_Mainview_Panel *panel, unsigned int row, unsigned int col)
{
   Edi_Editor *editor;

   if (!panel || !panel->current)
     return;

   editor = (Edi_Editor *)evas_object_data_get(panel->current->view, "editor");
   if (!editor || row <= 0 || col <= 0)
     return;

   elm_code_widget_cursor_position_set(editor->entry, row, col);
   elm_object_focus_set(editor->entry, EINA_TRUE);
}

static void
_edi_mainview_panel_goto_popup_go_cb(void *data,
                             Evas_Object *obj EINA_UNUSED,
                             void *event_info EINA_UNUSED)
{
   int number;

   number = atoi(elm_entry_entry_get((Evas_Object *) data));
   edi_mainview_goto(number);

   evas_object_del(_edi_mainview_goto_popup);
}

static void
_edi_mainview_panel_goto_popup_key_up_cb(void *data, Evas *e EINA_UNUSED,
                                   Evas_Object *obj, void *event_info)
{
   Edi_Mainview_Panel *panel = data;
   Evas_Event_Key_Up *ev = (Evas_Event_Key_Up *)event_info;
   const char *str;

   str = elm_object_text_get(obj);

   if (strlen(str) && (!strcmp(ev->key, "KP_Enter") || !strcmp(ev->key, "Return")))
     _edi_mainview_panel_goto_popup_go_cb(obj, NULL, NULL);
}

void
edi_mainview_panel_goto_popup_show(Edi_Mainview_Panel *panel)
{
   Evas_Object *popup, *box, *input, *sep, *button;
   Edi_Editor *editor;

   if (edi_mainview_is_empty())
     return;

   if (!edi_mainview_panel_item_count(edi_mainview_panel_current_get()))
     return;

   editor = evas_object_data_get(panel->current->view, "editor");
   if (!editor)
     return;

   popup = elm_popup_add(editor->entry);

   _edi_mainview_goto_popup = popup;
   elm_object_part_text_set(popup, "title,text",
                            _("Enter line number"));

   box = elm_box_add(popup);
   elm_box_horizontal_set(box, EINA_FALSE);
   elm_object_content_set(popup, box);

   sep = elm_separator_add(popup);
   elm_separator_horizontal_set(sep, EINA_TRUE);
   evas_object_show(sep);
   elm_box_pack_end(box, sep);

   input = elm_entry_add(box);
   elm_entry_single_line_set(input, EINA_TRUE);
   elm_entry_scrollable_set(input, EINA_TRUE);
   evas_object_event_callback_add(input, EVAS_CALLBACK_KEY_UP, _edi_mainview_panel_goto_popup_key_up_cb, panel);
   evas_object_size_hint_weight_set(input, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_align_set(input, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(input);
   elm_box_pack_end(box, input);

   sep = elm_separator_add(popup);
   elm_separator_horizontal_set(sep, EINA_TRUE);
   evas_object_show(sep);
   elm_box_pack_end(box, sep);

   button = elm_button_add(popup);
   elm_object_text_set(button, _("Cancel"));
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked",
                                       _edi_popup_cancel_cb, popup);

   button = elm_button_add(popup);
   elm_object_text_set(button, _("Go"));
   elm_object_part_content_set(popup, "button2", button);
   evas_object_smart_callback_add(button, "clicked",
                                       _edi_mainview_panel_goto_popup_go_cb, input);

   evas_object_show(popup);
   elm_object_focus_set(input, EINA_TRUE);
}

static void
_edi_mainview_panel_tab_stat_done(void *data, Eio_File *handler EINA_UNUSED, const Eina_Stat *stat)
{
   Edi_Mainview_Panel *panel;
   Edi_Path_Options *options;
   Edi_Content_Provider *provider;
   const char *mime;

   options = data;
   if (!S_ISREG(stat->mode))
     return;

   mime = efreet_mime_type_get(options->path);
   provider = edi_content_provider_for_mime_get(mime);
   if (!provider)
     {
        _edi_mainview_panel_mime_content_safe_popup();
        return;
     }

   options->type = provider->id;
   panel = edi_mainview_panel_current_get();
   _edi_mainview_panel_item_tab_add(panel, options, mime);
}

void
edi_mainview_panel_open_path(Edi_Mainview_Panel *panel, const char *path)
{
   Edi_Path_Options *options;

   options = edi_path_options_create(path);
   edi_mainview_panel_open(panel, options);
}

void
edi_mainview_panel_open(Edi_Mainview_Panel *panel, Edi_Path_Options *options)
{
   Edi_Mainview_Item *it;
   Edi_Mainview_Panel *current;
   Edi_Editor *editor;
   int i;

   current = panel;

   for (i = 0; i < edi_mainview_panel_count(); i++)
     {
        panel = edi_mainview_panel_by_index(i);

        it = _get_item_for_path(panel, options->path);
        if (it)
          {
             _edi_mainview_panel_current_tab_hide(panel);
             edi_mainview_panel_focus(panel);

             editor = evas_object_data_get(panel->current->view, "editor");
             if (editor)
               elm_object_focus_set(editor->entry, EINA_FALSE);

             edi_mainview_panel_item_select(panel, it);

             _edi_mainview_panel_current_tab_show(panel);

             if (options->line)
               {
                  if (options->character > 1)
                    edi_mainview_goto_position(options->line, options->character);
                  else
                    edi_mainview_goto(options->line);
               }
             return;
          }
   }

   panel = current;

   edi_mainview_panel_focus(panel);
   if (options->type == NULL)
     {
        eio_file_direct_stat(options->path, _edi_mainview_panel_tab_stat_done, dummy, options);
     }
   else if (!edi_content_provider_for_id_get(options->type))
     {
        const char *mime = options->type;
        Edi_Content_Provider *provider = edi_content_provider_for_mime_get(mime);

        if (provider)
          options->type = provider->id;
        else
          options->type = NULL;

        _edi_mainview_panel_item_tab_add(panel, options, mime);
     }
   else
     {
        _edi_mainview_panel_item_tab_add(panel, options, NULL);
     }
}

void
edi_mainview_panel_close_all(Edi_Mainview_Panel *panel)
{
   Eina_List *item;
   Edi_Mainview_Item *it;

   EINA_LIST_FOREACH(panel->items, item, it)
     edi_mainview_panel_item_close(panel, it);
}

void
edi_mainview_panel_refresh_all(Edi_Mainview_Panel *panel)
{
   Eina_List *item, *tabs = NULL;
   Edi_Mainview_Item *it;
   Edi_Path_Options *options;

   EINA_LIST_FOREACH(panel->items, item, it)
     {
        options = edi_path_options_create(it->path);
        options->type = eina_stringshare_add(it->editortype);
        options->background = it != panel->current;

        tabs = eina_list_append(tabs, options);
     }

   edi_mainview_panel_close_all(panel);

   EINA_LIST_FOREACH(tabs, item, options)
     {
        if (!ecore_file_exists(options->path))
          continue;

        edi_mainview_panel_open(panel, options);
     }

   EINA_LIST_FREE(tabs, options)
     free(options);
}

void
edi_mainview_panel_item_close_path(Edi_Mainview_Panel *panel, const char *path)
{
   Eina_List *item;
   int panel_id, it_index;
   Edi_Mainview_Item *it;

   if (!panel) return;

   EINA_LIST_FOREACH(panel->items, item, it)
     {
        if (it && !strcmp(it->path, path))
          {
             it_index = eina_list_data_idx(panel->items, it);
             edi_mainview_panel_item_close(panel, it);
             if (eina_list_count(panel->items))
               {
                  if (it_index)
                    it = eina_list_nth(panel->items, it_index - 1);
                  else
                    it = eina_list_nth(panel->items, it_index);

                   edi_mainview_panel_item_select(panel, it);
               }

             _edi_mainview_panel_current_tab_show(panel);

             if (edi_mainview_panel_item_count(panel) == 0 &&
                 edi_mainview_panel_count() > 1)
               {
                  edi_mainview_panel_remove(panel);
                  panel_id = edi_mainview_panel_count() -1;
                  panel = edi_mainview_panel_by_index(panel_id);
                  _edi_mainview_panel_current_tab_show(panel);
               }
          }
     }
}

static void
_edi_mainview_panel_next_clicked_cb(void *data,
                                    Evas_Object *obj EINA_UNUSED,
                                    void *event_info EINA_UNUSED)
{
   int x, y, w, h;
   Evas_Object *scroller = data;

   elm_scroller_region_get(scroller, &x, &y, &w, &h);
   x += w * 0.8;
   elm_scroller_region_bring_in(scroller, x, y, w, h);
}

static void
_edi_mainview_panel_prev_clicked_cb(void *data,
                                    Evas_Object *obj EINA_UNUSED,
                                    void *event_info EINA_UNUSED)
{
   int x, y, w, h;
   Evas_Object *scroller = data;

   elm_scroller_region_get(scroller, &x, &y, &w, &h);
   x -= w * 0.8;
   elm_scroller_region_bring_in(scroller, x, y, w, h);
}

static void
_edi_mainview_panel_welcome_focused_cb(void *data,
                                    Evas_Object *obj EINA_UNUSED,
                                    void *event_info EINA_UNUSED)
{
    edi_mainview_panel_focus((Edi_Mainview_Panel *) data);
}

void
edi_mainview_panel_free(Edi_Mainview_Panel *panel)
{
   evas_object_del(panel->welcome);
   evas_object_del(panel->content);
   evas_object_del(panel->tabs);
   evas_object_del(panel->scroll);
   evas_object_del(panel->box);
   if (panel->sep)
     evas_object_del(panel->sep);

   free(panel);
}

Edi_Mainview_Panel *
edi_mainview_panel_add(Evas_Object *parent)
{
   Edi_Mainview_Panel *panel;
   Evas_Object *box, *sep, *scroll, *txt, *nf, *tabs, *tab, *bg, *pad, *scr, *tb;
   Evas_Object *next, *prev, *ico_next, *ico_prev;
   _main_win = parent;

   panel = calloc(1, sizeof(*panel));

   if (edi_mainview_panel_count() > 0)
     {
        sep = elm_separator_add(parent);
        elm_separator_horizontal_set(sep, EINA_FALSE);
        evas_object_show(sep);
        panel->sep = sep;
        elm_box_pack_end(parent, sep);
     }

   box = elm_box_add(parent);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);
   elm_box_pack_end(parent, box);

   tabs = elm_table_add(box);
   evas_object_size_hint_weight_set(tabs, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(tabs, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(tabs);
   elm_box_pack_end(box, tabs);

   bg = elm_box_add(tabs);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bg, 0.0, EVAS_HINT_FILL);
   evas_object_show(bg);
   elm_table_pack(tabs, bg, 0, 0, 1, 1);

   scr = elm_scroller_add(parent);
   evas_object_size_hint_weight_set(scr, EVAS_HINT_EXPAND, 0.04);
   evas_object_size_hint_align_set(scr, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_min_set(scr, 0, 100 * elm_config_scale_get());
   elm_scroller_bounce_set(scr, EINA_FALSE, EINA_FALSE);
   elm_scroller_policy_set(scr, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
   elm_table_pack(tabs, scr, 0, 0, 1, 1);
   evas_object_show(scr);

   prev = elm_button_add(tabs);
   evas_object_size_hint_weight_set(prev, 0, 0);
   evas_object_size_hint_align_set(prev, 0, EVAS_HINT_FILL);
   elm_table_pack(tabs, prev, 1, 0, 1, 1);
   evas_object_smart_callback_add(prev, "clicked",
                                       _edi_mainview_panel_prev_clicked_cb, scr);
   ico_prev = elm_icon_add(prev);
   elm_icon_standard_set(ico_prev, "go-previous");
   elm_object_part_content_set(prev, "icon", ico_prev);
   evas_object_show(prev);

   next = elm_button_add(tabs);
   evas_object_size_hint_weight_set(next, 0, 0);
   evas_object_size_hint_align_set(next, 0, EVAS_HINT_FILL);
   elm_table_pack(tabs, next, 2, 0, 1, 1);
   evas_object_smart_callback_add(next, "clicked",
                                       _edi_mainview_panel_next_clicked_cb, scr);

   ico_next = elm_icon_add(next);
   elm_icon_standard_set(ico_next, "go-next");
   elm_object_part_content_set(next, "icon", ico_next);
   evas_object_show(next);

   tab = elm_button_add(tabs);
   evas_object_size_hint_weight_set(tab, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(tab, 0.0, EVAS_HINT_FILL);
   elm_layout_theme_set(tab, "multibuttonentry", "btn", "default");
   elm_object_part_text_set(tab, "elm.btn.text", "hg");
   elm_box_pack_end(bg, tab);

   pad = elm_box_add(tabs);
   evas_object_size_hint_weight_set(pad, 0.0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(pad, 0.0, EVAS_HINT_FILL);
   evas_object_size_hint_min_set(pad, 0, 1.5 * elm_config_scale_get());
   elm_box_pack_end(bg, pad);

   tb = elm_box_add(scr);
   evas_object_size_hint_weight_set(tb, 0.0, 0.0);
   evas_object_size_hint_align_set(tb, 0.0, EVAS_HINT_FILL);
   elm_box_horizontal_set(tb, EINA_TRUE);
   elm_object_content_set(scr, tb);
   evas_object_show(tb);
   panel->tabs = tb;
   panel->box = box;
   panel->tb = tb;

   nf = elm_box_add(parent);
   evas_object_size_hint_weight_set(nf, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(nf, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, nf);
   evas_object_show(nf);
   panel->content = nf;

   scroll = elm_scroller_add(parent);
   evas_object_size_hint_weight_set(scroll, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(scroll, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(scroll);
   panel->scroll = scroll;

   txt = elm_label_add(scroll);
   elm_object_text_set(txt, "<hilight>Welcome</hilight><br><br>Click on any file to edit.");
   evas_object_size_hint_weight_set(txt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(txt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(txt);
   elm_object_content_set(scroll, txt);

   elm_object_focus_allow_set(txt, EINA_TRUE);
   evas_object_smart_callback_add(txt, "focused",
                                       _edi_mainview_panel_welcome_focused_cb, panel);

   panel->welcome = scroll;
   _edi_mainview_panel_show(panel, scroll);
   return panel;
}

