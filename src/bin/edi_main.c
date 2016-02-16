#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* NOTE: Respecting header order is important for portability.
 * Always put system first, then EFL, then your public header,
 * and finally your private one. */

#if ENABLE_NLS
# include <libintl.h>
#endif

#include <Ecore_Getopt.h>
#include <Elementary.h>
#include <Elm_Code.h>
#include <Eio.h>

#include "Edi.h"
#include "edi_config.h"
#include "edi_filepanel.h"
#include "edi_logpanel.h"
#include "edi_consolepanel.h"
#include "mainview/edi_mainview.h"
#include "screens/edi_screens.h"

#include "edi_private.h"

typedef struct _Edi_Panel_Slide_Effect
{
   double max;
   Evas_Object *content;

   Eina_Bool expand;
   Eina_Bool left;
} Edi_Panel_Slide_Effect;

#define COPYRIGHT "Copyright © 2014-2015 Andy Williams <andy@andyilliams.me> and various contributors (see AUTHORS)."

static Evas_Object *_edi_toolbar, *_edi_leftpanes, *_edi_bottompanes;
static Evas_Object *_edi_logpanel, *_edi_consolepanel, *_edi_testpanel;
static Elm_Object_Item *_edi_logpanel_item, *_edi_consolepanel_item, *_edi_testpanel_item;
static Elm_Object_Item *_edi_selected_bottompanel;
static Evas_Object *_edi_filepanel, *_edi_filepanel_icon;

static Evas_Object *_edi_main_win, *_edi_main_box, *_edi_new_popup, *_edi_message_popup;
int _edi_log_dom = -1;

static void
_edi_on_close_message(void *data,
                     Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   evas_object_del(data);
   evas_object_del(_edi_message_popup);
}

static void
_edi_message_open(const char *message)
{
   Evas_Object *popup, *button;

   popup = elm_popup_add(_edi_main_win);
   _edi_message_popup = popup;
   elm_object_part_text_set(popup, "title,text",
                           message);

   button = elm_button_add(popup);
   elm_object_text_set(button, "Ok");
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked",
                                 _edi_on_close_message, NULL);

   evas_object_show(popup);
}

static void
_edi_file_open_cb(const char *path, const char *type, Eina_Bool newwin)
{
   Edi_Path_Options *options;

   if (path == NULL)
     {
        _edi_message_open("Please choose a file from the list");
        return;
     }

   options = edi_path_options_create(path);
   options->type = type;

   if (type == NULL)
     INF("Opening %s", path);
   else
     INF("Opening %s as %s", path, type);

   if (newwin)
     edi_mainview_open_window(options);
   else
     edi_mainview_open(options);
}

static Evas_Object *
_edi_panel_tab_for_index(int index)
{
   if (index == 1)
     return _edi_consolepanel;
   if (index == 2)
     return _edi_testpanel;

   return _edi_logpanel;
}

static void
_edi_panel_size_save(Eina_Bool left)
{
   double size;
   Eina_Bool open;

   if (left)
     size = elm_panes_content_left_size_get(_edi_leftpanes);
   else
     size = elm_panes_content_right_size_get(_edi_bottompanes);
   open = (size != 0.0);

   if (left)
     {
        _edi_project_config->gui.leftopen = open;
        if (open)
          _edi_project_config->gui.leftsize = size;
     }
   else
     {
        _edi_project_config->gui.bottomopen = open;
        if (open)
          _edi_project_config->gui.bottomsize = size;
     }

   _edi_project_config_save();
}

static void
_edi_panel_save_tab(int index)
{
   _edi_project_config->gui.bottomtab = index;
   _edi_project_config_save();
}

static void
_edi_slide_panel(Elm_Transit_Effect *effect, Elm_Transit *transit, double progress)
{
   Edi_Panel_Slide_Effect *slide;
   Evas_Object *obj;
   double position, weight;
   const Eina_List *item;
   const Eina_List *objs;

   if (!effect) return;
   slide = (Edi_Panel_Slide_Effect *)effect;
   objs = elm_transit_objects_get(transit);

   if (slide->left)
     position = elm_panes_content_left_size_get(eina_list_nth(objs, 0));
   else
     position = elm_panes_content_right_size_get(eina_list_nth(objs, 0));

   if (slide->expand)
     weight = progress * slide->max;
   else
     weight = (1 - progress) * position;

   if (slide->left)
     EINA_LIST_FOREACH(objs, item, obj)
        elm_panes_content_left_size_set(obj, weight);
   else
     EINA_LIST_FOREACH(objs, item, obj)
        elm_panes_content_right_size_set(obj, weight);
}

static void
_edi_slide_panel_free(Elm_Transit_Effect *effect, Elm_Transit *transit EINA_UNUSED)
{
   Edi_Panel_Slide_Effect *slide;

   slide = (Edi_Panel_Slide_Effect *) effect;
   if (!slide->expand)
     evas_object_hide(slide->content);
   free(slide);

   _edi_panel_size_save(slide->left);
}

static void
_edi_slide_panel_new(Evas_Object *panel, Evas_Object *content, double max,
                     Eina_Bool expand, Eina_Bool left)
{
   Edi_Panel_Slide_Effect *slide;
   Elm_Transit *trans;

   slide = malloc(sizeof(Edi_Panel_Slide_Effect));
   slide->max = max;
   slide->content = content;
   slide->expand = expand;
   slide->left = left;

   if (slide->expand)
     evas_object_show(slide->content);

   /* Adding Transit */
   trans = elm_transit_add();
   elm_transit_tween_mode_set(trans, ELM_TRANSIT_TWEEN_MODE_SINUSOIDAL);
   elm_transit_object_add(trans, panel);
   elm_transit_effect_add(trans, _edi_slide_panel, slide, _edi_slide_panel_free);
   elm_transit_duration_set(trans, 0.33);
   elm_transit_go(trans);
}

static void
_edi_toggle_file_panel(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *panel;
   double size;

   panel = (Evas_Object *)data;
   size = elm_panes_content_left_size_get(_edi_leftpanes);

   if (size == 0.0)
     {
        elm_icon_standard_set(_edi_filepanel_icon, "stock_left");
        _edi_slide_panel_new(_edi_leftpanes, panel, _edi_project_config->gui.leftsize, EINA_TRUE, EINA_TRUE);
     }
   else
     {
        elm_icon_standard_set(_edi_filepanel_icon, "stock_right");
        _edi_slide_panel_new(_edi_leftpanes, panel, _edi_project_config->gui.leftsize, EINA_FALSE, EINA_TRUE);
     }
}

static void
_edi_toggle_panel(void *data, Evas_Object *obj, void *event_info)
{
   int index, c;
   double size;
   Elm_Object_Item *item;
   Evas_Object *panel;

   index = atoi((char *) data);
   panel = _edi_panel_tab_for_index(index);
   item = (Elm_Object_Item *) event_info;

   if (obj)
     elm_object_focus_set(obj, EINA_FALSE);

   for (c = 0; c <= 2; c++)
     if (c != index)
       evas_object_hide(_edi_panel_tab_for_index(c));

   if (item == _edi_selected_bottompanel)
     {
        elm_toolbar_item_icon_set(item, "stock_up");

        _edi_slide_panel_new(_edi_bottompanes, panel, _edi_project_config->gui.bottomsize, EINA_FALSE, EINA_FALSE);
        _edi_selected_bottompanel = NULL;
     }
   else
     {
        if (_edi_selected_bottompanel)
          elm_toolbar_item_icon_set(_edi_selected_bottompanel, "stock_up");
        elm_toolbar_item_icon_set(item, "stock_down");

        size = elm_panes_content_right_size_get(_edi_bottompanes);
        if (size == 0.0)
          _edi_slide_panel_new(_edi_bottompanes, panel, _edi_project_config->gui.bottomsize, EINA_TRUE, EINA_FALSE);
        else
          evas_object_show(panel);

        _edi_selected_bottompanel = item;
     }
   elm_toolbar_item_selected_set(item, EINA_FALSE);
   _edi_panel_save_tab(index);
}

static void
_edi_panel_dragged_cb(void *data, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
   _edi_panel_size_save(data == _edi_filepanel);
}

void
edi_consolepanel_show()
{
   if (_edi_selected_bottompanel != _edi_consolepanel_item)
     elm_toolbar_item_selected_set(_edi_consolepanel_item, EINA_TRUE);
}

void
edi_testpanel_show()
{
   if (_edi_selected_bottompanel != _edi_testpanel_item)
     elm_toolbar_item_selected_set(_edi_testpanel_item, EINA_TRUE);
}

static Evas_Object *
edi_content_setup(Evas_Object *win, const char *path)
{
   Evas_Object *filepane, *logpane, *logpanels, *content_out, *content_in, *tb;
   Evas_Object *icon, *button;

   filepane = elm_panes_add(win);
   evas_object_size_hint_weight_set(filepane, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(filepane, EVAS_HINT_FILL, EVAS_HINT_FILL);
   logpane = elm_panes_add(win);
   elm_panes_horizontal_set(logpane, EINA_TRUE);
   evas_object_size_hint_weight_set(logpane, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(logpane, EVAS_HINT_FILL, EVAS_HINT_FILL);

   _edi_filepanel = elm_box_add(win);
   _edi_logpanel = elm_box_add(win);
   _edi_consolepanel = elm_box_add(win);
   _edi_testpanel = elm_box_add(win);

   // add main content
   content_out = elm_box_add(win);
   elm_box_horizontal_set(content_out, EINA_FALSE);
   evas_object_size_hint_weight_set(content_out, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(content_out, EVAS_HINT_FILL, EVAS_HINT_FILL);

   content_in = elm_box_add(content_out);
   elm_box_horizontal_set(content_in, EINA_TRUE);
   evas_object_size_hint_weight_set(content_in, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(content_in, EVAS_HINT_FILL, EVAS_HINT_FILL);

   icon = elm_icon_add(content_in);
   if (_edi_project_config->gui.leftopen)
     elm_icon_standard_set(icon, "stock_left");
   else
     elm_icon_standard_set(icon, "stock_right");
   button = elm_button_add(content_in);
   elm_object_part_content_set(button, "icon", icon);
   elm_object_focus_allow_set(button, EINA_FALSE);
   _edi_filepanel_icon = icon;

   evas_object_smart_callback_add(button, "clicked",
                                  _edi_toggle_file_panel, _edi_filepanel);
   evas_object_size_hint_align_set(button, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(content_in, button);
   evas_object_show(button);

   edi_mainview_add(content_in, win);
   evas_object_show(content_in);
   elm_object_part_content_set(filepane, "right", content_in);
   elm_box_pack_end(content_out, filepane);

   elm_object_part_content_set(logpane, "top", content_out);
   evas_object_show(filepane);
   _edi_leftpanes = filepane;

   // add file list
   evas_object_size_hint_weight_set(_edi_filepanel, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(_edi_filepanel, EVAS_HINT_FILL, EVAS_HINT_FILL);
   if (_edi_project_config->gui.leftopen)
     elm_panes_content_left_size_set(filepane, _edi_project_config->gui.leftsize);
   else
     elm_panes_content_left_size_set(filepane, 0.0);
   evas_object_smart_callback_add(filepane, "unpress", _edi_panel_dragged_cb, _edi_filepanel);

   edi_filepanel_add(_edi_filepanel, win, path, _edi_file_open_cb);
   elm_object_part_content_set(filepane, "left", _edi_filepanel);
   evas_object_show(_edi_filepanel);

   // add lower panel buttons
   tb = elm_toolbar_add(content_out);
   evas_object_size_hint_weight_set(tb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_toolbar_homogeneous_set(tb, EINA_FALSE);
   elm_toolbar_align_set(tb, 1.0);
   elm_toolbar_icon_size_set(tb, 14);
   elm_object_style_set(tb, "item_horizontal");
   elm_object_focus_allow_set(tb, EINA_FALSE);
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_SCROLL);
   elm_toolbar_select_mode_set(tb, ELM_OBJECT_SELECT_MODE_DEFAULT);
   elm_box_pack_end(content_out, tb);
   evas_object_show(tb);

   _edi_logpanel_item = elm_toolbar_item_append(tb, "stock_up", "Logs",
                                                _edi_toggle_panel, "0");
   _edi_consolepanel_item = elm_toolbar_item_append(tb, "stock_up", "Console",
                                                    _edi_toggle_panel, "1");
   _edi_testpanel_item = elm_toolbar_item_append(tb, "stock_up", "Tests",
                                                 _edi_toggle_panel, "2");

   // add lower panel panes
   logpanels = elm_table_add(logpane);
   evas_object_size_hint_weight_set(_edi_logpanel, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(_edi_logpanel, EVAS_HINT_FILL, EVAS_HINT_FILL);

   edi_logpanel_add(_edi_logpanel);
   elm_table_pack(logpanels, _edi_logpanel, 0, 0, 1, 1);

   evas_object_size_hint_weight_set(_edi_consolepanel, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(_edi_consolepanel, EVAS_HINT_FILL, EVAS_HINT_FILL);

   edi_consolepanel_add(_edi_consolepanel);
   elm_table_pack(logpanels, _edi_consolepanel, 0, 0, 1, 1);

   evas_object_size_hint_weight_set(_edi_testpanel, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(_edi_testpanel, EVAS_HINT_FILL, EVAS_HINT_FILL);

   edi_testpanel_add(_edi_testpanel);
   elm_table_pack(logpanels, _edi_testpanel, 0, 0, 1, 1);
   elm_object_part_content_set(logpane, "bottom", logpanels);
   if (_edi_project_config->gui.bottomopen)
     {
        elm_panes_content_right_size_set(logpane, _edi_project_config->gui.bottomsize);
        if (_edi_project_config->gui.bottomtab == 1)
          {
             elm_toolbar_item_icon_set(_edi_consolepanel_item, "stock_down");
             _edi_selected_bottompanel = _edi_consolepanel_item;
          }
        else if (_edi_project_config->gui.bottomtab == 2)
          {
             elm_toolbar_item_icon_set(_edi_testpanel_item, "stock_down");
             _edi_selected_bottompanel = _edi_testpanel_item;
          }
        else
          {
             elm_toolbar_item_icon_set(_edi_logpanel_item, "stock_down");
             _edi_selected_bottompanel = _edi_logpanel_item;
          }
     }
   else
     elm_panes_content_right_size_set(logpane, 0.0);
   if (_edi_project_config->gui.bottomopen)
     evas_object_show(_edi_panel_tab_for_index(_edi_project_config->gui.bottomtab));
   evas_object_smart_callback_add(logpane, "unpress", _edi_panel_dragged_cb, NULL);

   evas_object_show(logpane);
   _edi_bottompanes = logpane;

   return logpane;
}

static void
_edi_popup_cancel_cb(void *data, Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   evas_object_del((Evas_Object *)data);
}

static void
_edi_launcher_run(Edi_Project_Config_Launch *launch)
{
   Evas_Object *popup, *button;
   char *full_cmd;
   int full_len;

   if (_edi_project_config->launch.path)
     {
       if (!_edi_project_config->launch.args)
         {
            ecore_exe_run(launch->path, NULL);

            return;
         }
       else
         {
            full_len = strlen(_edi_project_config->launch.path)
                              + strlen(_edi_project_config->launch.path);
            full_cmd = malloc(sizeof(char) * (full_len + 1));
            snprintf(full_cmd, full_len + 2, "%s %s", _edi_project_config->launch.path,
                     _edi_project_config->launch.args);
            ecore_exe_run(full_cmd, NULL);

            free(full_cmd);
            return;
         }
     }

   popup = elm_popup_add(_edi_main_win);
   elm_object_part_text_set(popup, "title,text", "Unable to launch");
   elm_object_text_set(popup, "No launch binary found, please configure in Settings");

   button = elm_button_add(popup);
   elm_object_text_set(button, "OK");
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked", _edi_popup_cancel_cb, popup);

   evas_object_show(popup);
}

static void
_tb_new_create_cb(void *data,
                             Evas_Object *obj EINA_UNUSED,
                             void *event_info EINA_UNUSED)
{
   const char *selected, *path, *name;
   FILE *fileid;

   name = elm_entry_entry_get((Evas_Object *) data);
   if (!name || strlen(name) == 0)
     {
        _edi_message_open("Please enter a file name.");
        return;
     }

   selected = edi_filepanel_selected_path_get(_edi_filepanel);
   if (selected && ecore_file_is_dir(selected))
     path = edi_path_append(selected, name);
   else
     path = edi_project_file_path_get(name);

   fileid = fopen(path, "w");
   if (!fileid)
     _edi_message_open("Unable to write file.");
   else
     {
        _edi_filepanel_reload();
        edi_mainview_open_path(path);
     }

   evas_object_del(_edi_new_popup);
   free((char*)path);
}

static void
_edi_file_new()
{
   Evas_Object *popup, *box, *input, *button;

   popup = elm_popup_add(_edi_main_win);
   _edi_new_popup = popup;
   elm_object_part_text_set(popup, "title,text",
                            "Enter new file name");

   box = elm_box_add(popup);
   elm_box_horizontal_set(box, EINA_FALSE);
   elm_object_content_set(popup, box);

   input = elm_entry_add(box);
   elm_entry_single_line_set(input, EINA_TRUE);
   evas_object_size_hint_weight_set(input, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_align_set(input, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(input);
   elm_box_pack_end(box, input);

   button = elm_button_add(popup);
   elm_object_text_set(button, "cancel");
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked",
                                       _edi_popup_cancel_cb, _edi_new_popup);

   button = elm_button_add(popup);
   elm_object_text_set(button, "create");
   elm_object_part_content_set(popup, "button2", button);
   evas_object_smart_callback_add(button, "clicked",
                                       _tb_new_create_cb, input);

   evas_object_show(popup);
   elm_object_focus_set(input, EINA_TRUE);
}

static void
_tb_new_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   elm_toolbar_item_selected_set(elm_toolbar_selected_item_get(obj), EINA_FALSE);

   _edi_file_new();
}

static void
_tb_save_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   elm_toolbar_item_selected_set(elm_toolbar_selected_item_get(obj), EINA_FALSE);
   edi_mainview_save();
}

static void
_tb_close_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   elm_toolbar_item_selected_set(elm_toolbar_selected_item_get(obj), EINA_FALSE);
   edi_mainview_close();
}

static void
_tb_cut_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   elm_toolbar_item_selected_set(elm_toolbar_selected_item_get(obj), EINA_FALSE);
   edi_mainview_cut();
}

static void
_tb_copy_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   elm_toolbar_item_selected_set(elm_toolbar_selected_item_get(obj), EINA_FALSE);
   edi_mainview_copy();
}

static void
_tb_paste_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   elm_toolbar_item_selected_set(elm_toolbar_selected_item_get(obj), EINA_FALSE);
   edi_mainview_paste();
}

static void
_tb_search_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   elm_toolbar_item_selected_set(elm_toolbar_selected_item_get(obj), EINA_FALSE);
   edi_mainview_search();
}

static void
_tb_goto_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   elm_toolbar_item_selected_set(elm_toolbar_selected_item_get(obj), EINA_FALSE);

   edi_mainview_goto_popup_show();
}

static Eina_Bool
_edi_build_prep(Evas_Object *button, Eina_Bool test)
{
   elm_toolbar_item_selected_set(elm_toolbar_selected_item_get(button), EINA_FALSE);

   edi_consolepanel_clear();
   if (test)
     edi_testpanel_show();
   else
     edi_consolepanel_show();

   if (!edi_builder_can_build())
     {
        edi_consolepanel_append_error_line("Cowardly refusing to build unknown project type.");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

static void
_tb_build_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   if (_edi_build_prep(obj, EINA_FALSE))
     edi_builder_build();
}

static void
_tb_test_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   if (_edi_build_prep(obj, EINA_TRUE))
     edi_builder_test();
}

static void
_tb_run_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   if (_edi_build_prep(obj, EINA_FALSE))
     _edi_launcher_run(&_edi_project_config->launch);
}

static void
_tb_clean_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   if (_edi_build_prep(obj, EINA_FALSE))
     edi_builder_clean();
}

static void
_tb_about_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   edi_about_show(_edi_main_win);
}

static void
_tb_settings_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   edi_settings_show(_edi_main_win);
}

static void
_edi_menu_new_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   _edi_file_new();
}

static void
_edi_menu_save_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                  void *event_info EINA_UNUSED)
{
   edi_mainview_save();
}

static void
_edi_menu_open_window_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                         void *event_info EINA_UNUSED)
{
   edi_mainview_new_window();
}

static void
_edi_menu_close_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   edi_mainview_close();
}

static void
_edi_menu_closeall_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   edi_mainview_closeall();
}

static void
_edi_menu_settings_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
   edi_settings_show(_edi_main_win);
}

static void
_edi_menu_quit_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                  void *event_info EINA_UNUSED)
{
   elm_exit();
}

static void
_edi_menu_cut_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   edi_mainview_cut();
}

static void
_edi_menu_copy_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                  void *event_info EINA_UNUSED)
{
   edi_mainview_copy();
}

static void
_edi_menu_paste_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   edi_mainview_paste();
}

static void
_edi_menu_find_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                  void *event_info EINA_UNUSED)
{
   edi_mainview_search();
}

static void
_edi_menu_findfile_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
   edi_filepanel_search();
}

static void
_edi_menu_goto_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                  void *event_info EINA_UNUSED)
{
   edi_mainview_goto_popup_show();
}

static void
_edi_menu_build_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   edi_builder_build();
}

static void
_edi_menu_test_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                  void *event_info EINA_UNUSED)
{
   edi_builder_test();
}

static void
_edi_menu_run_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   _edi_launcher_run(&_edi_project_config->launch);
}

static void
_edi_menu_clean_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   edi_builder_clean();
}

static void
_edi_menu_website_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   edi_open_url("http://edi-ide.com");
}

static void
_edi_menu_about_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   edi_about_show(_edi_main_win);
}

static void
_edi_menu_setup(Evas_Object *win)
{
   Evas_Object *menu;
   Elm_Object_Item *menu_it;

   menu = elm_win_main_menu_get(win);

   menu_it = elm_menu_item_add(menu, NULL, NULL, "File", NULL, NULL);
   elm_menu_item_add(menu, menu_it, "document-new", "New ...", _edi_menu_new_cb, NULL);
   elm_menu_item_add(menu, menu_it, "document-save", "Save", _edi_menu_save_cb, NULL);
   elm_menu_item_add(menu, menu_it, "window-new", "New window", _edi_menu_open_window_cb, NULL);
   elm_menu_item_add(menu, menu_it, "stock_close", "Close", _edi_menu_close_cb, NULL);
   elm_menu_item_add(menu, menu_it, "stock_close", "Close all", _edi_menu_closeall_cb, NULL);
   elm_menu_item_separator_add(menu, menu_it);
   elm_menu_item_add(menu, menu_it, "preferences-desktop", "Settings", _edi_menu_settings_cb, NULL);
   elm_menu_item_separator_add(menu, menu_it);
   elm_menu_item_add(menu, menu_it, "application-exit", "Quit", _edi_menu_quit_cb, NULL);

   menu_it = elm_menu_item_add(menu, NULL, NULL, "Edit", NULL, NULL);
   elm_menu_item_add(menu, menu_it, "edit-cut", "Cut", _edi_menu_cut_cb, NULL);
   elm_menu_item_add(menu, menu_it, "edit-copy", "Copy", _edi_menu_copy_cb, NULL);
   elm_menu_item_add(menu, menu_it, "edit-paste", "Paste", _edi_menu_paste_cb, NULL);
   elm_menu_item_separator_add(menu, menu_it);
   elm_menu_item_add(menu, menu_it, "edit-find-replace", "Find & Replace", _edi_menu_find_cb, NULL);
   elm_menu_item_add(menu, menu_it, "edit-find", "Find file", _edi_menu_findfile_cb, NULL);
   elm_menu_item_add(menu, menu_it, "go-jump", "Goto Line ...", _edi_menu_goto_cb, NULL);

   menu_it = elm_menu_item_add(menu, NULL, NULL, "Build", NULL, NULL);
   elm_menu_item_add(menu, menu_it, "system-run", "Build", _edi_menu_build_cb, NULL);
   elm_menu_item_add(menu, menu_it, "dialog-ok", "Test", _edi_menu_test_cb, NULL);
   elm_menu_item_add(menu, menu_it, "stock_media-play", "Run", _edi_menu_run_cb, NULL);
   elm_menu_item_add(menu, menu_it, "edit-clear", "Clean", _edi_menu_clean_cb, NULL);

   menu_it = elm_menu_item_add(menu, NULL, NULL, "Help", NULL, NULL);
   elm_menu_item_add(menu, menu_it, "go-home", "Website", _edi_menu_website_cb, NULL);
   elm_menu_item_separator_add(menu, menu_it);
   elm_menu_item_add(menu, menu_it, "help-about", "About", _edi_menu_about_cb, NULL);
}

static Evas_Object *
edi_toolbar_setup(Evas_Object *win)
{
   Evas_Object *box, *tb;
   Elm_Object_Item *tb_it;

   box = elm_box_add(win);
   elm_box_horizontal_set(box, EINA_TRUE);
   elm_box_homogeneous_set(box, EINA_FALSE);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, 0.0);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, 0.0);
   evas_object_show(box);

   tb = elm_toolbar_add(box);
   elm_toolbar_homogeneous_set(tb, EINA_FALSE);
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_SCROLL);
   elm_toolbar_select_mode_set(tb, ELM_OBJECT_SELECT_MODE_NONE);
   elm_toolbar_align_set(tb, 0.0);
   elm_toolbar_icon_order_lookup_set(tb, ELM_ICON_LOOKUP_FDO_THEME);
   elm_object_focus_allow_set(tb, EINA_FALSE);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, 0.0);
   evas_object_size_hint_weight_set(tb, 0.86, 0.0);

   tb_it = elm_toolbar_item_append(tb, "document-new", "New File", _tb_new_cb, NULL);
   tb_it = elm_toolbar_item_append(tb, "document-save", "Save", _tb_save_cb, NULL);
   tb_it = elm_toolbar_item_append(tb, "stock_close", "Close", _tb_close_cb, NULL);

   tb_it = elm_toolbar_item_append(tb, "separator", "", NULL, NULL);
   elm_toolbar_item_separator_set(tb_it, EINA_TRUE);

   tb_it = elm_toolbar_item_append(tb, "edit-cut", "Cut", _tb_cut_cb, NULL);
   tb_it = elm_toolbar_item_append(tb, "edit-copy", "Copy", _tb_copy_cb, NULL);
   tb_it = elm_toolbar_item_append(tb, "edit-paste", "Paste", _tb_paste_cb, NULL);

   tb_it = elm_toolbar_item_append(tb, "separator", "", NULL, NULL);
   elm_toolbar_item_separator_set(tb_it, EINA_TRUE);

   tb_it = elm_toolbar_item_append(tb, "edit-find-replace", "Find & Replace", _tb_search_cb, NULL);
   tb_it = elm_toolbar_item_append(tb, "go-jump", "Goto Line", _tb_goto_cb, NULL);

   tb_it = elm_toolbar_item_append(tb, "separator", "", NULL, NULL);
   elm_toolbar_item_separator_set(tb_it, EINA_TRUE);

   tb_it = elm_toolbar_item_append(tb, "system-run", "Build", _tb_build_cb, NULL);
   tb_it = elm_toolbar_item_append(tb, "dialog-ok", "Test", _tb_test_cb, NULL);
   tb_it = elm_toolbar_item_append(tb, "stock_media-play", "Run", _tb_run_cb, NULL);
   tb_it = elm_toolbar_item_append(tb, "edit-clear", "Clean", _tb_clean_cb, NULL);

   elm_box_pack_end(box, tb);
   evas_object_show(tb);

   tb = elm_toolbar_add(box);
   elm_toolbar_homogeneous_set(tb, EINA_FALSE);
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_SCROLL);
   elm_toolbar_select_mode_set(tb, ELM_OBJECT_SELECT_MODE_NONE);
   elm_toolbar_align_set(tb, 1.0);
   elm_toolbar_icon_order_lookup_set(tb, ELM_ICON_LOOKUP_FDO_THEME);
   elm_object_focus_allow_set(tb, EINA_FALSE);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, 0.0);
   evas_object_size_hint_weight_set(tb, 0.14, 0.0);

   tb_it = elm_toolbar_item_append(tb, "preferences-desktop", "Settings", _tb_settings_cb, NULL);
   tb_it = elm_toolbar_item_append(tb, "help-about", "About", _tb_about_cb, NULL);

   elm_box_pack_end(box, tb);
   evas_object_show(tb);
   return box;
}

static char *
_edi_win_title_get(const char *path)
{
   char *winname, *dirname;

   dirname = basename((char*)path);
   winname = malloc((8 + strlen(dirname)) * sizeof(char));
   snprintf(winname, 8 + strlen(dirname), "Edi :: %s", dirname);

   return winname;
}

static void
_edi_exit(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   edi_close();
}

static void
_edi_resize_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj,
                           void *event_info EINA_UNUSED)
{
   int w, h;

   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   w /= elm_config_scale_get();
   h /= elm_config_scale_get();

   _edi_project_config->gui.width = w + 1;
   _edi_project_config->gui.height = h + 1;
   _edi_project_config_save();
}

static void
_edi_toolbar_set_visible(Eina_Bool visible)
{
   elm_box_unpack(_edi_main_box, _edi_toolbar);
   if (visible)
     evas_object_show(_edi_toolbar);
   else
     evas_object_hide(_edi_toolbar);

   if (visible)
     elm_box_pack_start(_edi_main_box, _edi_toolbar);
}

static Eina_Bool
_edi_config_changed(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   _edi_toolbar_set_visible(!_edi_project_config->gui.toolbar_hidden);
   return ECORE_CALLBACK_RENEW;
}

void
_edi_open_tabs()
{
   Edi_Project_Config_Tab *tab;
   Eina_List *tabs, *list;

   tabs = _edi_project_config->tabs;
   _edi_project_config->tabs = NULL;
   EINA_LIST_FOREACH(tabs, list, tab)
     {
        if (tab->windowed)
          edi_mainview_open_window_path(tab->path);
        else
          edi_mainview_open_path(tab->path);
     }

   EINA_LIST_FREE(tabs, tab)
     {
        free(tab);
     }
}

Evas_Object *
edi_open(const char *inputpath)
{
   Evas_Object *win, *vbx, *content, *tb;
   const char *winname;
   char *path;

   if (!edi_project_set(inputpath))
     {
        fprintf(stderr, "Project path must be a directory\n");
        return NULL;
     }
   path = realpath(inputpath, NULL);
   _edi_project_config_load();

   elm_need_ethumb();
   elm_need_efreet();

   winname = _edi_win_title_get(path);
   win = elm_win_util_standard_add("main", winname);
   free((char*)winname);
   if (!win) return NULL;

   _edi_main_win = win;
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   evas_object_smart_callback_add(win, "delete,request", _edi_exit, NULL);
   evas_object_event_callback_add(win, EVAS_CALLBACK_RESIZE, _edi_resize_cb, NULL);

   vbx = elm_box_add(win);
   _edi_main_box = vbx;
   evas_object_size_hint_weight_set(vbx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, vbx);
   evas_object_show(vbx);

   tb = edi_toolbar_setup(win);
   _edi_toolbar = tb;
   _edi_toolbar_set_visible(!_edi_project_config->gui.toolbar_hidden);
   _edi_menu_setup(win);

   content = edi_content_setup(vbx, path);
   evas_object_size_hint_weight_set(content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(content, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(vbx, content);

   ERR("Loaded project at %s", path);
   evas_object_resize(win, _edi_project_config->gui.width * elm_config_scale_get(),
                      _edi_project_config->gui.height * elm_config_scale_get());
   evas_object_show(win);

   _edi_config_project_add(path);
   _edi_open_tabs();
   ecore_event_handler_add(EDI_EVENT_CONFIG_CHANGED, _edi_config_changed, NULL);

   free(path);
   return win;
}

void
edi_close()
{
   elm_exit();
}

void
edi_open_url(const char *url)
{
   const char *format;
   char *cmd;

   format = "xdg-open \"%s\"";

   cmd = malloc(sizeof(char) * (strlen(format) + strlen(url) - 1));
   sprintf(cmd, format, url);
   ecore_exe_run(cmd, NULL);

   free(cmd);
}

static Eina_Bool
_edi_log_init()
{
   _edi_log_dom = eina_log_domain_register("edi", EINA_COLOR_GREEN);
   if (_edi_log_dom < 0)
     {
        EINA_LOG_ERR("Edi can not create its log domain.");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

static void
_edi_log_shutdown()
{
   eina_log_domain_unregister(_edi_log_dom);
   _edi_log_dom = -1;
}

static const Ecore_Getopt optdesc = {
  "edi",
  "%prog [options] [project-dir]",
  PACKAGE_VERSION,
  COPYRIGHT,
  "GPLv2",
  "The Enlightened IDE",
  EINA_TRUE,
  {
    ECORE_GETOPT_LICENSE('L', "license"),
    ECORE_GETOPT_COPYRIGHT('C', "copyright"),
    ECORE_GETOPT_VERSION('V', "version"),
    ECORE_GETOPT_HELP('h', "help"),
    ECORE_GETOPT_SENTINEL
  }
};

EAPI_MAIN int
elm_main(int argc EINA_UNUSED, char **argv EINA_UNUSED)
{
   Evas_Object *win;
   int args;
   Eina_Bool quit_option = EINA_FALSE;
   const char *project_path = NULL;

   Ecore_Getopt_Value values[] = {
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_NONE
   };

#if ENABLE_NLS
   setlocale(LC_ALL, "");
   bindtextdomain(PACKAGE, LOCALEDIR);
   bind_textdomain_codeset(PACKAGE, "UTF-8");
   textdomain(PACKAGE);
#endif

   if (!_edi_config_init())
     goto config_error;

   edi_init();
   if (!_edi_log_init())
     goto end;
   elm_code_init();

   args = ecore_getopt_parse(&optdesc, values, argc, argv);
   if (args < 0)
     {
        EINA_LOG_CRIT("Could not parse arguments.");
        goto end;
     }
   else if (quit_option)
     {
        goto end;
     }

   if (args < argc)
     {
        project_path = argv[args];
     }

   /* tell elm about our app so it can figure out where to get files */
   elm_app_compile_bin_dir_set(PACKAGE_BIN_DIR);
   elm_app_compile_lib_dir_set(PACKAGE_LIB_DIR);
   elm_app_compile_data_dir_set(PACKAGE_DATA_DIR);
   elm_app_info_set(elm_main, "edi", "images/edi.png");

   if (!project_path)
     {
        if (!edi_welcome_show())
          goto end;
     }
   else if (!(win = edi_open(project_path)))
     goto end;

   elm_run();

 end:
   _edi_log_shutdown();
   elm_code_shutdown();
   elm_shutdown();
   edi_shutdown();

 config_error:
   _edi_config_shutdown();

   return 0;
}
ELM_MAIN()
