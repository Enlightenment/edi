#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "Edi.h"
#include "mainview/edi_mainview.h"
#include "edi_consolepanel.h"
#include "edi_scm_screens.h"
#include "edi_private.h"

static Evas_Object *_parent_obj, *_popup, *_edi_scm_screens_message_popup;

static void
_edi_scm_screens_message_close_cb(void *data EINA_UNUSED,
                     Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   Evas_Object *popup = data;
   evas_object_del(popup);
}

static void
_edi_scm_screens_message_open(const char *message)
{
   Evas_Object *popup, *button;

   _edi_scm_screens_message_popup = popup = elm_popup_add(_parent_obj);
   elm_object_part_text_set(popup, "title,text",
                           message);

   button = elm_button_add(popup);
   elm_object_text_set(button, "Ok");
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked",
                                 _edi_scm_screens_message_close_cb, popup);

   evas_object_show(popup);
}

static void
_edi_scm_screens_popup_cancel_cb(void *data, Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   evas_object_del((Evas_Object *)data);
}

static void
_edi_scm_screens_commit_cb(void *data,
                           Evas_Object *obj EINA_UNUSED,
                           void *event_info EINA_UNUSED)
{
   Edi_Scm_Engine *engine;
   const char *message;

   engine = edi_scm_engine_get();
   // engine has been checked before now
   if (!engine)
     return;

   message = elm_entry_entry_get((Evas_Object *) data);
   if (!message || strlen(message) == 0)
     {
        _edi_scm_screens_message_open("Please enter a valid commit message.");
        return;
     }

   edi_consolepanel_clear();
   edi_consolepanel_show();
   edi_scm_commit(message);

   evas_object_del(_popup);
}

void
edi_scm_screens_commit(Evas_Object *parent)
{
   Evas_Object *popup, *box, *hbox, *label, *avatar, *input, *button;
   Eina_Strbuf *user;
   Edi_Scm_Engine *engine;

   engine= edi_scm_engine_get();
   if (!engine)
     {
        _edi_scm_screens_message_open("SCM engine is not available.");
        return;
     }

   _parent_obj = parent;
   _popup = popup = elm_popup_add(parent);

   elm_object_part_text_set(popup, "title,text",
                                     "Commit Changes");
   box = elm_box_add(popup);
   elm_box_horizontal_set(box, EINA_FALSE);
   elm_object_content_set(popup, box);

   hbox = elm_box_add(popup);
   elm_box_horizontal_set(hbox, EINA_TRUE);
   evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(hbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(hbox);
   elm_box_pack_end(box, hbox);

   label = elm_label_add(hbox);
   evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(label);
   elm_box_pack_end(hbox, label);

   user = eina_strbuf_new();
   eina_strbuf_append_printf(user, "Author:<br><b>%s</b><br>&lt;%s&gt;",
                             engine->remote_name_get(), engine->remote_email_get());
   elm_object_text_set(label, eina_strbuf_string_get(user));
   eina_strbuf_free(user);

   avatar = elm_image_add(hbox);
   edi_scm_screens_avatar_load(avatar, engine->remote_email_get());
   evas_object_size_hint_min_set(avatar, 48 * elm_config_scale_get(), 48 * elm_config_scale_get());
   evas_object_size_hint_weight_set(avatar, 0.1, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(avatar, 1.0, EVAS_HINT_FILL);
   evas_object_show(avatar);
   elm_box_pack_end(hbox, avatar);

   input = elm_entry_add(box);
   elm_entry_editable_set(input, EINA_TRUE);
   elm_object_text_set(input, "Enter commit summary<br><br>And change details");
   evas_object_size_hint_weight_set(input, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(input, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(input);
   elm_box_pack_end(box, input);

   button = elm_button_add(popup);
   elm_object_text_set(button, "cancel");
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked",
                                  _edi_scm_screens_popup_cancel_cb, popup);

   button = elm_button_add(popup);
   evas_object_data_set(button, "input", input);
   elm_object_text_set(button, "commit");
   elm_object_part_content_set(popup, "button2", button);
   evas_object_smart_callback_add(button, "clicked",
                                  _edi_scm_screens_commit_cb, input);

   evas_object_show(popup);
   elm_object_focus_set(input, EINA_TRUE);
}

void
edi_scm_screens_binary_missing(Evas_Object *parent, const char *binary)
{
   Evas_Object *popup, *button;
   Eina_Strbuf *text = eina_strbuf_new();

   eina_strbuf_append_printf(text, "No %s binary found, please install %s.", binary, binary);

   popup = elm_popup_add(parent);
   elm_object_part_text_set(popup, "title,text", "SCM: Unable to launch");
   elm_object_text_set(popup, eina_strbuf_string_get(text));

   eina_strbuf_free(text);

   button = elm_button_add(popup);
   elm_object_text_set(button, "OK");
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked", _edi_scm_screens_popup_cancel_cb, popup);

   evas_object_show(popup);
}

const char *
_edi_scm_avatar_cache_path_get(const char *email)
{
   return eina_stringshare_printf("%s/%s/avatars/%s.jpeg", efreet_cache_home_get(),
                                  PACKAGE_NAME, email);
}

void _edi_scm_screens_avatar_download_complete(void *data, const char *file,
                                               int status)
{
   Evas_Object *image = data;

   if (status != 200)
     {
        ecore_file_remove(file);
        return;
     }

   // TODO figure why this crashes
   //elm_image_file_set(image, file, NULL);
}

void edi_scm_screens_avatar_load(Evas_Object *image, const char *email)
{
   const char *tmp, *cache, *cachedir, *cacheparentdir;
   Ecore_File_Download_Job *job;

   cache = _edi_scm_avatar_cache_path_get(email);
   if (ecore_file_exists(cache))
     {
        elm_image_file_set(image, cache, NULL);
        return;
     }

   tmp = dirname((char *) cache);
   cachedir = strdup(tmp);
   cacheparentdir = dirname((char *) tmp);
   if (!ecore_file_exists(cacheparentdir) && !ecore_file_mkdir(cacheparentdir))
     {
        free((char *)tmp);
        free((char *)cachedir);
        return;
     }

   free((char *)tmp);
   if (!ecore_file_exists(cachedir) && !ecore_file_mkdir(cachedir))
     {
        free((char *)cachedir);
        return;
     }

   free((char *)cachedir);

   ecore_file_download(edi_scm_avatar_url_get(email), cache,
                       _edi_scm_screens_avatar_download_complete, NULL,
                       image, &job);
}

