#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <libgen.h>

#include <Eina.h>
#include <Eio.h>
#include <Elm_Code.h>

#include "mainview/edi_mainview_item.h"
#include "mainview/edi_mainview.h"

#include "editor/edi_editor.h"
#include "edi_content_provider.h"

#include "edi_private.h"
#include "edi_config.h"

static Evas_Object *nf, *tb, *_main_win;
static Evas_Object *_edi_mainview_choose_popup, *_edi_mainview_goto_popup;
static Edi_Path_Options *_edi_mainview_choose_options;

static Eina_List *_edi_mainview_items = NULL;

static void
dummy()
{}

Edi_Mainview_Item *
edi_mainview_item_current_get()
{
   Eina_List *item;
   Elm_Object_Item *current;
   Edi_Mainview_Item *it;

   current = elm_naviframe_top_item_get(nf);

   EINA_LIST_FOREACH(_edi_mainview_items, item, it)
     {
        if (it && it->view == current)
          {
             return it;
          }
     }

   return NULL;
}

void
edi_mainview_item_prev()
{
   Eina_List *item;
   Elm_Object_Item *current;
   Edi_Mainview_Item *it, *first, *prev = NULL;

   current = elm_naviframe_top_item_get(nf);
   first = (Edi_Mainview_Item *)eina_list_nth(_edi_mainview_items, 0);

   if (first->view == current)
     {
        prev = eina_list_nth(_edi_mainview_items, eina_list_count(_edi_mainview_items)-1);
        edi_mainview_item_select(prev);
        return;
     }

   EINA_LIST_FOREACH(_edi_mainview_items, item, it)
     {

        if (it && it->view == current)
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
   Elm_Object_Item *current;
   Edi_Mainview_Item *it, *last, *next;
   Eina_Bool open_next = EINA_FALSE;

   current = elm_naviframe_top_item_get(nf);
   last = eina_list_nth(_edi_mainview_items, eina_list_count(_edi_mainview_items)-1);

   if (last->view == current)
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

        if (it && it->view == current)
          open_next = EINA_TRUE;
     }
}

void
edi_mainview_item_select(Edi_Mainview_Item *item)
{
   if (item->win)
     {
        elm_win_raise(item->win);
     }
   else
     {
        elm_naviframe_item_promote(item->view);
        elm_toolbar_item_selected_set(item->tab, EINA_TRUE);
     }
}

static void
_promote(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_naviframe_item_promote(data);
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
   Evas_Object *content;
   Elm_Object_Item *it, *tab;
   Edi_Mainview_Item *item;
   Edi_Editor *editor;
   Edi_Content_Provider *provider;

   item = _edi_mainview_item_add(options, mime, NULL, NULL, NULL);
   provider = edi_content_provider_for_id_get(item->editortype);
   content = _edi_mainview_content_create(item, nf);

   it = elm_naviframe_item_simple_push(nf, content);
   item->view = it;
   elm_object_item_data_set(it, item);

   elm_naviframe_item_style_set(it, "overlap");
   tab = elm_toolbar_item_append(tb, provider->icon, basename((char*)options->path), _promote, it);
   item->tab = tab;
   elm_toolbar_item_selected_set(tab, EINA_TRUE);

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
_edi_mainview_choose_type_tab_cb(void *data,
                             Evas_Object *obj EINA_UNUSED,
                             void *event_info EINA_UNUSED)
{
   evas_object_del(_edi_mainview_choose_popup);

   _edi_mainview_choose_options->type = (const char *) data;
   _edi_config_mime_add(efreet_mime_type_get(_edi_mainview_choose_options->path), _edi_mainview_choose_options->type);
   edi_mainview_open(_edi_mainview_choose_options);
}

static void
_edi_mainview_choose_type_win_cb(void *data,
                             Evas_Object *obj EINA_UNUSED,
                             void *event_info EINA_UNUSED)
{
   evas_object_del(_edi_mainview_choose_popup);

   _edi_mainview_choose_options->type = (const char *) data;
   _edi_config_mime_add(efreet_mime_type_get(_edi_mainview_choose_options->path), _edi_mainview_choose_options->type);
   edi_mainview_open_window(_edi_mainview_choose_options);

}

static void
_edi_mainview_choose_type_close_cb(void *data EINA_UNUSED,
                                   Evas_Object *obj EINA_UNUSED,
                                   void *event_info EINA_UNUSED)
{
   evas_object_del(_edi_mainview_choose_popup);
}

static void
_edi_mainview_filetype_create(Evas_Object *popup, const char *type, void *cb)
{
   Edi_Content_Provider *provider;
   Evas_Object *icon;

   provider = edi_content_provider_for_id_get(type);
   if (!provider)
     return;

   icon = elm_icon_add(popup);
   elm_icon_standard_set(icon, provider->icon);
   elm_popup_item_append(popup, type, icon, cb, type);
}

static void
_edi_mainview_choose_type(Evas_Object *parent EINA_UNUSED, Edi_Path_Options *options, void *cb)
{
   Evas_Object *popup, *cancel;

   popup = elm_popup_add(_main_win);
   _edi_mainview_choose_popup = popup;
   _edi_mainview_choose_options = options;

   // popup title
   elm_object_part_text_set(popup, "title,text",
                            "Unrecognised file type");

   _edi_mainview_filetype_create(popup, "text", cb);
   _edi_mainview_filetype_create(popup, "code", cb);
   _edi_mainview_filetype_create(popup, "image", cb);

   cancel = elm_button_add(popup);
   elm_object_text_set(cancel, "cancel");
   elm_object_part_content_set(popup, "button1", cancel);
   evas_object_smart_callback_add(cancel, "clicked",
                                       _edi_mainview_choose_type_close_cb, NULL);

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
        _edi_mainview_choose_type(nf, options, _edi_mainview_choose_type_tab_cb);
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
        _edi_mainview_choose_type(nf, options, _edi_mainview_choose_type_win_cb);
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
        elm_naviframe_item_pop(nf);
        elm_object_item_del(elm_toolbar_selected_item_get(tb));
        _edi_mainview_items = eina_list_remove(_edi_mainview_items, it);

        eina_stringshare_del(it->path);
        free(it);
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
edi_mainview_save()
{
   Evas_Object *content;
   Elm_Object_Item *it;
   Edi_Editor *editor;
   Elm_Code *code;

   it = elm_naviframe_top_item_get(nf);
   content = elm_object_item_content_get(it);
   editor = (Edi_Editor *)evas_object_data_get(content, "editor");

   if (!editor)
     return;

   code = elm_code_widget_code_get(editor->entry);
   elm_code_file_save(code->file);
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

static void
_edi_mainview_close_item(Edi_Mainview_Item *item)
{
   if (!item)
     return;

   elm_naviframe_item_promote(item->view);
   elm_naviframe_item_pop(nf);
   elm_object_item_del(item->tab);
   _edi_mainview_items = eina_list_remove(_edi_mainview_items, item);

   _edi_project_config_tab_remove(item->path);
   eina_stringshare_del(item->path);
   free(item);
}

void
edi_mainview_close()
{
   Edi_Mainview_Item *item;

   item = edi_mainview_item_current_get();

   _edi_mainview_close_item(item);
}

void
edi_mainview_closeall()
{
   Eina_List *list, *next;
   Edi_Mainview_Item *item;

   EINA_LIST_FOREACH_SAFE(_edi_mainview_items, list, next, item)
     {
        _edi_mainview_close_item(item);
     }
}

void
edi_mainview_undo()
{
   Evas_Object *content;
   Elm_Object_Item *it;
   Edi_Editor *editor;

   it = elm_naviframe_top_item_get(nf);
   content = elm_object_item_content_get(it);
   editor = (Edi_Editor *)evas_object_data_get(content, "editor");

   if (editor)
     elm_code_widget_undo(editor->entry);
}

void
edi_mainview_cut()
{
   Evas_Object *content;
   Elm_Object_Item *it;
   Edi_Editor *editor;

   it = elm_naviframe_top_item_get(nf);
   content = elm_object_item_content_get(it);
   editor = (Edi_Editor *)evas_object_data_get(content, "editor");

   if (editor)
     elm_code_widget_selection_cut(editor->entry);
}

void
edi_mainview_copy()
{
   Evas_Object *content;
   Elm_Object_Item *it;
   Edi_Editor *editor;

   it = elm_naviframe_top_item_get(nf);
   content = elm_object_item_content_get(it);
   editor = (Edi_Editor *)evas_object_data_get(content, "editor");

   if (editor)
     elm_code_widget_selection_copy(editor->entry);
}

void
edi_mainview_paste()
{
   Evas_Object *content;
   Elm_Object_Item *it;
   Edi_Editor *editor;

   it = elm_naviframe_top_item_get(nf);
   content = elm_object_item_content_get(it);
   editor = (Edi_Editor *)evas_object_data_get(content, "editor");

   if (editor)
     elm_code_widget_selection_paste(editor->entry);
}

void
edi_mainview_search()
{
   Evas_Object *content;
   Elm_Object_Item *it;
   Edi_Editor *editor;

   it = elm_naviframe_top_item_get(nf);
   content = elm_object_item_content_get(it);
   editor = (Edi_Editor *)evas_object_data_get(content, "editor");

   if (editor)
     edi_editor_search(editor);
}

void
edi_mainview_goto(int line)
{
   Evas_Object *content;
   Elm_Object_Item *it;
   Edi_Editor *editor;

   it = elm_naviframe_top_item_get(nf);
   content = elm_object_item_content_get(it);
   editor = (Edi_Editor *)evas_object_data_get(content, "editor");
   if (!content || !editor || line <= 0)
     return;

   elm_code_widget_cursor_position_set(editor->entry, 1, line);
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

void
edi_mainview_add(Evas_Object *parent, Evas_Object *win)
{
   Evas_Object *box, *txt;
   Elm_Object_Item *it;

   _main_win = win;

   box = elm_box_add(parent);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);
   elm_box_pack_end(parent, box);

   tb = elm_toolbar_add(parent);
   evas_object_size_hint_weight_set(tb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_toolbar_homogeneous_set(tb, EINA_FALSE);
   elm_toolbar_icon_order_lookup_set(tb, ELM_ICON_LOOKUP_FDO_THEME);
   elm_toolbar_icon_size_set(tb, 20);
   elm_object_style_set(tb, "item_horizontal");
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_MENU);
   elm_toolbar_select_mode_set(tb, ELM_OBJECT_SELECT_MODE_ALWAYS);
   elm_box_pack_end(box, tb);
   evas_object_show(tb);

   nf = elm_naviframe_add(parent);
   evas_object_size_hint_weight_set(nf, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(nf, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, nf);
   evas_object_show(nf);

   txt = elm_label_add(parent);
   elm_object_text_set(txt, "Welcome - click on a file to edit");
   evas_object_size_hint_weight_set(txt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(txt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(txt);

   it = elm_naviframe_item_simple_push(nf, txt);
   elm_naviframe_item_style_set(it, "overlap");
   elm_toolbar_item_append(tb, "go-home", "Welcome", _promote, it);
}
