#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "Edi.h"
#include "mainview/edi_mainview.h"
#include "edi_config.h"
#include "edi_filepanel.h"
#include "edi_file.h"
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
   elm_object_text_set(button, _("OK"));
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
        _edi_scm_screens_message_open(_("Please enter a valid commit message."));
        return;
     }

   message = elm_entry_markup_to_utf8(text);

   edi_consolepanel_clear();
   edi_consolepanel_show();
   edi_scm_commit(message);
   edi_filepanel_status_refresh();
   evas_object_del(_popup);

   free(message);
}

static void
_entry_lines_append(Elm_Code *code, char *diff_text)
{
   char *pos = diff_text;
   char *start, *end = NULL;

   if (!*pos) return;

   start = pos;
   while (*pos++ != '\0')
    {
       if (*pos == '\n')
         end = pos;

       if (start && end)
         {
            elm_code_file_line_append(code->file, start, end - start, NULL);
            start = end + 1;
            end = NULL;
         }
    }
    end = pos;
    if (end > start)
      elm_code_file_line_append(code->file, start, end - start, NULL);
}

static void
_set_icons_status(Evas_Object *icon, Evas_Object *icon_status, Edi_Scm_Status *status)
{
   switch (status->change)
     {
        case EDI_SCM_STATUS_ADDED:
        case EDI_SCM_STATUS_ADDED_STAGED:
           elm_icon_standard_set(icon, "document-new");
           break;
        case EDI_SCM_STATUS_MODIFIED:
        case EDI_SCM_STATUS_MODIFIED_STAGED:
           elm_icon_standard_set(icon, "document-save-as");
           break;
        case EDI_SCM_STATUS_DELETED:
        case EDI_SCM_STATUS_DELETED_STAGED:
           elm_icon_standard_set(icon, "edit-delete");
           break;
        case EDI_SCM_STATUS_RENAMED:
        case EDI_SCM_STATUS_RENAMED_STAGED:
          elm_icon_standard_set(icon, "document-save-as");
           break;
        case EDI_SCM_STATUS_UNTRACKED:
           elm_icon_standard_set(icon, "dialog-question");
           break;
        default:
           elm_icon_standard_set(icon, "text-x-generic");
     }

   if (status->change == EDI_SCM_STATUS_UNTRACKED)
     elm_icon_standard_set(icon_status, "dialog-error");
   else if (status->staged)
     elm_icon_standard_set(icon_status, "dialog-information");
   else
     elm_icon_standard_set(icon_status, "dialog-error");
}

void
edi_scm_screens_commit(Evas_Object *parent)
{
   Evas_Object *popup, *box, *frame, *hbox, *cbox, *label, *avatar, *input, *button;
   Evas_Object *list, *icon, *icon_status;
   Elm_Code_Widget *entry;
   Elm_Code *code;
   Eina_Strbuf *text;
   Edi_Scm_Engine *engine;
   Edi_Scm_Status *status;
   Elm_Object_Item *item;
   char *diff_text;
   Eina_Bool staged_changes;

   engine= edi_scm_engine_get();
   if (!engine)
     {
        _edi_scm_screens_message_open(_("SCM engine is not available."));
        return;
     }

   _parent_obj = parent;
   _popup = popup = elm_popup_add(parent);
   evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(popup, EVAS_HINT_FILL, EVAS_HINT_FILL);

   box = elm_box_add(popup);
   elm_box_horizontal_set(box, EINA_FALSE);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_content_set(popup, box);

   hbox = elm_box_add(popup);
   elm_box_horizontal_set(hbox, EINA_TRUE);
   evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(hbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(hbox);

   frame = elm_frame_add(hbox);
   elm_object_text_set(frame, _("Summary"));
   evas_object_size_hint_weight_set(frame, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(frame, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_content_set(frame, hbox);
   evas_object_show(frame);
   elm_box_pack_end(box, frame);

   label = elm_label_add(hbox);
   evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(label);
   elm_box_pack_end(hbox, label);

   text = eina_strbuf_new();
   eina_strbuf_append_printf(text, "%s:<br><b>%s</b><br>&lt;%s&gt;", _("Author"),
                             engine->remote_name_get(), engine->remote_email_get());
   elm_object_text_set(label, eina_strbuf_string_get(text));
   eina_strbuf_reset(text);

   avatar = elm_image_add(hbox);
   edi_scm_screens_avatar_load(avatar, engine->remote_email_get());
   evas_object_size_hint_min_set(avatar, 48 * elm_config_scale_get(), 48 * elm_config_scale_get());
   evas_object_size_hint_weight_set(avatar, 0.1, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(avatar, 1.0, EVAS_HINT_FILL);
   evas_object_show(avatar);
   elm_box_pack_end(hbox, avatar);

   cbox = elm_box_add(box);
   evas_object_size_hint_weight_set(cbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(cbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_min_set(cbox, 250 * elm_config_scale_get(), 100 * elm_config_scale_get());
   evas_object_show(cbox);

   list = elm_list_add(box);
   elm_list_select_mode_set(list, ELM_OBJECT_SELECT_MODE_NONE);
   evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(cbox, list);
   elm_box_pack_end(box, cbox);

   staged_changes = EINA_FALSE;

   if (edi_scm_status_get())
     {
        EINA_LIST_FREE(engine->statuses, status)
          {
             icon = elm_icon_add(box);
             icon_status = elm_icon_add(box);

             _set_icons_status(icon, icon_status, status);

             item = elm_list_item_append(list, status->path, icon, icon_status, NULL, NULL);

             if (status->change == EDI_SCM_STATUS_UNTRACKED)
               {
                  elm_object_item_tooltip_text_set(item, _("Untracked changes"));
               }
             else if (status->staged)
               {
                  staged_changes = EINA_TRUE;
                  elm_object_item_tooltip_text_set(item, _("Staged changes"));
               }
             else
               {
                  elm_object_item_tooltip_text_set(item, _("Unstaged changes"));
               }

             eina_stringshare_del(status->path);
             free(status);
          }
        eina_list_free(engine->statuses);
        engine->statuses = NULL;
     }
   else
     {
        icon = elm_icon_add(box);
        elm_icon_standard_set(icon, "dialog-information");
        elm_list_item_append(list, _("Nothing to commit."), icon, NULL, NULL, NULL);
     }

   elm_scroller_bounce_set(list, EINA_TRUE, EINA_TRUE);
   elm_scroller_policy_set(list, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_ON);
   elm_list_go(list);
   evas_object_show(list);

   frame = elm_frame_add(popup);
   evas_object_size_hint_weight_set(frame, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(frame, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(frame, _("Commit message"));
   evas_object_show(frame);

   input = elm_entry_add(box);
   elm_object_text_set(input, _("Enter commit summary<br><br>And change details<br>"));
   evas_object_size_hint_weight_set(input, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(input, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_entry_editable_set(input, staged_changes);
   elm_entry_scrollable_set(input, EINA_TRUE);
   elm_entry_single_line_set(input, EINA_TRUE);
   elm_object_style_set(input, "entry");
   evas_object_show(input);
   elm_object_content_set(frame, input);
   elm_box_pack_end(box, frame);

   diff_text = edi_scm_diff();
   if (diff_text[0] && diff_text[1])
     {
        frame = elm_frame_add(popup);
        evas_object_size_hint_weight_set(frame, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(frame, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_object_text_set(frame, _("Unstaged changes"));
        evas_object_show(frame);

        cbox = elm_box_add(popup);
        evas_object_size_hint_weight_set(cbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(cbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_size_hint_min_set(cbox, 500 * elm_config_scale_get(), 250 * elm_config_scale_get());
        evas_object_show(cbox);
        elm_object_content_set(frame, cbox);
        elm_box_pack_end(box, frame);

        code = elm_code_create();
        entry = elm_code_widget_add(box, code);
        elm_code_parser_standard_add(code, ELM_CODE_PARSER_STANDARD_DIFF);
        elm_obj_code_widget_font_set(entry, _edi_project_config->font.name, _edi_project_config->font.size);
        elm_obj_code_widget_gravity_set(entry, 0.0, 1.0);
        elm_obj_code_widget_editable_set(entry, EINA_FALSE);
        elm_obj_code_widget_line_numbers_set(entry, EINA_FALSE);
        evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_show(entry);
        elm_box_pack_end(cbox, entry);

        _entry_lines_append(code, diff_text);
     }

   free(diff_text);

   button = elm_button_add(popup);
   elm_object_text_set(button, _("Cancel"));
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked",
                                  _edi_scm_screens_popup_cancel_cb, popup);

   button = elm_button_add(popup);
   evas_object_data_set(button, "input", input);
   elm_object_text_set(button, _("Commit"));
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

   eina_strbuf_append_printf(text, _("No %s binary found, please install %s."), binary, binary);

   popup = elm_popup_add(parent);
   elm_object_part_text_set(popup, "title,text", _("Unable to launch SCM binary"));
   label = elm_label_add(popup);
   elm_object_text_set(label, eina_strbuf_string_get(text));
   evas_object_show(label);
   elm_object_content_set(popup, label);

   eina_strbuf_free(text);

   button = elm_button_add(popup);
   elm_object_text_set(button, _("OK"));
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

