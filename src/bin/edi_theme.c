#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Elementary.h>
#include <Efl_Ui.h>

#include "Edi.h"
#include "edi_theme.h"
#include "edi_config.h"
#include "edi_private.h"

static Eina_List *_edi_themes = NULL;
static Evas_Object *_main_box = NULL;

// we are hooking into Efl for now...
Efl_Ui_Theme_Apply efl_ui_widget_theme_apply(Eo *obj);

void
edi_theme_main_box_set(Evas_Object *box)
{
   _main_box = box;
}

Evas_Object *
edi_theme_main_box_get(void)
{
   return _main_box;
}

void
edi_theme_window_alpha_set(void)
{
   Evas_Object *win;
   Evas_Object *background;
   static int r, g, b, a;
   Eina_Bool enabled = _edi_project_config->gui.translucent;

   win = edi_main_win_get();

   elm_win_alpha_set(win, enabled);

   background = edi_theme_main_box_get();
   if (!background)
     return;

   if (!r && !g && !b && !a)
     evas_object_color_get(background, &r, &g, &b, &a);

   if (enabled)
     evas_object_color_set(background, r, g, b, _edi_project_config->gui.alpha);
   else
     evas_object_color_set(background, r, g, b, 255);
}

void edi_theme_elm_code_alpha_set(Evas_Object *obj)
{
  if (_edi_project_config->gui.translucent)
     elm_code_widget_alpha_set(obj, _edi_project_config->gui.alpha);
   else
     elm_code_widget_alpha_set(obj, 255);
}

void
edi_theme_elm_code_set(Evas_Object *obj, const char *name)
{
   Eina_List *l;
   Edi_Theme *theme;

   if (!obj || !name)
     return;

   edi_theme_themes_get();

   EINA_LIST_FOREACH(_edi_themes, l, theme)
     {
        if (strcmp(theme->name, name))
          continue;

        elm_layout_file_set(obj, theme->path, "elm/code/layout/default");
        elm_code_widget_theme_refresh(obj);
     }
}

Edi_Theme *
edi_theme_theme_by_name(const char *name)
{
    Eina_List *l;
    Edi_Theme *theme;

    if (!name) return NULL;

    edi_theme_themes_get();

    EINA_LIST_FOREACH(_edi_themes, l, theme)
      {
         if (!strcmp(theme->name, name))
           return theme;
      }

    return NULL;
}

static int
_theme_sort_cb(const void *t1, const void *t2)
{
   const Edi_Theme *theme1 = t1;
   const Edi_Theme *theme2 = t2;

   if (!theme1) return 1;
   if (!theme2) return -1;

   return strcmp(theme1->title, theme2->title);
}

Eina_List *
edi_theme_themes_get(void)
{
   Eina_List *files;
   char *directory, *file, *name;
   Edi_Theme *theme;

   if (_edi_themes) return _edi_themes;

   directory = PACKAGE_DATA_DIR "/themes";

   theme = malloc(sizeof(Edi_Theme));
   theme->name = strdup("default");
   theme->path = edi_path_append(elm_theme_system_dir_get(), "default.edj");
   theme->title = strdup("Default EFL");

   _edi_themes = eina_list_append(_edi_themes, theme);

   files = ecore_file_ls(directory);
   EINA_LIST_FREE(files, file)
     {
        if (eina_str_has_extension(file, ".edj") && strcmp(file, "default.edj"))
          {
             theme = malloc(sizeof(Edi_Theme));
             name = strdup(file);
             name[strlen(name) - 4] = '\0';
             theme->name = name;
             theme->path = edi_path_append(directory, file);
             theme->title = edje_file_data_get(theme->path, "title");
             _edi_themes = eina_list_append(_edi_themes, theme);
          }
        free(file);
     }

   if (files)
     eina_list_free(files);

   _edi_themes = eina_list_sort(_edi_themes, eina_list_count(_edi_themes), _theme_sort_cb);

   return _edi_themes;
}


