#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <libgen.h>

#include <Eina.h>

#include "edi_mainview.h"

#include "edi_private.h"

static Evas_Object *nf, *tb;

static void
_promote(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_naviframe_item_promote(data);
}

EAPI void
edi_mainview_open(const char *path)
{
   Evas_Object *txt;
   Elm_Object_Item *it, *tab;
        
   txt = elm_entry_add(nf);
   elm_entry_scrollable_set(txt, EINA_TRUE);
   elm_entry_file_set(txt, path, ELM_TEXT_FORMAT_PLAIN_UTF8);
   elm_entry_autosave_set(txt, EINA_TRUE);
   evas_object_size_hint_weight_set(txt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(txt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(txt);

   it = elm_naviframe_item_simple_push(nf, txt);
   tab = elm_toolbar_item_append(tb, NULL, basename(path), _promote, it);
   elm_toolbar_item_selected_set(tab, EINA_TRUE);
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
   elm_toolbar_item_append(tb, NULL, "Welcome", _promote, it);
}
