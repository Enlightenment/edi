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
#include "edi_mainview.h"

#include "edi_private.h"

#define COPYRIGHT "Copyright © 2014 Andy Williams <andy@andyilliams.me> and various contributors (see AUTHORS)."

static Evas_Object *_edi_filepanel, *_edi_logpanel, *_edi_consolepanel;
static Evas_Object *_edi_main_win, *_edi_new_popup;
static const char *_edi_projectpath;

static Evas_Object *edi_win_setup(const char *path);

static void
_edi_exit(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_exit();
}

static void
_edi_file_open_cb(const char *path, const char *type, Eina_Bool newwin)
{
   if (type == NULL)
     INF("Opening %s", path);
   else
     INF("Opening %s as %s", path, type);

   if (newwin)
     edi_mainview_open_window_path(path, type);
   else
     edi_mainview_open_path(path, type);
}

static void
_edi_toggle_panel(void *data, Evas_Object *obj,
                      void *event_info EINA_UNUSED)
{
   elm_object_focus_set(obj, EINA_FALSE);
   elm_panel_toggle((Evas_Object *) data);
}

void edi_consolepanel_show()
{
   elm_panel_hidden_set(_edi_consolepanel, EINA_FALSE);
}

static Evas_Object *
edi_content_setup(Evas_Object *win, const char *path)
{
   Evas_Object *panes, *content_out, *content_in, *tb, *button;

   panes = elm_table_add(win);
   evas_object_size_hint_weight_set(panes, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   _edi_filepanel = elm_panel_add(win);
   _edi_logpanel = elm_panel_add(win);
   _edi_consolepanel = elm_panel_add(win);

   // add main content
   content_out = elm_box_add(win);
   elm_box_horizontal_set(content_out, EINA_FALSE);
   evas_object_size_hint_weight_set(content_out, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(content_out, EVAS_HINT_FILL, EVAS_HINT_FILL);

   content_in = elm_box_add(content_out);
   elm_box_horizontal_set(content_in, EINA_TRUE);
   evas_object_size_hint_weight_set(content_in, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(content_in, EVAS_HINT_FILL, EVAS_HINT_FILL);

   button = elm_button_add(content_in);
   elm_object_text_set(button, "    ");
   evas_object_smart_callback_add(button, "clicked",
                                  _edi_toggle_panel, _edi_filepanel);
   evas_object_size_hint_align_set(button, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(content_in, button);
   evas_object_show(button);

   elm_box_pack_end(content_out, content_in);
   edi_mainview_add(content_in, win);
   evas_object_show(content_in);

   elm_table_pack(panes, content_out, 0, 0, 6, 5);
   evas_object_show(content_out);

   // add file list
   elm_panel_orient_set(_edi_filepanel, ELM_PANEL_ORIENT_LEFT);
   evas_object_size_hint_weight_set(_edi_filepanel, 0.3, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(_edi_filepanel, EVAS_HINT_FILL, EVAS_HINT_FILL);

   elm_panel_hidden_set(_edi_filepanel, EINA_FALSE);
   edi_filepanel_add(_edi_filepanel, win, path, _edi_file_open_cb);
   elm_table_pack(panes, _edi_filepanel, 0, 0, 1, 5);
   evas_object_show(_edi_filepanel);

   // add lower panel
   tb = elm_toolbar_add(content_out);
   evas_object_size_hint_weight_set(tb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_toolbar_homogeneous_set(tb, EINA_FALSE);
   elm_toolbar_align_set(tb, 1.0);
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_SCROLL);
   elm_toolbar_select_mode_set(tb, ELM_OBJECT_SELECT_MODE_ALWAYS);
   elm_box_pack_end(content_out, tb);
   evas_object_show(tb);

   elm_toolbar_item_append(tb, NULL, "Logs", _edi_toggle_panel, _edi_logpanel);
   elm_toolbar_item_append(tb, NULL, "Console", _edi_toggle_panel, _edi_consolepanel);

   elm_panel_orient_set(_edi_logpanel, ELM_PANEL_ORIENT_BOTTOM);
   evas_object_size_hint_weight_set(_edi_logpanel, EVAS_HINT_EXPAND, 0.15);
   evas_object_size_hint_align_set(_edi_logpanel, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(_edi_logpanel);

   elm_panel_hidden_set(_edi_logpanel, EINA_FALSE);
   elm_panel_hidden_set(_edi_logpanel, EINA_TRUE);
   edi_logpanel_add(_edi_logpanel);
   elm_table_pack(panes, _edi_logpanel, 0, 4, 6, 1);
   evas_object_show(_edi_logpanel);

   elm_panel_orient_set(_edi_consolepanel, ELM_PANEL_ORIENT_BOTTOM);
   evas_object_size_hint_weight_set(_edi_consolepanel, EVAS_HINT_EXPAND, 0.15);
   evas_object_size_hint_align_set(_edi_consolepanel, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(_edi_consolepanel);

   elm_panel_hidden_set(_edi_consolepanel, EINA_FALSE);
   elm_panel_hidden_set(_edi_consolepanel, EINA_TRUE);
   edi_consolepanel_add(_edi_consolepanel);
   elm_table_pack(panes, _edi_consolepanel, 0, 4, 6, 1);
   evas_object_show(_edi_consolepanel);

   evas_object_show(panes);
   return panes;
}

static void
_tb_new_create_cb(void *data,
                             Evas_Object *obj EINA_UNUSED,
                             void *event_info EINA_UNUSED)
{
   const char *path, *name;
   int len;

   name = elm_entry_entry_get((Evas_Object *) data);

   len = strlen(name) + strlen(_edi_projectpath) + 2;
   path = malloc(sizeof(char) * len);
   snprintf(path, len, "%s/%s", _edi_projectpath, name);

   fclose(fopen(path, "w"));
   edi_mainview_open_path(path, NULL);

   evas_object_del(_edi_new_popup);
   free(path);
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

   // popup title
   elm_object_part_text_set(popup, "title,text",
                            "Enter new file name");

   input = elm_entry_add(popup);
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
_tb_build_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   unsigned int printed, buffer_len = 512;
   char buffer [buffer_len];
   FILE *pf;

   elm_toolbar_item_selected_set(elm_toolbar_selected_item_get(obj), EINA_FALSE);

   edi_consolepanel_clear();
   edi_consolepanel_show();

   ecore_exe_pipe_run("make", ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR, NULL);
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

   tb_it = elm_toolbar_item_append(tb, "system-run", "Build", _tb_build_cb, NULL);

   evas_object_show(tb);
   return tb;
}

static void
_edi_project_chosen_cb(void *data,
                       Evas_Object *obj EINA_UNUSED,
                       void *event_info)
{
   evas_object_del(data);

   if (event_info)
     {
        _edi_projectpath = event_info;
        edi_win_setup(_edi_projectpath);
     }
   else
     elm_exit();
}

static Evas_Object *
_edi_project_choose()
{
   Evas_Object *win, *fs;

   elm_need_ethumb();
   elm_need_efreet();

   win = elm_win_util_standard_add("projectselector", "Choose a Project Folder");
   if (!win) return NULL;

   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   evas_object_smart_callback_add(win, "delete,request", _edi_exit, NULL);

   fs = elm_fileselector_add(win);
   evas_object_size_hint_weight_set(fs, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(fs, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(fs, "done", _edi_project_chosen_cb, win);
   elm_win_resize_object_add(win, fs);
   evas_object_show(fs);

   elm_fileselector_expandable_set(fs, EINA_TRUE);
   elm_fileselector_folder_only_set(fs, EINA_TRUE);
   elm_fileselector_path_set(fs, getenv("HOME"));
   elm_fileselector_sort_method_set(fs, ELM_FILESELECTOR_SORT_BY_FILENAME_ASC);

   evas_object_resize(win, 380, 260);
   evas_object_show(win);

   return win;
}

static char *
_edi_win_title_get(const char *path)
{
   char *winname, *dirname;

   dirname = basename(path);
   winname = malloc((8 + strlen(dirname)) * sizeof(char));
   snprintf(winname, 8 + strlen(dirname), "Edi :: %s", dirname);

   return winname;
}

static Evas_Object *
edi_win_setup(const char *path)
{
   Evas_Object *win, *vbx, *content, *tb;
   const char *winname;

   if (!path) return _edi_project_choose();
   _edi_projectpath = path;

   elm_need_ethumb();
   elm_need_efreet();

   winname = _edi_win_title_get(path);
   win = elm_win_util_standard_add("main", winname);
   free(winname);
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
   evas_object_resize(win, 560, 420);
   evas_object_show(win);

   return win;
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
        project_path = realpath(argv[args], NULL);
     }

   elm_app_info_set(elm_main, "edi", "images/edi.png");

   if (!(win = edi_win_setup(project_path)))
     goto end;

   edi_library_call();

   elm_run();

 end:
   edi_shutdown();
   elm_shutdown();

   return 0;
}
ELM_MAIN()
