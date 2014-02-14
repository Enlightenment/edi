#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <libgen.h>

#include <Eio.h>

#include "edi_filepanel.h"

#include "edi_private.h"

static void stat_done(void *data, Eio_File *handler EINA_UNUSED, const Eina_Stat *st);

// TODO take this from the command line!
static char *PROJECT_ROOT = "/home/andy/Code/edi";

static Elm_Genlist_Item_Class itc, itc2;
static Evas_Object *list;

static void
dummy()
{}

static void
ls_done(void *data EINA_UNUSED, Eio_File *handler EINA_UNUSED, const char *path)
{
   eio_file_direct_stat(path, stat_done, dummy, eina_stringshare_add(path));
}

static Eina_Bool
ls_filter(void *data EINA_UNUSED, Eio_File *handler EINA_UNUSED, const char *file)
{
   struct stat st;
   if (stat(file, &st)) return EINA_FALSE;

   return !(eina_str_has_prefix(basename(file), "."));
}

static void
stat_done(void *data, Eio_File *handler EINA_UNUSED, const Eina_Stat *st)
{
   if (S_ISREG(st->mode))
     {
	elm_genlist_item_append(list, &itc, data,
				NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
     }
   else if (S_ISDIR(st->mode))
     {
	if (strcmp(data, PROJECT_ROOT))
	  {
	     elm_genlist_item_append(list, &itc2, data,
				     NULL, ELM_GENLIST_ITEM_TREE, NULL, NULL);
	// TODO remove this and recurse
	     return;
	  }

	eio_file_ls(data, ls_filter, ls_done, dummy, dummy, data);
     }
   else
     printf("WAT %s\n", data);

   eina_stringshare_del(data);
}

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
_item_del(void *data, Evas_Object *obj EINA_UNUSED)
{
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

void
edi_filepanel_add(Evas_Object *parent)
{
   list = elm_genlist_add(parent);
   evas_object_size_hint_min_set(list, 100, -1);
   evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(list);

   itc.item_style = "default";
   itc.func.text_get = _text_get;
   itc.func.content_get = _content_get;
//   itc.func.state_get = _state_get;
   itc.func.del = _item_del;

   itc2.item_style = "default";
   itc2.func.text_get = _text_get;
   itc2.func.content_get = _content_dir_get;
   itc2.func.del = _item_del;

   eio_file_direct_stat(PROJECT_ROOT, stat_done, dummy, eina_stringshare_add(PROJECT_ROOT));
   elm_box_pack_end(parent, list);
}
