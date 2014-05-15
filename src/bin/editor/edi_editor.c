#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <libgen.h>

#include <Eina.h>
#include <Eio.h>

#include "edi_editor.h"

#include "edi_mainview.h"

#include "edi_private.h"


static void
_smart_cb_key_down(void *data, Evas *e EINA_UNUSED,
                   Evas_Object *obj EINA_UNUSED, void *event)
{
   Eina_Bool ctrl, alt, shift;
   Evas_Event_Key_Down *ev = event;

   ctrl = evas_key_modifier_is_set(ev->modifiers, "Control");
   alt = evas_key_modifier_is_set(ev->modifiers, "Alt");
   shift = evas_key_modifier_is_set(ev->modifiers, "Shift");

   if ((!alt) && (ctrl) && (!shift))
     {
        if (!strcmp(ev->key, "Prior"))
          {
             edi_mainview_item_prev();
          }
        else if (!strcmp(ev->key, "Next"))
          {
             edi_mainview_item_next();
          }
        else if (!strcmp(ev->key, "s"))
          {
             edi_mainview_save();
          }
        else if (!strcmp(ev->key, "f"))
          {
             edi_mainview_search();
          }
     }
}

EAPI Evas_Object *edi_editor_add(Evas_Object *parent, const char *path)
{
   Evas_Object *txt;
   Evas_Modifier_Mask ctrl, shift, alt;
   Evas *e;

   txt = elm_entry_add(parent);
   elm_entry_editable_set(txt, EINA_TRUE);
   elm_entry_scrollable_set(txt, EINA_TRUE);
   elm_entry_line_wrap_set(txt, EINA_FALSE);
   elm_entry_text_style_user_push(txt, "DEFAULT='font=Monospace font_size=12'");
   elm_entry_file_set(txt, path, ELM_TEXT_FORMAT_PLAIN_UTF8);
   elm_entry_autosave_set(txt, EDI_CONTENT_AUTOSAVE);
   evas_object_size_hint_weight_set(txt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(txt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(txt);

   evas_object_event_callback_add(txt, EVAS_CALLBACK_KEY_DOWN,
                                  _smart_cb_key_down, txt);

   e = evas_object_evas_get(txt);
   ctrl = evas_key_modifier_mask_get(e, "Control");
   alt = evas_key_modifier_mask_get(e, "Alt");
   shift = evas_key_modifier_mask_get(e, "Shift");

   evas_object_key_grab(txt, "Prior", ctrl, shift | alt, 1);
   evas_object_key_grab(txt, "Next", ctrl, shift | alt, 1);
   evas_object_key_grab(txt, "s", ctrl, shift | alt, 1);
   evas_object_key_grab(txt, "f", ctrl, shift | alt, 1);

   return txt;
}
