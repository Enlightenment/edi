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

#include "editor/edi_editor.h"
#include "edi_content_provider.h"
#include "../edi_searchpanel.h"

#include "edi_private.h"
#include "edi_config.h"

static Evas_Object *_main_win, *_mainview_panel, *_mainview_parent;
static Evas_Object *_edi_mainview_search_project_popup;

static Edi_Mainview_Panel *_current_panel;
static Eina_List *_edi_mainview_panels = NULL, *_edi_mainview_wins = NULL;

static void
dummy()
{}

int
edi_mainview_panel_count(void)
{
   return eina_list_count(_edi_mainview_panels);
}

int edi_mainview_panel_id(Edi_Mainview_Panel *panel)
{
   Eina_List *it;
   Edi_Mainview_Panel *p;
   int i = 0;

   EINA_LIST_FOREACH(_edi_mainview_panels, it, p)
     {
        if (panel == p)
          break;
        i++;
     }

   return i;
}

Edi_Mainview_Panel *
edi_mainview_panel_by_index(int index)
{
   return eina_list_nth(_edi_mainview_panels, index);
}

void edi_mainview_panel_focus(Edi_Mainview_Panel *panel)
{
   _current_panel = panel;
}

Edi_Mainview_Item *
edi_mainview_item_current_get()
{
   return edi_mainview_panel_item_current_get(_current_panel);
}

Edi_Mainview_Panel *
edi_mainview_panel_current_get()
{
   return _current_panel;
}

Edi_Mainview_Panel *
edi_mainview_panel_for_item_get(Edi_Mainview_Item *item)
{
   Eina_List *it;
   Edi_Mainview_Panel *panel;

   EINA_LIST_FOREACH(_edi_mainview_panels, it, panel)
     {
        if (edi_mainview_panel_item_contains(panel, item))
          return panel;
     }

   return NULL;
}

unsigned int
edi_mainview_panel_index_get(Edi_Mainview_Panel *panel)
{
   Eina_List *it;
   Edi_Mainview_Panel *panel2;
   unsigned int i = 0;

   EINA_LIST_FOREACH(_edi_mainview_panels, it, panel2)
     {
        if (panel == panel2)
          break;

        i++;
     }

   return i;
}

void
edi_mainview_item_prev()
{
   edi_mainview_panel_item_prev(_current_panel);
}

void
edi_mainview_item_next()
{
   edi_mainview_panel_item_next(_current_panel);
}

void
edi_mainview_tab_select(unsigned int id)
{
   edi_mainview_panel_tab_select(_current_panel, id);
}

static void
_edi_mainview_win_exit(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Edi_Mainview_Item *it;

   evas_object_hide(obj);

   it = evas_object_data_get(obj, "edi_mainview_item");
   _edi_mainview_wins = eina_list_remove(_edi_mainview_wins, it);

   _edi_project_config_tab_remove(it->path, EINA_TRUE, 0);
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

static Evas_Object *
_edi_mainview_content_create(Edi_Mainview_Item *item, Evas_Object *parent)
{

   Evas_Object *container;

   container = elm_box_add(parent);
   evas_object_size_hint_weight_set(container, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(container, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(container);

   item->loaded = EINA_FALSE;
   item->container = container;
// TODO not in 2 halfs
   Edi_Content_Provider *provider;
   Evas_Object *child;

   provider = edi_content_provider_for_id_get(item->editortype);
   if (!provider)
     {
        ERR("No content provider found for type %s", item->editortype);
        return container;
     }
   child = provider->content_ui_add(item->container, item);

   evas_object_size_hint_weight_set(child, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(child, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(item->container, child);
   evas_object_show(child);

   item->loaded = EINA_TRUE;
   return container;
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
   item = edi_mainview_item_add(options, mime, NULL, win);
   _edi_mainview_wins = eina_list_append(_edi_mainview_wins, item);
   evas_object_data_set(win, "edi_mainview_item", item);

   content = _edi_mainview_content_create(item, win);
   elm_win_resize_object_add(win, content);

   // Set focus on the newly opening window so that one can just start typing
   editor = (Edi_Editor *)evas_object_data_get(content, "editor");
   if (editor)
     elm_object_focus_set(editor->entry, EINA_TRUE);

   evas_object_resize(win, 380 * elm_config_scale_get(), 260 * elm_config_scale_get());
   evas_object_show(win);

   _edi_project_config_tab_add(options->path, mime?mime:options->type, EINA_TRUE, 0);
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
//TODO        _edi_mainview_mime_content_safe_popup();
        return;
     }

   options->type = provider->id;
   _edi_mainview_item_win_add(options, mime);
}

void
edi_mainview_open_path(const char *path)
{
   edi_mainview_panel_open_path(_current_panel, path);
}

void
edi_mainview_open(Edi_Path_Options *options)
{
   edi_mainview_panel_open(_current_panel, options);
}

void
edi_mainview_open_window_path(const char *path)
{
   Edi_Path_Options *options;

   options = edi_path_options_create(path);
// TODO this will not work right now - windows come from tabs so do we need it?
   edi_mainview_open_window(options);
}

void
edi_mainview_open_window(Edi_Path_Options *options)
{
   edi_mainview_item_close_path(options->path);

   if (options->type == NULL)
     {
        eio_file_direct_stat(options->path, _edi_mainview_win_stat_done, dummy, options);
     }
   else if (!edi_content_provider_for_id_get(options->type))
     {
        const char *mime = options->type;
        Edi_Content_Provider *provider = edi_content_provider_for_mime_get(mime);

        if (provider)
          options->type = provider->id;
        else
          options->type = NULL;

        _edi_mainview_item_win_add(options, mime);
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
   Edi_Mainview_Panel *it;

   EINA_LIST_FOREACH(_edi_mainview_panels, item, it)
     edi_mainview_panel_close_all(it);
}

void
edi_mainview_refresh_all(void)
{
   Eina_List *item;
   Edi_Mainview_Panel *it;

   EINA_LIST_FOREACH(_edi_mainview_panels, item, it)
     edi_mainview_panel_refresh_all(it);
}

void
edi_mainview_item_close_path(const char *path)
{
   Eina_List *item;
   Edi_Mainview_Panel *it;

   EINA_LIST_FOREACH(_edi_mainview_panels, item, it)
     edi_mainview_panel_item_close_path(it, path);
}

void
edi_mainview_save()
{
   edi_mainview_panel_save(_current_panel);
}

void
edi_mainview_new_window()
{
   Edi_Mainview_Item *item;

   item = edi_mainview_item_current_get();
   if (!item)
     return;
// TODO OPTIONS!
   edi_mainview_open_window_path(item->path);
}

void
edi_mainview_close()
{
   edi_mainview_panel_close(_current_panel);
}

void
edi_mainview_undo()
{
   edi_mainview_panel_undo(_current_panel);
}

Eina_Bool
edi_mainview_can_undo()
{
   return edi_mainview_panel_can_undo(_current_panel);
}

void
edi_mainview_redo()
{
   edi_mainview_panel_redo(_current_panel);
}

Eina_Bool
edi_mainview_can_redo()
{
   return edi_mainview_panel_can_redo(_current_panel);
}

Eina_Bool
edi_mainview_modified()
{
   return edi_mainview_panel_modified(_current_panel);
}

void
edi_mainview_cut()
{
   edi_mainview_panel_cut(_current_panel);
}

void
edi_mainview_copy()
{
   edi_mainview_panel_copy(_current_panel);
}

void
edi_mainview_paste()
{
   edi_mainview_panel_paste(_current_panel);
}

void
edi_mainview_search()
{
   edi_mainview_panel_search(_current_panel);
}

void
edi_mainview_goto(unsigned int number)
{
   edi_mainview_panel_goto(_current_panel, number);
}

void
edi_mainview_goto_position(unsigned int row, unsigned int col)
{
   edi_mainview_panel_goto_position(_current_panel, row, col);
}

void
edi_mainview_goto_popup_show()
{
   edi_mainview_panel_goto_popup_show(_current_panel);
}

static void
_edi_mainview_project_search_popup_cancel_cb(void *data EINA_UNUSED,
                                   Evas_Object *obj EINA_UNUSED,
                                   void *event_info EINA_UNUSED)
{
   evas_object_del(_edi_mainview_search_project_popup);
}

static void
_edi_mainview_project_search_cb(void *data EINA_UNUSED,
                             Evas_Object *obj,
                             void *event_info EINA_UNUSED)
{
   const char *text_markup;
   char *text;

   text_markup = elm_object_text_get(obj);
   if (!text_markup || !text_markup[0]) return;

   text = elm_entry_markup_to_utf8(text_markup);

   edi_searchpanel_show();
   edi_searchpanel_find(text);

   free(text);
   evas_object_del(_edi_mainview_search_project_popup);
}

static void
_edi_mainview_project_search_popup_key_up_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED,
                                   Evas_Object *obj, void *event_info)
{
   Evas_Event_Key_Up *ev = (Evas_Event_Key_Up *)event_info;
   const char *str;

   str = elm_object_text_get(obj);

   if (strlen(str) && (!strcmp(ev->key, "KP_Enter") || !strcmp(ev->key, "Return")))
     _edi_mainview_project_search_cb(NULL, obj, NULL);
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
   elm_entry_scrollable_set(input, EINA_TRUE);
   evas_object_event_callback_add(input, EVAS_CALLBACK_KEY_UP, _edi_mainview_project_search_popup_key_up_cb, NULL);
   evas_object_size_hint_weight_set(input, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
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
void
edi_mainview_panel_remove(Edi_Mainview_Panel *panel)
{
   evas_object_del(panel->welcome);
   evas_object_del(panel->content);
   evas_object_del(panel->tabs);
   evas_object_del(panel->scroll);
   evas_object_del(panel->box);

   _edi_mainview_panels = eina_list_remove(_edi_mainview_panels, panel);

   free(panel);
   panel = NULL;
}

Edi_Mainview_Panel *
edi_mainview_panel_append()
{
   Edi_Mainview_Panel *panel;
   Evas_Object *split, *box;

   elm_box_unpack_all(_mainview_parent);

   split = elm_panes_add(_mainview_parent);
   evas_object_size_hint_weight_set(split, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(split, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(split);
   elm_box_pack_end(_mainview_parent, split);

   box = elm_box_add(_mainview_parent);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);

   elm_object_part_content_set(split, "left", _mainview_panel);
   elm_object_part_content_set(split, "right", box);

   panel = edi_mainview_panel_add(box);
   _edi_mainview_panels = eina_list_append(_edi_mainview_panels, panel);

   _mainview_panel = split;

   return panel;
}

void
edi_mainview_add(Evas_Object *parent, Evas_Object *win)
{
   Edi_Mainview_Panel *panel;
   Evas_Object *box;

   _main_win = win;
   _mainview_parent = parent;

   box = elm_box_add(parent);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);
   elm_box_pack_end(parent, box);
   _mainview_panel = box;

   panel = edi_mainview_panel_add(box);
   _current_panel = panel;
   _edi_mainview_panels = eina_list_append(_edi_mainview_panels, panel);
}

