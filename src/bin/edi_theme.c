#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Elementary.h>
#include <Evas.h>

#include "Edi.h"
#include "edi_theme.h"
#include "edi_config.h"
#include "edi_private.h"

static Eina_List *_edi_themes = NULL;
static Eina_Bool _edi_theme_internal_icons;

void
edi_theme_window_alpha_set(void)
{
   Evas_Object *win, *bg, *mainbox;
   static int r = 0, g = 0, b = 0, a = 0;
   double fade;
   Eina_Bool enabled = _edi_project_config->gui.translucent;

   win = edi_main_win_get();

   elm_win_alpha_set(win, enabled);

   mainbox = evas_object_data_get(win, "mainbox");
   if (enabled)
     efl_gfx_color_set(mainbox, 255, 255, 255, _edi_project_config->gui.alpha);
   else
     efl_gfx_color_set(mainbox, 255, 255, 255, 255);

   bg = evas_object_data_get(win, "background");

   fade = _edi_project_config->gui.alpha / 255.0;

   if (r == 0 && g == 0 && b == 0 && a == 0)
     efl_gfx_color_get(efl_part(win, "background"), &r, &g, &b, &a);

   if (enabled)
     {
        efl_gfx_color_set(efl_part(win, "background"), r, g, b, 0);
        efl_gfx_color_set(bg, r * fade, g * fade, b * fade, fade * _edi_project_config->gui.alpha);
     }
   else
     {
        efl_gfx_color_set(efl_part(win, "background"), r, g, b, 255);
        efl_gfx_color_set(bg, r, g, b, 128);
     }
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

const char *
edi_theme_icon_path_get(const char *name)
{
   char *path;
   const char *icon_path, *directory = PACKAGE_DATA_DIR "/icons";
   icon_path = name;

   path = edi_path_append(directory, eina_slstr_printf("%s.png", name));
   if (path)
     {
        if (ecore_file_exists(path))
          {
             icon_path = eina_slstr_printf("%s", path);
          }
        free(path);
     }

   return icon_path;
}

void
edi_theme_internal_icons_set(Eina_Bool enabled)
{
   _edi_theme_internal_icons = enabled;
}

Eina_Bool
edi_theme_internal_icons_get(void)
{
   return _edi_theme_internal_icons;
}
