#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* NOTE: Respecting header order is important for portability.
 * Always put system first, then EFL, then your public header,
 * and finally your private one. */

#include <Ecore_Getopt.h>
#include <Elementary.h>

#include "gettext.h"

#include "Edi.h"

#include "edi_private.h"

#define COPYRIGHT "Copyright © 2014 Andy Williams <andy@andyilliams.me> and various contributors (see AUTHORS)."

static Elm_Genlist_Item_Class itc;

static void
_edi_win_del(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_exit();
}

static char *
_text_get(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, const char *source EINA_UNUSED)
{
   return strdup(data);
}

static Evas_Object *
_content_get(void *data, Evas_Object *obj, const char *source)
{
   if (!strcmp(source, "elm.swallow.icon"))
     {
        Evas_Object *ic;

        ic = elm_icon_add(obj);
        if (!strcmp(data, "Dir"))
          elm_icon_standard_set(ic, "folder");
        else
          elm_icon_standard_set(ic, "file");
        evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
        evas_object_show(ic);
        return ic;
     }
   return NULL;
}

static Evas_Object *
edi_filetree_setup(Evas_Object *win)
{
   Evas_Object *list;

   list = elm_genlist_add(win);
   evas_object_size_hint_min_set(list, 100, -1);
   evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(list);

   itc.item_style = "default";
   itc.func.text_get = _text_get;
   itc.func.content_get = _content_get;
//   itc.func.state_get = _state_get;
//   itc.func.del = _item_del;

   elm_genlist_item_append(list, &itc, "File 1",
			   NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
   elm_genlist_item_append(list, &itc, "File 2",
			   NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
   elm_genlist_item_append(list, &itc, "Dir",
			   NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
                                  
   return list;
}

static Evas_Object *
edi_content_setup(Evas_Object *win)
{
   Evas_Object *panes, *panes_h, *txt;

   panes = elm_panes_add(win);
   evas_object_size_hint_weight_set(panes, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   // add file list
   elm_object_part_content_set(panes, "left", edi_filetree_setup(win));
   elm_panes_content_left_size_set(panes, 0.2);

   // add panes
   panes_h = elm_panes_add(win);
   elm_panes_horizontal_set(panes_h, EINA_TRUE);
   evas_object_size_hint_weight_set(panes_h, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(panes_h, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(panes_h);
   elm_panes_content_right_size_set(panes_h, 0.15);
   elm_object_part_content_set(panes, "right", panes_h);

   // add main content
   txt = elm_entry_add(win);
   elm_entry_scrollable_set(txt, EINA_TRUE);
   elm_object_text_set(txt, "Main");
   evas_object_size_hint_weight_set(txt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(txt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(txt);
   elm_object_part_content_set(panes_h, "top", txt);

   // add lower panel
   txt = elm_label_add(win);
   elm_object_text_set(txt, "Info");
   evas_object_size_hint_weight_set(txt, EVAS_HINT_EXPAND, 0.1);
   evas_object_size_hint_align_set(txt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(txt);
   elm_object_part_content_set(panes_h, "bottom", txt);
   
   evas_object_show(panes);
   return panes;
}

static void
_tb_sel_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("Toolbar button pressed\n");
}

static Evas_Object *
edi_toolbar_setup(Evas_Object *win)
{
   Evas_Object *tb, *menu;
   Elm_Object_Item *tb_it, *menu_it;

   tb = elm_toolbar_add(win);
   elm_toolbar_homogeneous_set(tb, EINA_FALSE);
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_SCROLL);
   evas_object_size_hint_weight_set(tb, 0.0, 0.0);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, 0.0);

   tb_it = elm_toolbar_item_append(tb, "document-print", "Hello", _tb_sel_cb, NULL);
   elm_object_item_disabled_set(tb_it, EINA_TRUE);
   elm_toolbar_item_priority_set(tb_it, -100);

   tb_it = elm_toolbar_item_append(tb, "folder-new", "World", _tb_sel_cb, NULL);
   elm_toolbar_item_priority_set(tb_it, 100);

   tb_it = elm_toolbar_item_append(tb, "object-rotate-right", "H", _tb_sel_cb, NULL);
   elm_toolbar_item_priority_set(tb_it, -150);

   tb_it = elm_toolbar_item_append(tb, "mail-send", "Comes", _tb_sel_cb, NULL);
   elm_toolbar_item_priority_set(tb_it, -200);

   tb_it = elm_toolbar_item_append(tb, "clock", "Elementary", _tb_sel_cb, NULL);
   elm_toolbar_item_priority_set(tb_it, 0);

   tb_it = elm_toolbar_item_append(tb, "refresh", "Menu", NULL, NULL);
   elm_toolbar_item_menu_set(tb_it, EINA_TRUE);
   elm_toolbar_item_priority_set(tb_it, -9999);
   elm_toolbar_menu_parent_set(tb, win);
   menu = elm_toolbar_item_menu_get(tb_it);

   elm_menu_item_add(menu, NULL, "edit-cut", "Shrink", _tb_sel_cb, NULL);
   menu_it = elm_menu_item_add(menu, NULL, "edit-copy", "Mode", _tb_sel_cb, NULL);
   elm_menu_item_add(menu, menu_it, "edit-paste", "is set to", _tb_sel_cb, NULL);
   elm_menu_item_add(menu, NULL, "edit-delete", "Scroll", _tb_sel_cb, NULL);
   
   evas_object_show(tb);
   return tb;
}

static Evas_Object *
edi_win_setup(void)
{
   Evas_Object *win, *content, *tb;

   win = elm_win_util_standard_add("main", "Edi");
   if (!win) return NULL;

   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   evas_object_smart_callback_add(win, "delete,request", _edi_win_del, NULL);

   content = edi_content_setup(win);
   elm_win_resize_object_add(win, content);

   tb = edi_toolbar_setup(win);
   elm_box_pack_end(content, tb);

   evas_object_resize(win, 420, 300);
   evas_object_show(win);

   return win;
}

static const Ecore_Getopt optdesc = {
  "edi",
  "%prog [options]",
  PACKAGE_VERSION,
  COPYRIGHT,
  "BSD with advertisement clause",
  "The EFL IDE",
  0,
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

   elm_app_info_set(elm_main, "edi", "images/edi.png");

   if (!(win = edi_win_setup()))
     goto end;

   edi_library_call();

   elm_run();

 end:
   edi_shutdown();
   elm_shutdown();

   return 0;
}
ELM_MAIN()
