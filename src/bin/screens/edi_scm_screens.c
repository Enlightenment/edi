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
static Eina_Hash *_hash_statuses = NULL;

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

   text = elm_object_text_get((Evas_Object *) data);
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
_entry_lines_append(Elm_Code *code, char *text)
{
   char *pos = text;
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

static Edi_Scm_Status_Code *
_file_status_item_find(const char *path)
{
   return eina_hash_find(_hash_statuses, path);
}

static void
_file_status_item_add(const char *path, Edi_Scm_Status_Code status)
{
   Edi_Scm_Status_Code *code;

   if (_file_status_item_find(path)) return;

   code = malloc(sizeof(Edi_Scm_Status_Code));

   *code = status;
   eina_hash_add(_hash_statuses, path, code);
}


static void _list_status_free_cb(void *data)
{
   Edi_Scm_Status_Code *code = data;

   free(code);
}

static const char *
_icon_status(Edi_Scm_Status_Code code, Eina_Bool *staged)
{
   switch (code)
     {
        case EDI_SCM_STATUS_NONE:
        case EDI_SCM_STATUS_RENAMED:
           return "document-new";
        case EDI_SCM_STATUS_DELETED:
           return "edit-delete";
        case EDI_SCM_STATUS_UNKNOWN:
           return NULL;
        case EDI_SCM_STATUS_RENAMED_STAGED:
           *staged = EINA_TRUE;
           return "document-new";
        case EDI_SCM_STATUS_DELETED_STAGED:
           *staged = EINA_TRUE;
           return "edit-delete";
        case EDI_SCM_STATUS_ADDED:
           return "document-new";
        case EDI_SCM_STATUS_ADDED_STAGED:
           *staged = EINA_TRUE;
           return "document-new";
        case EDI_SCM_STATUS_MODIFIED:
           return "document-save-as";
        case EDI_SCM_STATUS_MODIFIED_STAGED:
           *staged = EINA_TRUE;
           return "document-save-as";
        case EDI_SCM_STATUS_UNTRACKED:
           return "dialog-question";
     }

   return NULL;
}

static void
_content_del(void *data, Evas_Object *obj EINA_UNUSED)
{
  char *path = data;

  free(path);
}

static Evas_Object *
_content_get(void *data, Evas_Object *obj, const char *source)
{
   Evas_Object *box, *lbox, *mbox, *rbox, *label, *ic;
   Edi_Scm_Status_Code *code;
   const char *text, *icon_name, *icon_status;
   char *path;
   Eina_Bool staged = EINA_FALSE;

   if (strcmp(source, "elm.swallow.content"))
     return NULL;

   path =  (char *) data;

   icon_name = icon_status = NULL;

   code = _file_status_item_find(path);
   if (code)
     icon_status = _icon_status(*code, &staged);

   icon_name = "dialog-information";

   box = elm_box_add(obj);
   elm_box_horizontal_set(box, EINA_TRUE);
   elm_box_align_set(box, 0, 0);

   lbox = elm_box_add(box);
   elm_box_horizontal_set(lbox, EINA_TRUE);
   elm_box_padding_set(lbox, 5, 0);
   evas_object_show(lbox);

   ic = elm_icon_add(lbox);
   elm_icon_standard_set(ic, icon_name);
   evas_object_size_hint_min_set(ic, ELM_SCALE_SIZE(16), ELM_SCALE_SIZE(16));
   evas_object_show(ic);
   elm_box_pack_end(lbox, ic);

   label = elm_label_add(lbox);
   elm_object_text_set(label, path);
   evas_object_show(label);
   elm_box_pack_end(lbox, label);

   mbox = elm_box_add(lbox);
   elm_box_horizontal_set(mbox, EINA_TRUE);
   evas_object_size_hint_weight_set(mbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(mbox);

   rbox = elm_box_add(mbox);
   elm_box_horizontal_set(rbox, EINA_TRUE);
   elm_box_padding_set(rbox, 5, 0);
   evas_object_show(rbox);

   if (icon_status)
     {
        ic = elm_icon_add(rbox);
        elm_icon_standard_set(ic, icon_status);
        evas_object_size_hint_min_set(ic, ELM_SCALE_SIZE(16), ELM_SCALE_SIZE(16));
        evas_object_show(ic);
        elm_box_pack_end(rbox, ic);

        if (staged)
          {
             ic = elm_icon_add(mbox);
             elm_icon_standard_set(ic, "dialog-information");
             evas_object_size_hint_min_set(ic, ELM_SCALE_SIZE(16), ELM_SCALE_SIZE(16));
             evas_object_show(ic);
             elm_box_pack_end(rbox, ic);
             text = _("Staged changes");
          }
        else
          {
             ic = elm_icon_add(mbox);
             elm_icon_standard_set(ic, "dialog-error");
             evas_object_size_hint_min_set(ic, ELM_SCALE_SIZE(16), ELM_SCALE_SIZE(16));
             evas_object_show(ic);
             elm_box_pack_end(rbox, ic);

             if (*code != EDI_SCM_STATUS_UNTRACKED)
               text = _("Unstaged changes");
             else
               text = _("Untracked changes");
          }

          elm_object_tooltip_text_set(box, text);
      }

   elm_box_pack_end(box, lbox);
   elm_box_pack_end(box, mbox);
   elm_box_pack_end(box, rbox);

   return box;
}

static Eina_Bool
_file_status_list_fill(Evas_Object *list)
{
   Edi_Scm_Engine *e;
   Edi_Scm_Status *status;
   Elm_Genlist_Item_Class *itc;
   Eina_Bool staged = EINA_FALSE;

   e = edi_scm_engine_get();
   if (!e)
     return EINA_FALSE;

   if (_hash_statuses)
     {
        eina_hash_free_buckets(_hash_statuses);
        eina_hash_free(_hash_statuses);
     }

   _hash_statuses = eina_hash_string_superfast_new(_list_status_free_cb);

   itc = elm_genlist_item_class_new();
   itc->item_style = "full";
   itc->func.text_get = NULL;
   itc->func.content_get = _content_get;
   itc->func.state_get = NULL;
   itc->func.del = _content_del;

   if (edi_scm_status_get())
     {
        EINA_LIST_FREE(e->statuses, status)
          {
             if (status->staged)
               {
                  staged = EINA_TRUE;
                  _file_status_item_add(status->path, status->change);
                  elm_genlist_item_append(list, itc, strdup(status->path), NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
               }

             eina_stringshare_del(status->fullpath);
             eina_stringshare_del(status->path);
             free(status);
          }
     }

   if (e->statuses)
     {
        eina_list_free(e->statuses);
        e->statuses = NULL;
     }

   elm_genlist_item_class_free(itc);

   return staged;
}

void
edi_scm_screens_commit(Evas_Object *parent)
{
   Evas_Object *popup, *box, *frame, *hbox, *cbox, *label, *avatar, *input, *button;
   Evas_Object *table, *rect, *list;
   Elm_Code_Widget *entry;
   Elm_Code *code;
   Eina_Strbuf *string;
   Edi_Scm_Engine *engine;
   char *text;
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

   string = eina_strbuf_new();
   eina_strbuf_append_printf(string, "%s:<br><b>%s</b><br>&lt;%s&gt;", _("Author"),
                             engine->remote_name_get(), engine->remote_email_get());
   elm_object_text_set(label, eina_strbuf_string_get(string));
   eina_strbuf_free(string);

   avatar = elm_image_add(hbox);
   edi_scm_screens_avatar_load(avatar, engine->remote_email_get());
   evas_object_size_hint_min_set(avatar, 48 * elm_config_scale_get(), 48 * elm_config_scale_get());
   evas_object_size_hint_weight_set(avatar, 0.1, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(avatar, 1.0, EVAS_HINT_FILL);
   evas_object_show(avatar);
   elm_box_pack_end(hbox, avatar);

   list = elm_genlist_add(box);
   elm_genlist_mode_set(list, ELM_LIST_SCROLL);
   elm_list_select_mode_set(list, ELM_OBJECT_SELECT_MODE_NONE);
   elm_scroller_bounce_set(list, EINA_TRUE, EINA_TRUE);
   elm_scroller_policy_set(list, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_ON);
   evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(list);

    // Start of trick
   table = elm_table_add(popup);
   evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(table, EVAS_HINT_FILL, EVAS_HINT_FILL);
   rect = evas_object_rectangle_add(table);
   evas_object_size_hint_weight_set(rect, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(rect, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_min_set(rect, 300 * elm_config_scale_get(), 100 * elm_config_scale_get());
   elm_table_pack(table, rect, 0, 0, 1, 1);
   evas_object_show(table);

   frame = elm_frame_add(popup);
   evas_object_size_hint_weight_set(frame, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(frame, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(frame, _("File changes"));
   evas_object_show(frame);
   elm_object_content_set(frame, table);
   elm_table_pack(table, list, 0, 0, 1, 1);
   elm_object_content_set(frame, table);
   elm_box_pack_end(box, frame);
   // End of Trick

   staged_changes = _file_status_list_fill(list);

   // Start of trick
   table = elm_table_add(popup);
   evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(table, EVAS_HINT_FILL, EVAS_HINT_FILL);
   rect = evas_object_rectangle_add(table);
   evas_object_size_hint_weight_set(rect, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(rect, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_min_set(rect, 300 * elm_config_scale_get(), 100 * elm_config_scale_get());
   elm_table_pack(table, rect, 0, 0, 1, 1);
   evas_object_show(table);

   frame = elm_frame_add(popup);
   evas_object_size_hint_weight_set(frame, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(frame, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(frame, _("Commit message"));
   evas_object_show(frame);
   elm_object_content_set(frame, table);

   input = elm_entry_add(box);
   elm_object_text_set(input, _("Enter commit summary<br><br>And change details<br>"));
   evas_object_size_hint_weight_set(input, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(input, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_entry_editable_set(input, staged_changes);
   elm_entry_scrollable_set(input, EINA_TRUE);
   elm_entry_single_line_set(input, EINA_FALSE);
   elm_entry_line_wrap_set(input, ELM_WRAP_WORD);

   elm_table_pack(table, input, 0, 0, 1, 1);
   evas_object_show(input);

   elm_object_content_set(frame, table);
   elm_box_pack_end(box, frame);
   // End of Trick

   text = edi_scm_diff(EINA_TRUE);
   if (text[0] && text[1])
     {
        frame = elm_frame_add(popup);
        evas_object_size_hint_weight_set(frame, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(frame, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_object_text_set(frame, _("Committed changes"));
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

        _entry_lines_append(code, text);
     }

   free(text);

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
   if (staged_changes)
     elm_entry_select_all(input);

   elm_object_focus_set(input, EINA_TRUE);

   evas_object_show(popup);
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

