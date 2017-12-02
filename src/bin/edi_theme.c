#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Elementary.h>

#include "Edi.h"
#include "edi_theme.h"
#include "edi_config.h"
#include "edi_private.h"

static Eina_List *_edi_themes = NULL;

// we are hooking into Efl for now...
Efl_Ui_Theme_Apply efl_ui_widget_theme_apply(Eo *obj);

void
edi_theme_elm_code_set(Evas_Object *obj, const char *name)
{
   Eina_List *l;
   Edi_Theme *theme;

   if (!name)
     return;

   edi_theme_themes_get();

   EINA_LIST_FOREACH(_edi_themes, l, theme)
     {
        if (strcmp(theme->name, name))
          continue;

        elm_layout_file_set(obj, theme->path, "elm/code/layout/default");
        efl_ui_widget_theme_apply(obj);
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
   theme->title = strdup("Default");

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

   return _edi_themes;
}


