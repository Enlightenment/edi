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
   const char *text;
   char *message;

   engine = edi_scm_engine_get();
   // engine has been checked before now
   if (!engine)
     return;

   text = elm_entry_entry_get((Evas_Object *) data);
   if (!text || !text[0])
     {
        _edi_scm_screens_message_open("Please enter a valid commit message.");
        return;
     }

   message = elm_entry_markup_to_utf8(text);

   edi_consolepanel_clear();
   edi_consolepanel_show();
   edi_scm_commit(message);

   evas_object_del(_popup);

   free(message);
}

void
edi_scm_screens_commit(Evas_Object *parent)
{
   Evas_Object *popup, *box, *hbox, *sep, *label, *avatar, *input, *button;
   Evas_Object *list, *icon, *entry;
   Eina_Strbuf *text, *user;
   Eina_List *l;
   Edi_Scm_Engine *engine;
   Edi_Scm_Status *status;
   char *diff, *markup;
   Eina_Bool staged_changes;

   engine= edi_scm_engine_get();
   if (!engine)
     {
        _edi_scm_screens_message_open("SCM engine is not available.");
        return;
     }

   _parent_obj = parent;
   _popup = popup = elm_popup_add(parent);
   elm_popup_scrollable_set(popup, EINA_TRUE);
   evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(popup, EVAS_HINT_FILL, EVAS_HINT_FILL);
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

   sep = elm_separator_add(box);
   elm_separator_horizontal_set(sep, EINA_TRUE);
   evas_object_show(sep);
   elm_box_pack_end(box, sep);

   label = elm_label_add(box);
   evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(label, "<b>Summary<b>");
   elm_box_pack_end(box, label);
   evas_object_show(label);

   list = elm_list_add(box);
   elm_list_mode_set(list, ELM_LIST_EXPAND);
   elm_list_select_mode_set(list, ELM_OBJECT_SELECT_MODE_NONE);
   evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, list);

   staged_changes = EINA_FALSE;

   if (edi_scm_status_get())
     {
        text = eina_strbuf_new();
        EINA_LIST_FOREACH(engine->statuses, l, status)
          {
             icon = elm_icon_add(box);
             if (status->staged)
               staged_changes = EINA_TRUE;

             eina_strbuf_append_printf(text, "%s ", status->path);

             switch (status->change)
               {
                case EDI_SCM_STATUS_ADDED:
                  elm_icon_standard_set(icon, "document-new");
                  eina_strbuf_append(text, "(add) ");
                  break;
                case EDI_SCM_STATUS_MODIFIED:
                  elm_icon_standard_set(icon, "document-save-as");
                  eina_strbuf_append(text, "(mod) ");
                  break;
                case EDI_SCM_STATUS_DELETED:
                  elm_icon_standard_set(icon, "edit-delete");
                  eina_strbuf_append(text, "(del) ");
                  break;
                case EDI_SCM_STATUS_RENAMED:
                  elm_icon_standard_set(icon, "document-save-as");
                  eina_strbuf_append(text, "(ren) ");
                  break;
                case EDI_SCM_STATUS_UNTRACKED:
                  elm_icon_standard_set(icon, "dialog-question");
                  eina_strbuf_append(text, "(untracked)");
                  break;
                default:
                  elm_icon_standard_set(icon, "text-x-generic");
               }

             if (!status->staged && status->change != EDI_SCM_STATUS_UNTRACKED)
               eina_strbuf_append(text, "- unstaged");

             elm_list_item_append(list, eina_strbuf_string_get(text), icon, NULL, NULL, NULL);

             eina_strbuf_reset(text);

             eina_stringshare_del(status->path);
             free(status);
          }
        eina_strbuf_free(text);
        eina_list_free(engine->statuses);
        engine->statuses = NULL;
     }
   else
     {
        icon = elm_icon_add(box);
        elm_icon_standard_set(icon, "dialog-information");
        elm_list_item_append(list, "Nothing to commit.", icon, NULL, NULL, NULL);
     }

   elm_scroller_bounce_set(list, EINA_TRUE, EINA_TRUE);
   elm_scroller_policy_set(list, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_ON);
   elm_list_go(list);
   evas_object_show(list);

   input = elm_entry_add(box);
   elm_object_text_set(input, "Enter commit summary<br><br>And change details<br>");
   evas_object_size_hint_weight_set(input, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(input, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_entry_editable_set(input, staged_changes);
   elm_entry_scrollable_set(input, EINA_TRUE);
   elm_entry_single_line_set(input, EINA_TRUE);
   elm_object_style_set(input, "entry");
   evas_object_show(input);
   elm_box_pack_end(box, input);

   entry = elm_entry_add(box);
   evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_entry_line_wrap_set(entry, ELM_WRAP_NONE);
   elm_entry_editable_set(entry, EINA_FALSE);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   elm_entry_single_line_set(entry, EINA_TRUE);
   evas_object_show(entry);
   elm_box_pack_end(box, entry);

   diff = edi_scm_diff();
   text = eina_strbuf_new();
   markup = elm_entry_utf8_to_markup(diff);
   if (strlen(markup))
     eina_strbuf_append_printf(text, "<font=Fixed>%s</font>", markup);
   else
     eina_strbuf_append(text, "No changes to display.");

   elm_object_text_set(entry, eina_strbuf_string_get(text));

   eina_strbuf_free(text);
   free(markup);
   free(diff);

   button = elm_button_add(popup);
   elm_object_text_set(button, "Cancel");
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked",
                                  _edi_scm_screens_popup_cancel_cb, popup);

   button = elm_button_add(popup);
   evas_object_data_set(button, "input", input);
   elm_object_text_set(button, "Commit");
   elm_object_disabled_set(button, !staged_changes);
   elm_object_part_content_set(popup, "button2", button);
   evas_object_smart_callback_add(button, "clicked",
                                  _edi_scm_screens_commit_cb, input);
   evas_object_show(popup);
   elm_object_focus_set(input, EINA_TRUE);
}

void
edi_scm_screens_binary_missing(Evas_Object *parent, const char *binary)
{
   Evas_Object *popup, *label, *button;
   Eina_Strbuf *text = eina_strbuf_new();

   eina_strbuf_append_printf(text, "No %s binary found, please install %s.", binary, binary);

   popup = elm_popup_add(parent);
   elm_object_part_text_set(popup, "title,text", "SCM: Unable to launch");
   label = elm_label_add(popup);
   elm_object_text_set(label, eina_strbuf_string_get(text));
   evas_object_show(label);
   elm_object_content_set(popup, label);

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

   elm_image_file_set(image, file, NULL);
}

void edi_scm_screens_avatar_load(Evas_Object *image, const char *email)
{
   const char *tmp, *cache, *cachedir, *cacheparentdir;

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
     goto clear;

   if (!ecore_file_exists(cachedir) && !ecore_file_mkdir(cachedir))
     goto clear;

   ecore_file_download(edi_scm_avatar_url_get(email), cache,
                       _edi_scm_screens_avatar_download_complete, NULL,
                       image, NULL);

clear:
   free((char *)cachedir);
}

