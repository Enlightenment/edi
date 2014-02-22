#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <libgen.h>

#include <Eina.h>
#include <Eio.h>

#include "edi_mainview.h"

#include "edi_private.h"

static Evas_Object *nf, *tb;

static void
dummy()
{}

static void
_promote(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_naviframe_item_promote(data);
}

static Elm_Object_Item *
_get_item_for_path(const char *path)
{
   Eina_List *list, *item;
   Elm_Object_Item *it;

   list = elm_naviframe_items_get(nf);
   EINA_LIST_FOREACH(list, item, it)
     {
        const char *loaded;
        loaded = elm_object_item_data_get(it);

        if (loaded && !strcmp(loaded, path))
          return it;
     }
   return NULL;
}

static void
_edi_mainview_open_file_text(const char *path)
{
   Evas_Object *txt;
   Elm_Object_Item *it, *tab;
        
   txt = elm_entry_add(nf);
   elm_entry_scrollable_set(txt, EINA_TRUE);
   elm_entry_text_style_user_push(txt, "DEFAULT='font=Monospace font_size=12'");
   elm_entry_file_set(txt, path, ELM_TEXT_FORMAT_PLAIN_UTF8);
   elm_entry_autosave_set(txt, EDI_CONTENT_AUTOSAVE);
   evas_object_size_hint_weight_set(txt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(txt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(txt);

   it = elm_naviframe_item_simple_push(nf, txt);
   elm_object_item_data_set(it, path);
   elm_naviframe_item_style_set(it, "overlap");
   tab = elm_toolbar_item_append(tb, NULL, basename(path), _promote, it);
   elm_toolbar_item_selected_set(tab, EINA_TRUE);
}

static void
_edi_mainview_open_file_image(const char *path)
{
   Evas_Object *img, *scroll;
   Elm_Object_Item *it, *tab;

   scroll = elm_scroller_add(nf);
   evas_object_size_hint_weight_set(scroll, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(scroll, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(scroll);
   img = elm_image_add(scroll);
   elm_image_file_set(img, path, NULL);
   elm_image_no_scale_set(img, EINA_TRUE);
   elm_object_content_set(scroll, img);
   evas_object_show(img);

   it = elm_naviframe_item_simple_push(nf, scroll);
   elm_object_item_data_set(it, path);
   elm_naviframe_item_style_set(it, "overlap");
   tab = elm_toolbar_item_append(tb, NULL, basename(path), _promote, it);
   elm_toolbar_item_selected_set(tab, EINA_TRUE);
}

static void
_edi_mainview_open_stat_done(void *data, Eio_File *handler EINA_UNUSED, const Eina_Stat *stat)
{
   const char *path, *mime;

   path = data;
   if (S_ISREG(stat->mode))
     {
        mime = efreet_mime_type_get(path);
        if (!strcasecmp(mime, "text/plain") || !strcasecmp(mime, "application/x-shellscript"))
          _edi_mainview_open_file_text(path);
        else if (!strcasecmp(mime, "text/x-chdr") || !strcasecmp(mime, "text/x-csrc"))
          _edi_mainview_open_file_text(path); // TODO make a code view
        else if (!strncasecmp(mime, "image/", 6))
          _edi_mainview_open_file_image(path);
        else
          printf("Unknown mime %s\n", mime);
     }

   eina_stringshare_del(path);
}

EAPI void
edi_mainview_open_path(const char *path)
{
   Elm_Object_Item *it;

   it = _get_item_for_path(path);
   if (it)
     {
        elm_naviframe_item_promote(it);
        return;
     }

   eio_file_direct_stat(path, _edi_mainview_open_stat_done, dummy,
                        eina_stringshare_add(path));
}

EAPI void
edi_mainview_save()
{
   Evas_Object *txt;
   Elm_Object_Item *it;

   it = elm_naviframe_top_item_get(nf);
   txt = elm_object_item_content_get(it);
   if (txt)
     elm_entry_file_save(txt);
}

EAPI void
edi_mainview_close()
{
   if (!elm_object_item_data_get(elm_naviframe_top_item_get(nf)))
     return;

   elm_naviframe_item_pop(nf);
   elm_object_item_del(elm_toolbar_selected_item_get(tb));
}

EAPI void
edi_mainview_cut()
{
   Evas_Object *txt;
   Elm_Object_Item *it;

   it = elm_naviframe_top_item_get(nf);
   txt = elm_object_item_content_get(it);
   if (txt)
     elm_entry_selection_cut(txt);
}

EAPI void
edi_mainview_copy()
{
   Evas_Object *txt;
   Elm_Object_Item *it;

   it = elm_naviframe_top_item_get(nf);
   txt = elm_object_item_content_get(it);
   if (txt)
     elm_entry_selection_copy(txt);
}

EAPI void
edi_mainview_paste()
{
   Evas_Object *txt;
   Elm_Object_Item *it;

   it = elm_naviframe_top_item_get(nf);
   txt = elm_object_item_content_get(it);
   if (txt)
     elm_entry_selection_paste(txt);
}

EAPI void
edi_mainview_add(Evas_Object *parent)
{
   Evas_Object *box, *txt;
   Elm_Object_Item *it;

   box = elm_box_add(parent);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);
   elm_box_pack_end(parent, box);   
   
   tb = elm_toolbar_add(parent);
   evas_object_size_hint_weight_set(tb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_toolbar_homogeneous_set(tb, EINA_FALSE);
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_SCROLL);
   elm_toolbar_select_mode_set(tb, ELM_OBJECT_SELECT_MODE_ALWAYS);
   elm_box_pack_end(box, tb);
   evas_object_show(tb);

   nf = elm_naviframe_add(parent);
   evas_object_size_hint_weight_set(nf, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(nf, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, nf);
   evas_object_show(nf);

   txt = elm_label_add(parent);
   elm_object_text_set(txt, "Welcome - tap a file to edit");
   evas_object_size_hint_weight_set(txt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(txt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(txt);

   it = elm_naviframe_item_simple_push(nf, txt);
   elm_naviframe_item_style_set(it, "overlap");
   elm_toolbar_item_append(tb, NULL, "Welcome", _promote, it);
}
