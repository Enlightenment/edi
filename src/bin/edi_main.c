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
#include "edi_mainview.h"

#include "edi_private.h"

#define COPYRIGHT "Copyright © 2014 Andy Williams <andy@andyilliams.me> and various contributors (see AUTHORS)."

static Evas_Object *edi_win_setup(const char *path);

static void
_edi_exit(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_exit();
}

static void
_edi_file_open_cb(const char *path, const char *type)
{
   if (type == NULL)
     INF("Opening %s", path);
   else
     INF("Opening %s as %s", path, type);

   edi_mainview_open_path(path, type);
}

static Evas_Object *
edi_content_setup(Evas_Object *win, const char *path)
{
   Evas_Object *panes, *panes_h, *panel;

   panes = elm_panes_add(win);
   evas_object_size_hint_weight_set(panes, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   // add panes
   panes_h = elm_panes_add(win);
   elm_panes_horizontal_set(panes_h, EINA_TRUE);
   evas_object_size_hint_weight_set(panes_h, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(panes_h, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(panes_h);
   elm_panes_content_right_size_set(panes_h, 0.15);
   elm_object_part_content_set(panes, "right", panes_h);

   // add lower panel
   panel = elm_box_add(win);
   evas_object_size_hint_weight_set(panel, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(panel, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(panel);
   edi_logpanel_add(panel);

   elm_object_part_content_set(panes_h, "bottom", panel);

   // add main content
   panel = elm_box_add(win);
   evas_object_size_hint_weight_set(panel, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(panel, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(panel);
   edi_mainview_add(panel);

   elm_object_part_content_set(panes_h, "top", panel);

   // add file list
   panel = elm_box_add(win);
   evas_object_size_hint_weight_set(panel, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(panel, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(panel);
   edi_filepanel_add(panel, win, path, _edi_file_open_cb);

   elm_object_part_content_set(panes, "left", panel);
   elm_panes_content_left_size_set(panes, 0.2);

   
   evas_object_show(panes);
   return panes;
}

static void
_tb_save_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   edi_mainview_save();
}

static void
_tb_close_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   edi_mainview_close();
}

static void
_tb_cut_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   edi_mainview_cut();
}

static void
_tb_copy_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   edi_mainview_copy();
}

static void
_tb_paste_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   edi_mainview_paste();
}

static Evas_Object *
edi_toolbar_setup(Evas_Object *win)
{
   Evas_Object *tb;
   Elm_Object_Item *tb_it;

   tb = elm_toolbar_add(win);
   elm_toolbar_homogeneous_set(tb, EINA_TRUE);
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_SCROLL);
   elm_toolbar_align_set(tb, 0.0);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, 0.0);

   tb_it = elm_toolbar_item_append(tb, "save", "Save", _tb_save_cb, NULL);
   tb_it = elm_toolbar_item_append(tb, "close", "Close", _tb_close_cb, NULL);

   tb_it = elm_toolbar_item_append(tb, "separator", "", NULL, NULL);
   elm_toolbar_item_separator_set(tb_it, EINA_TRUE);

   tb_it = elm_toolbar_item_append(tb, "edit-cut", "Cut", _tb_cut_cb, NULL);
   tb_it = elm_toolbar_item_append(tb, "edit-copy", "Copy", _tb_copy_cb, NULL);
   tb_it = elm_toolbar_item_append(tb, "edit-paste", "Paste", _tb_paste_cb, NULL);
   
   evas_object_show(tb);
   return tb;
}

static void
_edi_project_chosen_cb(void *data,
                       Evas_Object *obj EINA_UNUSED,
                       void *event_info)
{
    const char *selected;

    evas_object_del(data);

   selected = event_info;
    if (selected)
      edi_win_setup(selected);
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

   elm_need_ethumb();
   elm_need_efreet();

   winname = _edi_win_title_get(path);
   win = elm_win_util_standard_add("main", winname);
   free(winname);
   if (!win) return NULL;

   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   evas_object_smart_callback_add(win, "delete,request", _edi_exit, NULL);

   vbx = elm_box_add(win);
   evas_object_size_hint_weight_set(vbx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, vbx);
   evas_object_show(vbx);

   tb = edi_toolbar_setup(win);
   elm_box_pack_end(vbx, tb);

   content = edi_content_setup(win, path);
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
