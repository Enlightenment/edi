#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <libgen.h>

#include <Eina.h>

#include "edi_filepanel.h"

#include "edi_private.h"

// TODO take this from the command line!
static char *PROJECT_ROOT = "/home/andy/Code/E/edi";

static Elm_Genlist_Item_Class itc, itc2;
static Evas_Object *list;
static edi_filepanel_item_clicked_cb _open_cb;

static char *
_text_get(void *data, Evas_Object *obj EINA_UNUSED, const char *source EINA_UNUSED)
{
   return strdup(basename(data));
}

static Evas_Object *
_content_get(void *data, Evas_Object *obj, const char *source)
{
   if (!strcmp(source, "elm.swallow.icon"))
     {
        Evas_Object *ic;

        ic = elm_icon_add(obj);
        elm_icon_standard_set(ic, "file");
        evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
        evas_object_show(ic);
        return ic;
     }
   return NULL;
}

static void
_item_del(void *data, Evas_Object *obj)
{
   eina_stringshare_del(data);
   eina_stringshare_del(elm_object_item_data_get(obj));
}

static void
_item_sel(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _open_cb((const char*)data);
}

static Evas_Object *
_content_dir_get(void *data, Evas_Object *obj, const char *source)
{
   if (!strcmp(source, "elm.swallow.icon"))
     {
        Evas_Object *ic;

        ic = elm_icon_add(obj);
        elm_icon_standard_set(ic, "folder");
        evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
        evas_object_show(ic);
        return ic;
     }
   return NULL;
}

static Eina_Bool
ignore_file(const char *file)
{
   return eina_str_has_prefix(file, ".");
}

static int
_item_sort(const void *data1, const void *data2)
{
   const Elm_Object_Item *item1 = data1;
   const Elm_Object_Item *item2 = data2;

   const char *name1 = elm_object_item_data_get(item1);
   const char *name2 = elm_object_item_data_get(item2);
   return strcasecmp(name1, name2);
}

static void
load_tree(char *path, Elm_Object_Item *parent)
{
   Eina_Iterator *iter;
   Eina_File_Direct_Info *info;
   Elm_Object_Item *newParent;
   char *name;

   iter = eina_file_stat_ls(path);
   if (iter)
     {
        EINA_ITERATOR_FOREACH(iter, info)
          {
              name = info->path + info->name_start;
              if (ignore_file(name)) continue;

              if (info->type == EINA_FILE_DIR)
                {
                   newParent = elm_genlist_item_sorted_insert(list, &itc2, eina_stringshare_add(name),
                                                              parent, ELM_GENLIST_ITEM_NONE, _item_sort, _item_sel, eina_stringshare_add(info->path));
                   load_tree(info->path, newParent);
                }
              else if (info->type == EINA_FILE_REG)
                {
                   elm_genlist_item_sorted_insert(list, &itc, eina_stringshare_add(name),
                                                  parent, ELM_GENLIST_ITEM_NONE, _item_sort, _item_sel, eina_stringshare_add(info->path));
                }
          }
     }
}

void
edi_filepanel_add(Evas_Object *parent,
                  const char *path,
                  edi_filepanel_item_clicked_cb cb)
{
   list = elm_genlist_add(parent);
   evas_object_size_hint_min_set(list, 100, -1);
   evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(list);

   itc.item_style = "default";
   itc.func.text_get = _text_get;
   itc.func.content_get = _content_get;
   itc.func.del = _item_del;

   itc2.item_style = "default";
   itc2.func.text_get = _text_get;
   itc2.func.content_get = _content_dir_get;
//   itc2.func.state_get = _state_get;
   itc2.func.del = _item_del;

   _open_cb = cb;
   load_tree(path, NULL);

   elm_box_pack_end(parent, list);
}
