#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <libgen.h>

#include <Eina.h>
#include <Eio.h>

#include "mainview/edi_mainview_item.h"

#include "editor/edi_editor.h"

#include "edi_private.h"

Edi_Mainview_Item *
edi_mainview_item_add(Edi_Path_Options *path, const char *mime,
                      Elm_Object_Item *tab, Evas_Object *win)
{
   Edi_Mainview_Item *item;

   item = malloc(sizeof(Edi_Mainview_Item));
   item->path = eina_stringshare_add(path->path);
   item->editortype = path->type;
   item->mimetype = mime;
   item->win = win;
   item->tab = calloc(1, sizeof(Edi_Mainview_Item_Tab));
   item->tab->button = tab;
   item->view = NULL;

   return item;
}

