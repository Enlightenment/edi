#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* NOTE: Respecting header order is important for portability.
 * Always put system first, then EFL, then your public header,
 * and finally your private one. */

#include <Ecore_Getopt.h>
#include <Elementary.h>
#include <Eio.h>

#include "gettext.h"

#include "Edi.h"
#include "edi_filepanel.h"
#include "edi_logpanel.h"
#include "edi_consolepanel.h"
#include "mainview/edi_mainview.h"
#include "welcome/edi_welcome.h"

#include "edi_private.h"

#define COPYRIGHT "Copyright © 2014 Andy Williams <andy@andyilliams.me> and various contributors (see AUTHORS)."

static Evas_Object *_edi_leftpanes, *_edi_bottompanes;
static Evas_Object *_edi_logpanel, *_edi_consolepanel, *_edi_testpanel;
static Elm_Object_Item *_edi_logpanel_item, *_edi_consolepanel_item, *_edi_testpanel_item;
static Elm_Object_Item *_edi_selected_bottompanel;
static Evas_Object *_edi_filepanel, *_edi_filepanel_icon;

static Evas_Object *_edi_main_win, *_edi_new_popup, *_edi_goto_popup;

static void
_edi_file_open_cb(const char *path, const char *type, Eina_Bool newwin)
{
   Edi_Path_Options *options;

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

static void
_edi_toggle_file_panel(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *panel;
   double size;

   panel = (Evas_Object *)data;
   size = elm_panes_content_left_size_get(_edi_leftpanes);

   if (size == 0.0)
     {
        evas_object_show(panel);
        elm_icon_standard_set(_edi_filepanel_icon, "stock_left");
        elm_panes_content_left_size_set(_edi_leftpanes, 0.25);
     }
   else
     {
        elm_icon_standard_set(_edi_filepanel_icon, "stock_right");
        elm_panes_content_left_size_set(_edi_leftpanes, 0.0);
        evas_object_hide(panel);
     }
}

static void
_edi_toggle_panel(void *data, Evas_Object *obj, void *event_info)
{
   double size;
   Elm_Object_Item *item;
   Evas_Object *panel;

   panel = (Evas_Object *) data;
   item = (Elm_Object_Item *) event_info;
   if (obj)
     elm_object_focus_set(obj, EINA_FALSE);

   evas_object_hide(_edi_logpanel);
   evas_object_hide(_edi_consolepanel);
   evas_object_hide(_edi_testpanel);

   if (item == _edi_selected_bottompanel)
     {
        elm_toolbar_item_icon_set(item, "stock_up");
        elm_panes_content_right_size_set(_edi_bottompanes, 0.0);

        _edi_selected_bottompanel = NULL;
        elm_toolbar_item_selected_set(item, EINA_FALSE);
     }
   else
     {
        if (_edi_selected_bottompanel)
          elm_toolbar_item_icon_set(_edi_selected_bottompanel, "stock_up");

        elm_toolbar_item_icon_set(item, "stock_down");
        evas_object_show(panel);

        size = elm_panes_content_right_size_get(_edi_bottompanes);
        if (size == 0.0)
          elm_panes_content_right_size_set(_edi_bottompanes, 0.2);

        _edi_selected_bottompanel = item;
        elm_toolbar_item_selected_set(item, EINA_TRUE);
     }
   elm_toolbar_select_mode_set(obj, ELM_OBJECT_SELECT_MODE_ALWAYS);
}

void edi_consolepanel_show()
{
   elm_toolbar_item_selected_set(_edi_consolepanel_item, EINA_TRUE);
}

void edi_testpanel_show()
{
   elm_toolbar_item_selected_set(_edi_testpanel_item, EINA_TRUE);
}

static Evas_Object *
edi_content_setup(Evas_Object *win, const char *path)
{
   Evas_Object *filepane, *logpane, *logpanels, *content_out, *content_in, *tb;
   Evas_Object *icon, *button;

   filepane = elm_panes_add(win);
   evas_object_size_hint_weight_set(filepane, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   logpane = elm_panes_add(filepane);
   elm_panes_horizontal_set(logpane, EINA_TRUE);
   evas_object_size_hint_weight_set(logpane, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   _edi_filepanel = elm_box_add(win);
   _edi_logpanel = elm_box_add(win);
   _edi_consolepanel = elm_box_add(win);
   _edi_testpanel = elm_box_add(win);

   // add main content
   content_out = elm_box_add(win);
   elm_box_horizontal_set(content_out, EINA_FALSE);
   evas_object_size_hint_weight_set(content_out, 0.8, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(content_out, EVAS_HINT_FILL, EVAS_HINT_FILL);

   content_in = elm_box_add(content_out);
   elm_box_horizontal_set(content_in, EINA_TRUE);
   evas_object_size_hint_weight_set(content_in, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(content_in, EVAS_HINT_FILL, EVAS_HINT_FILL);

   icon = elm_icon_add(content_in);
   elm_icon_standard_set(icon, "stock_left");
   button = elm_button_add(content_in);
   elm_object_part_content_set(button, "icon", icon);
   elm_object_focus_allow_set(button, EINA_FALSE);
   _edi_filepanel_icon = icon;

   evas_object_smart_callback_add(button, "clicked",
                                  _edi_toggle_file_panel, _edi_filepanel);
   evas_object_size_hint_align_set(button, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(content_in, button);
   evas_object_show(button);

   elm_box_pack_end(content_out, content_in);
   edi_mainview_add(content_in, win);
   evas_object_show(content_in);

   elm_object_part_content_set(filepane, "right", content_out);
   elm_object_part_content_set(logpane, "top", filepane);
   evas_object_show(logpane);
   _edi_leftpanes = filepane;

   // add file list
   evas_object_size_hint_weight_set(_edi_filepanel, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(_edi_filepanel, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_panes_content_left_size_set(filepane, 0.25);

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
                                                _edi_toggle_panel, _edi_logpanel);
   _edi_consolepanel_item = elm_toolbar_item_append(tb, "stock_up", "Console",
                                                    _edi_toggle_panel, _edi_consolepanel);
   _edi_testpanel_item = elm_toolbar_item_append(tb, "stock_up", "Tests",
                                                 _edi_toggle_panel, _edi_testpanel);

   // add lower panel panes
   logpanels = elm_table_add(logpane);
   evas_object_size_hint_weight_set(_edi_logpanel, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(_edi_logpanel, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(_edi_logpanel);

   edi_logpanel_add(_edi_logpanel);
   elm_table_pack(logpanels, _edi_logpanel, 0, 0, 1, 1);
   evas_object_hide(_edi_logpanel);

   evas_object_size_hint_weight_set(_edi_consolepanel, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(_edi_consolepanel, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(_edi_consolepanel);

   edi_consolepanel_add(_edi_consolepanel);
   elm_table_pack(logpanels, _edi_consolepanel, 0, 0, 1, 1);
   evas_object_hide(_edi_consolepanel);

   evas_object_size_hint_weight_set(_edi_testpanel, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(_edi_testpanel, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(_edi_testpanel);

   edi_testpanel_add(_edi_testpanel);
   elm_table_pack(logpanels, _edi_testpanel, 0, 0, 1, 1);
   elm_object_part_content_set(logpane, "bottom", logpanels);
   elm_panes_content_right_size_set(logpane, 0.0);
   evas_object_hide(_edi_testpanel);
   evas_object_show(logpane);
   _edi_bottompanes = logpane;

   return logpane;
}

static void
_tb_new_create_cb(void *data,
                             Evas_Object *obj EINA_UNUSED,
                             void *event_info EINA_UNUSED)
{
   const char *path, *name;

   name = elm_entry_entry_get((Evas_Object *) data);
   path = edi_project_file_path_get(name);

   fclose(fopen(path, "w"));
   edi_mainview_open_path(path);

   evas_object_del(_edi_new_popup);
   free((char*)path);
}

static void
_tb_new_cancel_cb(void *data EINA_UNUSED,
                                   Evas_Object *obj EINA_UNUSED,
                                   void *event_info EINA_UNUSED)
{
   evas_object_del(_edi_new_popup);
}

static void
_tb_new_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *popup, *input, *button;

   elm_toolbar_item_selected_set(elm_toolbar_selected_item_get(obj), EINA_FALSE);

   popup = elm_popup_add(_edi_main_win);
   _edi_new_popup = popup;
   elm_object_part_text_set(popup, "title,text",
                            "Enter new file name");

   input = elm_entry_add(popup);
   elm_entry_single_line_set(input, EINA_TRUE);
   elm_object_content_set(popup, input);

   button = elm_button_add(popup);
   elm_object_text_set(button, "cancel");
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked",
                                       _tb_new_cancel_cb, NULL);

   button = elm_button_add(popup);
   elm_object_text_set(button, "create");
   elm_object_part_content_set(popup, "button2", button);
   evas_object_smart_callback_add(button, "clicked",
                                       _tb_new_create_cb, input);

   evas_object_show(popup);
}

static void
_tb_save_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   elm_toolbar_item_selected_set(elm_toolbar_selected_item_get(obj), EINA_FALSE);
   edi_mainview_save();
}

static void
_tb_open_window_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   elm_toolbar_item_selected_set(elm_toolbar_selected_item_get(obj), EINA_FALSE);
   edi_mainview_new_window();
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
_tb_goto_go_cb(void *data,
                             Evas_Object *obj EINA_UNUSED,
                             void *event_info EINA_UNUSED)
{
   int number;

   number = atoi(elm_entry_entry_get((Evas_Object *) data));
   edi_mainview_goto(number);

   evas_object_del(_edi_goto_popup);
}

static void
_tb_goto_cancel_cb(void *data EINA_UNUSED,
                                   Evas_Object *obj EINA_UNUSED,
                                   void *event_info EINA_UNUSED)
{
   evas_object_del(_edi_goto_popup);
}

static void
_tb_goto_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *popup, *input, *button;

   elm_toolbar_item_selected_set(elm_toolbar_selected_item_get(obj), EINA_FALSE);

   popup = elm_popup_add(_edi_main_win);
   _edi_goto_popup = popup;
   elm_object_part_text_set(popup, "title,text",
                            "Enter line number");

   input = elm_entry_add(popup);
   elm_entry_single_line_set(input, EINA_TRUE);
   elm_object_content_set(popup, input);

   button = elm_button_add(popup);
   elm_object_text_set(button, "cancel");
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked",
                                       _tb_goto_cancel_cb, NULL);

   button = elm_button_add(popup);
   elm_object_text_set(button, "go");
   elm_object_part_content_set(popup, "button2", button);
   evas_object_smart_callback_add(button, "clicked",
                                       _tb_goto_go_cb, input);

   evas_object_show(popup);
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
/*
static void
_tb_run_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   if (_edi_build_prep(obj, EINA_FALSE))
     edi_builder_run();
}
*/
static void
_tb_clean_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   if (_edi_build_prep(obj, EINA_FALSE))
     edi_builder_clean();
}

static Evas_Object *
edi_toolbar_setup(Evas_Object *win)
{
   Evas_Object *tb;
   Elm_Object_Item *tb_it;

   tb = elm_toolbar_add(win);
   elm_toolbar_homogeneous_set(tb, EINA_FALSE);
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_SCROLL);
   elm_toolbar_select_mode_set(tb, ELM_OBJECT_SELECT_MODE_NONE);
   elm_toolbar_align_set(tb, 0.0);
   elm_object_focus_allow_set(tb, EINA_FALSE);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, 0.0);

   tb_it = elm_toolbar_item_append(tb, "document-new", "New File", _tb_new_cb, NULL);
   tb_it = elm_toolbar_item_append(tb, "document-save", "Save", _tb_save_cb, NULL);
   tb_it = elm_toolbar_item_append(tb, "window-new", "New window", _tb_open_window_cb, NULL);
   tb_it = elm_toolbar_item_append(tb, "window-close", "Close", _tb_close_cb, NULL);

   tb_it = elm_toolbar_item_append(tb, "separator", "", NULL, NULL);
   elm_toolbar_item_separator_set(tb_it, EINA_TRUE);

   tb_it = elm_toolbar_item_append(tb, "edit-cut", "Cut", _tb_cut_cb, NULL);
   tb_it = elm_toolbar_item_append(tb, "edit-copy", "Copy", _tb_copy_cb, NULL);
   tb_it = elm_toolbar_item_append(tb, "edit-paste", "Paste", _tb_paste_cb, NULL);

   tb_it = elm_toolbar_item_append(tb, "separator", "", NULL, NULL);
   elm_toolbar_item_separator_set(tb_it, EINA_TRUE);

   tb_it = elm_toolbar_item_append(tb, "find", "Find", _tb_search_cb, NULL);
   tb_it = elm_toolbar_item_append(tb, "go-jump", "Goto Line", _tb_goto_cb, NULL);

   tb_it = elm_toolbar_item_append(tb, "separator", "", NULL, NULL);
   elm_toolbar_item_separator_set(tb_it, EINA_TRUE);

   tb_it = elm_toolbar_item_append(tb, "system-run", "Build", _tb_build_cb, NULL);
   tb_it = elm_toolbar_item_append(tb, "emblem-default", "Test", _tb_test_cb, NULL);
//   tb_it = elm_toolbar_item_append(tb, "player-play", "Run", _tb_run_cb, NULL);
   tb_it = elm_toolbar_item_append(tb, "edit-clear", "Clean", _tb_clean_cb, NULL);

   evas_object_show(tb);
   return tb;
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

EAPI Evas_Object *
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

   elm_need_ethumb();
   elm_need_efreet();

   winname = _edi_win_title_get(path);
   win = elm_win_util_standard_add("main", winname);
   free((char*)winname);
   if (!win) return NULL;

   _edi_main_win = win;
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   evas_object_smart_callback_add(win, "delete,request", _edi_exit, NULL);

   vbx = elm_box_add(win);
   evas_object_size_hint_weight_set(vbx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, vbx);
   evas_object_show(vbx);

   tb = edi_toolbar_setup(win);
   elm_box_pack_end(vbx, tb);

   content = edi_content_setup(vbx, path);
   evas_object_size_hint_weight_set(content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(content, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(vbx, content);

   ERR("Loaded project at %s", path);
   evas_object_resize(win, 560 * elm_config_scale_get(), 420 * elm_config_scale_get());
   evas_object_show(win);

   free(path);
   return win;
}

EAPI void
edi_close()
{
   elm_exit();
}

static const Ecore_Getopt optdesc = {
  "edi",
  "%prog [options] [project-dir]",
  PACKAGE_VERSION,
  COPYRIGHT,
  "BSD with advertisement clause",
  "The EFL IDE",
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

   edi_init();

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
   edi_shutdown();
   elm_shutdown();

   return 0;
}
ELM_MAIN()
