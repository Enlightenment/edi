#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <libgen.h>

#include <Eina.h>
#include <Eio.h>
#include <Elementary.h>

#include "mainview/edi_mainview_item.h"
#include "mainview/edi_mainview.h"

#include "editor/edi_editor.h"
#include "edi_content_provider.h"
#include "../edi_searchpanel.h"

#include "edi_private.h"
#include "edi_config.h"

static Evas_Object *_content_frame, *_current_view, *tb, *_main_win, *_welcome_panel, *_tab_scroller;
static Evas_Object *_edi_mainview_goto_popup;
static Evas_Object *_edi_mainview_search_project_popup;

static Eina_List *_edi_mainview_items = NULL;

static void
dummy()
{}

Edi_Mainview_Item *
edi_mainview_item_current_get()
{
   Eina_List *item;
   Edi_Mainview_Item *it;

   EINA_LIST_FOREACH(_edi_mainview_items, item, it)
     {
        if (it && it->view == _current_view)
          {
             return it;
          }
     }

   return NULL;
}

static void
_edi_mainview_view_show(Evas_Object *view)
{
   elm_box_unpack(_content_frame, _current_view);
   evas_object_hide(_current_view);

   _current_view = view;
   elm_box_pack_end(_content_frame, view);
   evas_object_show(view);
}

void
edi_mainview_item_prev()
{
   Eina_List *item;
   Edi_Mainview_Item *it, *first, *prev = NULL;

   first = (Edi_Mainview_Item *)eina_list_nth(_edi_mainview_items, 0);
   if (first->view == _current_view)
     {
        prev = eina_list_nth(_edi_mainview_items, eina_list_count(_edi_mainview_items)-1);
        edi_mainview_item_select(prev);
        return;
     }

   EINA_LIST_FOREACH(_edi_mainview_items, item, it)
     {
        if (it && it->view == _current_view)
          {
             if (prev)
               edi_mainview_item_select(prev);
             return;
          }

        prev = it;
     }
}

void
edi_mainview_item_next()
{
   Eina_List *item;
   Edi_Mainview_Item *it, *last, *next;
   Eina_Bool open_next = EINA_FALSE;

   last = eina_list_nth(_edi_mainview_items, eina_list_count(_edi_mainview_items)-1);

   if (last->view == _current_view)
     {
        next = eina_list_nth(_edi_mainview_items, 0);
        edi_mainview_item_select(next);
        return;
     }

   EINA_LIST_FOREACH(_edi_mainview_items, item, it)
     {

        if (it && open_next)
          {
             edi_mainview_item_select(it);
             return;
          }

        if (it && it->view == _current_view)
          open_next = EINA_TRUE;
     }
}

void
edi_mainview_item_select(Edi_Mainview_Item *item)
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
        EINA_LIST_FOREACH(_edi_mainview_items, list, it)
          {
             elm_object_signal_emit(it->tab, "mouse,up,1", "base");
             evas_object_geometry_get(it->tab, NULL, NULL, &w, NULL);
             if (item == it) region_x = total_w;
             total_w += w;
          }

        _edi_mainview_view_show(item->view);
        elm_object_signal_emit(item->tab, "mouse,down,1", "base");

        evas_object_geometry_get(item->tab, NULL, NULL, &tabw, NULL);
        elm_scroller_region_bring_in(_tab_scroller, region_x, 0, tabw, 0);
     }

   ecore_event_add(EDI_EVENT_TAB_CHANGED, NULL, NULL, NULL);
}

static void
_edi_mainview_item_close(Edi_Mainview_Item *item)
{
   if (!item)
     return;

   edi_mainview_item_prev();
   evas_object_del(item->view);
   elm_box_unpack(tb, item->tab);
   evas_object_del(item->tab);
   _edi_mainview_items = eina_list_remove(_edi_mainview_items, item);

   _edi_project_config_tab_remove(item->path);
   eina_stringshare_del(item->path);
   free(item);

   if (eina_list_count(_edi_mainview_items) == 0)
     _edi_mainview_view_show(_welcome_panel);
}

static void
_promote(void *data, Evas_Object *obj EINA_UNUSED,
         const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   edi_mainview_item_select((Edi_Mainview_Item *)data);
}

static void
_closetab(void *data, Evas_Object *obj EINA_UNUSED,
          const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   _edi_mainview_item_close(data);
}

static Edi_Mainview_Item *
_get_item_for_path(const char *path)
{
   Eina_List *item;
   Edi_Mainview_Item *it;

   EINA_LIST_FOREACH(_edi_mainview_items, item, it)
     {
        if (it && !strcmp(it->path, path))
          return it;
     }
   return NULL;
}

static Edi_Mainview_Item *
_edi_mainview_item_add(Edi_Path_Options *path, const char *mime, Elm_Object_Item *tab, Elm_Object_Item *view,
                       Evas_Object *win)
{
   Edi_Mainview_Item *item;

   item = malloc(sizeof(Edi_Mainview_Item));
   item->path = eina_stringshare_add(path->path);
   item->editortype = path->type;
   item->mimetype = mime;
   item->tab = tab;
   item->view = view;
   item->win = win;

   _edi_mainview_items = eina_list_append(_edi_mainview_items, item);

   return item;
}

static Evas_Object *
_edi_mainview_content_create(Edi_Mainview_Item *item, Evas_Object *parent)
{
   Edi_Content_Provider *provider;

   provider = edi_content_provider_for_id_get(item->editortype);
   if (!provider)
     return NULL;

   return provider->content_ui_add(parent, item);
}

static void
_edi_mainview_item_tab_add(Edi_Path_Options *options, const char *mime)
{
   Evas_Object *content, *tab;//, *icon;
   Edi_Mainview_Item *item;
   Edi_Editor *editor;
//   Edi_Content_Provider *provider;

   item = _edi_mainview_item_add(options, mime, NULL, NULL, NULL);
//   provider = edi_content_provider_for_id_get(item->editortype);
   content = _edi_mainview_content_create(item, _content_frame);

   _edi_mainview_view_show(content);
   item->view = content;

   tab = elm_button_add(tb);
   evas_object_size_hint_weight_set(tab, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(tab, 0.0, EVAS_HINT_FILL);

   elm_layout_theme_set(tab, "multibuttonentry", "btn", "default");
   elm_object_part_text_set(tab, "elm.btn.text", basename((char*)options->path));
/*
   icon = elm_icon_add(tab);
   elm_icon_standard_set(icon, provider->icon);
   elm_object_part_content_set(tab, "icon", icon);
*/
   elm_layout_signal_callback_add(tab, "mouse,clicked,1", "*", _promote, item);
   elm_layout_signal_callback_add(tab, "elm,deleted", "elm", _closetab, item);

   elm_box_pack_end(tb, tab);
   evas_object_show(tab);
   elm_box_recalculate(tb);
   item->tab = tab;
   edi_mainview_item_select(item);

   // Set focus on the newly opening window so that one can just start typing
   editor = (Edi_Editor *)evas_object_data_get(content, "editor");
   if (editor)
     elm_object_focus_set(editor->entry, EINA_TRUE);

   if (options->line)
     edi_mainview_goto(options->line);

   _edi_project_config_tab_add(options->path, EINA_FALSE);
}

static void
_edi_mainview_win_exit(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Edi_Mainview_Item *it;

   evas_object_hide(obj);

   it = evas_object_data_get(obj, "edi_mainview_item");
   _edi_mainview_items = eina_list_remove(_edi_mainview_items, it);

   _edi_project_config_tab_remove(it->path);
   eina_stringshare_del(it->path);

   if (edi_noproject())
     edi_close();
   free(it);
}

static char *
_edi_mainview_win_title_get(const char *path)
{
   char *winname, *filename;

   filename = basename((char*)path);
   winname = malloc((8 + strlen(filename)) * sizeof(char));
   snprintf(winname, 8 + strlen(filename), "Edi :: %s", filename);

   return winname;
}

static void
_edi_mainview_item_win_add(Edi_Path_Options *options, const char *mime)
{
   Evas_Object *win, *content;
   Edi_Mainview_Item *item;
   Edi_Editor *editor;

   win = elm_win_util_standard_add("mainview", _edi_mainview_win_title_get(options->path));
   if (!win) return;

   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   evas_object_smart_callback_add(win, "delete,request", _edi_mainview_win_exit, NULL);
   item = _edi_mainview_item_add(options, mime, NULL, NULL, win);
   evas_object_data_set(win, "edi_mainview_item", item);

   content = _edi_mainview_content_create(item, win);
   elm_win_resize_object_add(win, content);

   // Set focus on the newly opening window so that one can just start typing
   editor = (Edi_Editor *)evas_object_data_get(content, "editor");
   if (editor)
     elm_object_focus_set(editor->entry, EINA_TRUE);

   evas_object_resize(win, 380 * elm_config_scale_get(), 260 * elm_config_scale_get());
   evas_object_show(win);

   _edi_project_config_tab_add(options->path, EINA_TRUE);
}

static void
_edi_popup_cancel_cb(void *data, Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   evas_object_del((Evas_Object *) data);
}

static void
_edi_mainview_mime_content_safe_popup(void)
{
   Evas_Object *popup, *button;

   popup = elm_popup_add(_main_win);
   elm_object_part_text_set(popup, "title,text",
                                   "Unrecognised file type");
   elm_object_text_set(popup, "To force open, select this file in the file browser, and use \"open as\" menu options.");

   button = elm_button_add(popup);
   elm_object_text_set(button, "OK");
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked", _edi_popup_cancel_cb, popup);

   evas_object_show(popup);
}

static void
_edi_mainview_tab_stat_done(void *data, Eio_File *handler EINA_UNUSED, const Eina_Stat *stat)
{
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
        _edi_mainview_mime_content_safe_popup();
        return;
     }

   options->type = provider->id;
   _edi_mainview_item_tab_add(options, mime);
}

static void
_edi_mainview_win_stat_done(void *data, Eio_File *handler EINA_UNUSED, const Eina_Stat *stat)
{
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
        _edi_mainview_mime_content_safe_popup();
        return;
     }

   options->type = provider->id;
   _edi_mainview_item_win_add(options, mime);
}

void
edi_mainview_open_path(const char *path)
{
   Edi_Path_Options *options;

   options = edi_path_options_create(path);
   edi_mainview_open(options);
}

void
edi_mainview_open(Edi_Path_Options *options)
{
   Edi_Mainview_Item *it;

   it = _get_item_for_path(options->path);
   if (it)
     {
        edi_mainview_item_select(it);
        if (options->line)
           edi_mainview_goto(options->line);
        return;
     }

   if (options->type == NULL)
     {
        eio_file_direct_stat(options->path, _edi_mainview_tab_stat_done, dummy, options);
     }
   else
     {
        _edi_mainview_item_tab_add(options, NULL);
     }
}

void
edi_mainview_open_window_path(const char *path)
{
   Edi_Path_Options *options;

   options = edi_path_options_create(path);

   edi_mainview_open_window(options);
}

void
edi_mainview_open_window(Edi_Path_Options *options)
{
   Edi_Mainview_Item *it;

   it = _get_item_for_path(options->path);
   if (it)
     {
        edi_mainview_item_select(it);
        _edi_mainview_item_close(it);
        _edi_mainview_items = eina_list_remove(_edi_mainview_items, it);
     }

   if (options->type == NULL)
     {
        eio_file_direct_stat(options->path, _edi_mainview_win_stat_done, dummy, options);
     }
   else
     {
        _edi_mainview_item_win_add(options, NULL);
     }
}

void
edi_mainview_close_all(void)
{
   Eina_List *item;
   Edi_Mainview_Item *it;

   EINA_LIST_FOREACH(_edi_mainview_items, item, it)
     {
        if (it)
          _edi_mainview_item_close(it);
     }
}

void
edi_mainview_refresh_all(void)
{
   Eina_List *item;
   Edi_Mainview_Item *it;
   char *path;

   EINA_LIST_FOREACH(_edi_mainview_items, item, it)
     {
        if (it)
          {
             path = strdup(it->path);
             _edi_mainview_item_close(it);
             if (ecore_file_exists(path))
               edi_mainview_open_path(path);
             free(path);
          }
     }
}

void
edi_mainview_item_close_path(const char *path)
{
   Eina_List *item;
   Edi_Mainview_Item *it;

   EINA_LIST_FOREACH(_edi_mainview_items, item, it)
     {
        if (it && !strcmp(it->path, path))
          {
             _edi_mainview_item_close(it);
             return;
          }
     }
}

void
edi_mainview_save()
{
   Edi_Editor *editor;
   Elm_Code *code;

   editor = (Edi_Editor *)evas_object_data_get(_current_view, "editor");

   if (!editor)
     return;

   editor->modified = EINA_FALSE;

   code = elm_code_widget_code_get(editor->entry);
   elm_code_file_save(code->file);
   ecore_event_add(EDI_EVENT_FILE_SAVED, NULL, NULL, NULL);
}

void
edi_mainview_new_window()
{
   Edi_Mainview_Item *item;

   item = edi_mainview_item_current_get();
   if (!item)
     return;

   edi_mainview_open_window_path(item->path);
}

void
edi_mainview_close()
{
   Edi_Mainview_Item *item;

   item = edi_mainview_item_current_get();

   _edi_mainview_item_close(item);
}

void
edi_mainview_closeall()
{
   Eina_List *list, *next;
   Edi_Mainview_Item *item;

   EINA_LIST_FOREACH_SAFE(_edi_mainview_items, list, next, item)
     {
        _edi_mainview_item_close(item);
     }
}

void
edi_mainview_undo()
{
   Edi_Editor *editor;

   editor = (Edi_Editor *)evas_object_data_get(_current_view, "editor");

   if (editor)
     elm_code_widget_undo(editor->entry);
}

Eina_Bool
edi_mainview_can_undo()
{
   Edi_Editor *editor;

   editor = (Edi_Editor *)evas_object_data_get(_current_view, "editor");

   if (!editor)
     return EINA_FALSE;

   return elm_code_widget_can_undo_get(editor->entry);
}

void
edi_mainview_redo()
{
   Edi_Editor *editor;

   editor = (Edi_Editor *)evas_object_data_get(_current_view, "editor");

   if (editor)
     elm_code_widget_redo(editor->entry);
}

Eina_Bool
edi_mainview_can_redo()
{
   Edi_Editor *editor;

   editor = (Edi_Editor *)evas_object_data_get(_current_view, "editor");

   if (!editor)
     return EINA_FALSE;

   return elm_code_widget_can_redo_get(editor->entry);
}

Eina_Bool
edi_mainview_modified()
{
   Edi_Editor *editor;

   editor = (Edi_Editor *)evas_object_data_get(_current_view, "editor");
   if (!editor)
     return EINA_FALSE;

   return editor->modified;
}

void
edi_mainview_cut()
{
   Edi_Editor *editor;

   editor = (Edi_Editor *)evas_object_data_get(_current_view, "editor");

   if (editor)
     elm_code_widget_selection_cut(editor->entry);
}

void
edi_mainview_copy()
{
   Edi_Editor *editor;

   editor = (Edi_Editor *)evas_object_data_get(_current_view, "editor");

   if (editor)
     elm_code_widget_selection_copy(editor->entry);
}

void
edi_mainview_paste()
{
   Edi_Editor *editor;

   editor = (Edi_Editor *)evas_object_data_get(_current_view, "editor");

   if (editor)
     elm_code_widget_selection_paste(editor->entry);
}

void
edi_mainview_search()
{
   Edi_Editor *editor;

   editor = (Edi_Editor *)evas_object_data_get(_current_view, "editor");

   if (editor)
     edi_editor_search(editor);
}

void
edi_mainview_goto(int number)
{
   Edi_Editor *editor;
   Elm_Code *code;
   Elm_Code_Line *line;

   editor = (Edi_Editor *)evas_object_data_get(_current_view, "editor");
   if (!editor || number <= 0)
     return;

   code = elm_code_widget_code_get(editor->entry);

   line = elm_code_file_line_get(code->file, number);
   if (!line)
     return;

   elm_code_widget_cursor_position_set(editor->entry, number, 1);
   elm_object_focus_set(editor->entry, EINA_TRUE);
}

static void
_edi_mainview_goto_popup_go_cb(void *data,
                             Evas_Object *obj EINA_UNUSED,
                             void *event_info EINA_UNUSED)
{
   int number;

   number = atoi(elm_entry_entry_get((Evas_Object *) data));
   edi_mainview_goto(number);

   evas_object_del(_edi_mainview_goto_popup);
}

static void
_edi_mainview_goto_popup_cancel_cb(void *data EINA_UNUSED,
                                   Evas_Object *obj EINA_UNUSED,
                                   void *event_info EINA_UNUSED)
{
   evas_object_del(_edi_mainview_goto_popup);
}

static void
_edi_mainview_goto_popup_key_up_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED,
                                   Evas_Object *obj, void *event_info)
{
   Evas_Event_Key_Up *ev = (Evas_Event_Key_Up *)event_info;
   const char *str;

   str = elm_object_text_get(obj);

   if (strlen(str) && (!strcmp(ev->key, "KP_Enter") || !strcmp(ev->key, "Return")))
     _edi_mainview_goto_popup_go_cb(obj, NULL, NULL);
}

void
edi_mainview_goto_popup_show()
{
   Evas_Object *popup, *box, *input, *button;

   popup = elm_popup_add(_main_win);
   _edi_mainview_goto_popup = popup;
   elm_object_part_text_set(popup, "title,text",
                            "Enter line number");

   box = elm_box_add(popup);
   elm_box_horizontal_set(box, EINA_FALSE);
   elm_object_content_set(popup, box);

   input = elm_entry_add(box);
   elm_entry_single_line_set(input, EINA_TRUE);
   evas_object_event_callback_add(input, EVAS_CALLBACK_KEY_UP, _edi_mainview_goto_popup_key_up_cb, NULL);
   evas_object_size_hint_weight_set(input, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_align_set(input, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(input);
   elm_box_pack_end(box, input);

   button = elm_button_add(popup);
   elm_object_text_set(button, "cancel");
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked",
                                       _edi_mainview_goto_popup_cancel_cb, NULL);

   button = elm_button_add(popup);
   elm_object_text_set(button, "go");
   elm_object_part_content_set(popup, "button2", button);
   evas_object_smart_callback_add(button, "clicked",
                                       _edi_mainview_goto_popup_go_cb, input);

   evas_object_show(popup);
   elm_object_focus_set(input, EINA_TRUE);
}

static void
_edi_mainview_project_search_popup_cancel_cb(void *data EINA_UNUSED,
                                   Evas_Object *obj EINA_UNUSED,
                                   void *event_info EINA_UNUSED)
{
   evas_object_del(_edi_mainview_search_project_popup);
}

static void
_edi_mainview_project_search_cb(void *data,
                             Evas_Object *obj EINA_UNUSED,
                             void *event_info EINA_UNUSED)
{
   const char *text;

   text = elm_entry_entry_get((Evas_Object *) data);
   if (!text || strlen(text) == 0) return;

   edi_searchpanel_show();
   edi_searchpanel_find(text);
   evas_object_del(_edi_mainview_search_project_popup);
}

void
edi_mainview_project_search_popup_show(void)
{
   Evas_Object *popup, *box, *input, *button;

   popup = elm_popup_add(_main_win);
   _edi_mainview_search_project_popup = popup;
   elm_object_part_text_set(popup, "title,text",
                            "Search for");

   box = elm_box_add(popup);
   elm_box_horizontal_set(box, EINA_FALSE);
   elm_object_content_set(popup, box);

   input = elm_entry_add(box);
   elm_entry_single_line_set(input, EINA_TRUE);
   evas_object_event_callback_add(input, EVAS_CALLBACK_KEY_UP, _edi_mainview_goto_popup_key_up_cb, NULL);
   evas_object_size_hint_weight_set(input, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_align_set(input, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(input);
   elm_box_pack_end(box, input);

   button = elm_button_add(popup);
   elm_object_text_set(button, "cancel");
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked",
                                       _edi_mainview_project_search_popup_cancel_cb, NULL);

   button = elm_button_add(popup);
   elm_object_text_set(button, "search");
   elm_object_part_content_set(popup, "button2", button);
   evas_object_smart_callback_add(button, "clicked",
                                       _edi_mainview_project_search_cb, input);

   evas_object_show(popup);
   elm_object_focus_set(input, EINA_TRUE);
}

static void
_edi_mainview_next_clicked_cb(void *data EINA_UNUSED,
                             Evas_Object *obj EINA_UNUSED,
                             void *event_info EINA_UNUSED)
{
   int x, y, w, h;
   Evas_Object *scroller = _tab_scroller;

   elm_scroller_region_get(scroller, &x, &y, &w, &h);
   x += w * 0.8;
   elm_scroller_region_bring_in(scroller, x, y, w, h);
}

static void
_edi_mainview_prev_clicked_cb(void *data EINA_UNUSED,
                             Evas_Object *obj EINA_UNUSED,
                             void *event_info EINA_UNUSED)
{
   int x, y, w, h;
   Evas_Object *scroller = _tab_scroller;

   elm_scroller_region_get(scroller, &x, &y, &w, &h);
   x -= w * 0.8;
   elm_scroller_region_bring_in(scroller, x, y, w, h);
}

void
edi_mainview_add(Evas_Object *parent, Evas_Object *win)
{
   Evas_Object *box, *scroll, *txt, *nf, *tabs, *tab, *bg, *pad, *scr;
   Evas_Object *next, *prev, *ico_next, *ico_prev;
   _main_win = win;

   box = elm_box_add(parent);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);
   elm_box_pack_end(parent, box);

   tabs = elm_table_add(box);
   elm_box_pack_end(box, tabs);
   evas_object_size_hint_weight_set(tabs, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(tabs, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(tabs);

   bg = elm_box_add(tabs);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bg, 0.0, EVAS_HINT_FILL);
   evas_object_show(bg);
   elm_table_pack(tabs, bg, 0, 0, 1, 1);

   next = elm_button_add(tabs);
   evas_object_size_hint_weight_set(next, 0, 0);
   evas_object_size_hint_align_set(next, 0, EVAS_HINT_FILL);
   elm_table_pack(tabs, next, 2, 0, 1, 1);
   evas_object_smart_callback_add(next, "clicked",
                                       _edi_mainview_next_clicked_cb, NULL);

   ico_next = elm_icon_add(next);
   elm_icon_standard_set(ico_next, "go-next");
   elm_object_part_content_set(next, "icon", ico_next);
   evas_object_show(next);

   prev = elm_button_add(tabs);
   evas_object_size_hint_weight_set(prev, 0, 0);
   evas_object_size_hint_align_set(prev, 0, EVAS_HINT_FILL);
   elm_table_pack(tabs, prev, 1, 0, 1, 1);
   evas_object_smart_callback_add(prev, "clicked",
                                       _edi_mainview_prev_clicked_cb, NULL);
   ico_prev = elm_icon_add(prev);
   elm_icon_standard_set(ico_prev, "go-previous");
   elm_object_part_content_set(prev, "icon", ico_prev);
   evas_object_show(prev);

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

   scr = elm_scroller_add(parent);
   evas_object_size_hint_weight_set(scr, EVAS_HINT_EXPAND, 0.04);
   evas_object_size_hint_align_set(scr, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_min_set(scr, 0, 100 * elm_config_scale_get());
   elm_scroller_bounce_set(scr, EINA_FALSE, EINA_FALSE);
   elm_scroller_policy_set(scr, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
   elm_table_pack(tabs, scr, 0, 0, 1, 1);
   evas_object_show(scr);
   _tab_scroller = scr;

   tb = elm_box_add(scr);
   evas_object_size_hint_weight_set(tb, 0.0, 0.0);
   evas_object_size_hint_align_set(tb, 0.0, EVAS_HINT_FILL);
   elm_box_horizontal_set(tb, EINA_TRUE);
   elm_object_content_set(scr, tb);
   evas_object_show(tb);

   nf = elm_box_add(parent);
   evas_object_size_hint_weight_set(nf, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(nf, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, nf);
   evas_object_show(nf);
   _content_frame = nf;

   scroll = elm_scroller_add(parent);
   evas_object_size_hint_weight_set(scroll, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(scroll, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(scroll);

   txt = elm_label_add(scroll);
   elm_object_text_set(txt, "<hilight>Welcome</hilight><br><br>Click on any file to edit.");
   evas_object_size_hint_weight_set(txt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(txt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(txt);
   elm_object_content_set(scroll, txt);

   _welcome_panel = scroll;
   _edi_mainview_view_show(scroll);
}
