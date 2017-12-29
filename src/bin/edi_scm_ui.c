#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "Edi.h"
#include <Eio.h>
#include "edi_scm_ui.h"
#include "edi_private.h"

#define DEFAULT_USER_ICON "applications-development"

typedef struct _Edi_Scm_Ui_Data {
   Ecore_Thread *thread;
   Eio_Monitor  *monitor;
   Elm_Code     *code;
   const char   *workdir;
   void         *data;

   Eina_Bool results_max;
   Eina_Bool is_configured;
   Eina_Bool in_progress;

   Evas_Object *parent;
   Evas_Object *list;
   Evas_Object *check;
   Evas_Object *commit_button;
   Evas_Object *commit_entry;

} Edi_Scm_Ui_Data;

const char *
_edi_scm_ui_avatar_cache_path_get(const char *email)
{
   return eina_stringshare_printf("%s/%s/avatars/%s.jpeg", efreet_cache_home_get(),
                                  PACKAGE_NAME, email);
}

void _edi_scm_ui_screens_avatar_download_complete(void *data, const char *file,
                                                  int status)
{
   Evas_Object *image = data;

   if (status != 200)
     {
        ecore_file_remove(file);
        return;
     }

   elm_photo_file_set(image, file);
}

static void
_edi_scm_ui_screens_avatar_load(Evas_Object *image, const char *email)
{
   char *tmp, *tmp2;
   const char *cache, *cachedir, *cacheparentdir;

   cache = _edi_scm_ui_avatar_cache_path_get(email);
   if (ecore_file_exists(cache))
     {
        elm_photo_file_set(image, cache);
        return;
     }

   tmp = strdup(cache);
   cachedir = dirname(tmp);
   tmp2 = strdup(tmp);
   cacheparentdir = dirname(tmp2);
   if ((ecore_file_exists(cacheparentdir) || ecore_file_mkdir(cacheparentdir))
       && (ecore_file_exists(cachedir) || ecore_file_mkdir(cachedir)))
     {
        ecore_file_download(edi_scm_avatar_url_get(email), cache,
                            _edi_scm_ui_screens_avatar_download_complete, NULL,
                            image, NULL);
     }
   else
     {
        elm_icon_standard_set(image, DEFAULT_USER_ICON);
     }

   free(tmp2);
   free(tmp);
}

static void
_edi_scm_ui_screens_message_close_cb(void *data EINA_UNUSED,
                                     Evas_Object *obj EINA_UNUSED,
                                     void *event_info EINA_UNUSED)
{
   Evas_Object *popup = data;

   evas_object_del(popup);
}

static void
_edi_scm_ui_screens_message_open(Evas_Object *parent, const char *message)
{
   Evas_Object *popup, *button;

   popup = elm_popup_add(parent);
   elm_object_part_text_set(popup, "title,text",
                            message);

   button = elm_button_add(popup);
   elm_object_text_set(button, _("OK"));
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked",
                                  _edi_scm_ui_screens_message_close_cb, popup);

   evas_object_show(popup);
}

static void
_edi_scm_ui_screens_cancel_cb(void *data, Evas_Object *obj EINA_UNUSED,
                              void *event_info EINA_UNUSED)
{
   Edi_Scm_Ui_Data *pd = data;

   if (pd->thread)
     ecore_thread_cancel(pd->thread);

   while ((ecore_thread_wait(pd->thread, 0.1)) != EINA_TRUE);

   evas_object_del(pd->parent);

   if (pd->monitor)
     eio_monitor_del(pd->monitor);

   free(pd);

   elm_exit();
}

static void
_edi_scm_ui_screens_commit_cb(void *data,
                              Evas_Object *obj EINA_UNUSED,
                              void *event_info EINA_UNUSED)
{
   Edi_Scm_Engine *engine;
   Edi_Scm_Ui_Data *pd;
   const char *text;
   char *message;

   engine = edi_scm_engine_get();
   if (!engine)
     return;

   pd = data;

   text = elm_object_text_get((Evas_Object *) pd->commit_entry);
   if (!text || !text[0])
     {
        _edi_scm_ui_screens_message_open(pd->parent, _("Please enter a valid commit message."));
        return;
     }

   message = elm_entry_markup_to_utf8(text);
   edi_scm_commit(message);

   free(message);

   if (pd->thread)
     ecore_thread_cancel(pd->thread);

   while ((ecore_thread_wait(pd->thread, 0.1)) != EINA_TRUE);

   evas_object_del(pd->parent);

   if (pd->monitor)
     eio_monitor_del(pd->monitor);

   free(pd);

   elm_exit();
}

static const char *
_icon_status(Edi_Scm_Status_Code code)
{
   switch (code)
     {
        case EDI_SCM_STATUS_NONE:
        case EDI_SCM_STATUS_UNKNOWN:
           return NULL;
        case EDI_SCM_STATUS_RENAMED:
           return "document-new";
        case EDI_SCM_STATUS_DELETED:
           return "edit-delete";
        case EDI_SCM_STATUS_RENAMED_STAGED:
           return "document-new";
        case EDI_SCM_STATUS_DELETED_STAGED:
           return "edit-delete";
        case EDI_SCM_STATUS_ADDED:
           return "document-new";
        case EDI_SCM_STATUS_ADDED_STAGED:
           return "document-new";
        case EDI_SCM_STATUS_MODIFIED:
           return "document-save-as";
        case EDI_SCM_STATUS_MODIFIED_STAGED:
           return "document-save-as";
        case EDI_SCM_STATUS_UNTRACKED:
           return "dialog-question";
     }

   return NULL;
}

static void
_edi_scm_ui_status_free(Edi_Scm_Status *status)
{
   eina_stringshare_del(status->fullpath);
   eina_stringshare_del(status->path);
   eina_stringshare_del(status->unescaped);

   free(status);
}

static void
_content_del(void *data, Evas_Object *obj EINA_UNUSED)
{
   Edi_Scm_Status *status = (Edi_Scm_Status *) data;

   _edi_scm_ui_status_free(status);
}

static Evas_Object *
_content_get(void *data, Evas_Object *obj, const char *source)
{
   Evas_Object *box, *lbox, *mbox, *rbox, *label, *ic;
   const char *text, *icon_file, *icon_status, *mime;
   Edi_Scm_Status *status;

   icon_file = NULL;

   if (strcmp(source, "elm.swallow.content"))
     return NULL;

   status = (Edi_Scm_Status *) data;

   mime = efreet_mime_type_get(status->fullpath);
   if (mime)
     icon_file = efreet_mime_type_icon_get(mime, elm_config_icon_theme_get(), 32);

   if (!icon_file)
     icon_file = "dialog-information";

   box = elm_box_add(obj);
   elm_box_horizontal_set(box, EINA_TRUE);
   elm_box_align_set(box, 0, 0);

   lbox = elm_box_add(box);
   elm_box_horizontal_set(lbox, EINA_TRUE);
   elm_box_padding_set(lbox, 5, 0);
   evas_object_show(lbox);

   ic = elm_icon_add(lbox);
   elm_icon_standard_set(ic, icon_file);
   evas_object_size_hint_min_set(ic, ELM_SCALE_SIZE(16), ELM_SCALE_SIZE(16));
   evas_object_show(ic);
   elm_box_pack_end(lbox, ic);

   label = elm_label_add(lbox);
   elm_object_text_set(label, status->unescaped);
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

   icon_status = _icon_status(status->change);
   if (icon_status)
     {
        ic = elm_icon_add(rbox);
        elm_icon_standard_set(ic, icon_status);
        evas_object_size_hint_min_set(ic, ELM_SCALE_SIZE(16), ELM_SCALE_SIZE(16));
        evas_object_show(ic);
        elm_box_pack_end(rbox, ic);

        if (status->staged)
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

             if (status->change != EDI_SCM_STATUS_UNTRACKED)
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
_edi_scm_ui_status_list_fill(Edi_Scm_Ui_Data *pd)
{
   Elm_Genlist_Item_Class *itc;
   Edi_Scm_Status *status;
   Edi_Scm_Engine *e;
   Eina_List *l;
   Eina_Bool staged = EINA_FALSE;
   Evas_Object *list = pd->list;

   e = edi_scm_engine_get();
   if (!e)
     return EINA_FALSE;

   itc = elm_genlist_item_class_new();
   itc->item_style = "full";
   itc->func.text_get = NULL;
   itc->func.content_get = _content_get;
   itc->func.state_get = NULL;
   itc->func.del = _content_del;

   if (!edi_scm_status_get())
     goto done;

   EINA_LIST_FOREACH(e->statuses, l, status)
     {
        if (status->staged)
          staged = EINA_TRUE;

        if (pd->results_max)
          {
             elm_genlist_item_append(list, itc, status, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
          }
        else
          {
             if (status->staged)
               elm_genlist_item_append(list, itc, status, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
             else
               _edi_scm_ui_status_free(status);
          }
     }

   if (e->statuses)
     {
        eina_list_free(e->statuses);
        e->statuses = NULL;
     }
done:
   elm_genlist_item_class_free(itc);

   return staged;
}

static void
_diff_widget_lines_append(Ecore_Thread *thread, Elm_Code *code, char *text)
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
            ecore_thread_main_loop_begin();
            elm_code_file_line_append(code->file, start, end - start, NULL);
            ecore_thread_main_loop_end();
            start = end + 1;
            end = NULL;
            if (ecore_thread_check(thread))
              return;
         }
    }

    end = pos;

    if (end > start)
      {
         ecore_thread_main_loop_begin();
         elm_code_file_line_append(code->file, start, end - start, NULL);
         ecore_thread_main_loop_end();
      }
}

static void
_edi_scm_diff_thread_cancel_cb(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Edi_Scm_Ui_Data *pd = data;

   pd->in_progress = EINA_FALSE;
   pd->thread = NULL;

   if (pd->data)
     {
        free(pd->data);
        pd->data = NULL;
     }
}

static void
_edi_scm_diff_thread_end_cb(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Edi_Scm_Ui_Data *pd = data;

   pd->in_progress = EINA_FALSE;
   pd->thread = NULL;
}

static void
_edi_scm_diff_thread_cb(void *data, Ecore_Thread *thread)
{
   Edi_Scm_Ui_Data *pd = data;

   if (pd->in_progress) return;

   pd->data = edi_scm_diff(!pd->results_max);

   pd->in_progress = EINA_TRUE;
   pd->thread = thread;

   _diff_widget_lines_append(thread, pd->code, pd->data);

   free(pd->data);
   pd->data = NULL;
}

static void
_edi_scm_ui_refresh(Edi_Scm_Ui_Data *pd)
{
   Eina_Bool staged;

   pd->results_max = elm_check_state_get(pd->check);

   elm_genlist_clear(pd->list);

   elm_code_file_clear(pd->code->file);

   staged = _edi_scm_ui_status_list_fill(pd);

   if (!pd->is_configured)
     {
        elm_object_disabled_set(pd->commit_button, EINA_TRUE);
        elm_entry_editable_set(pd->commit_entry, EINA_FALSE);
     }
   else
     {
        elm_object_disabled_set(pd->commit_button, !staged);
        elm_entry_editable_set(pd->commit_entry, staged);
     }

   elm_genlist_realized_items_update(pd->list);

   ecore_thread_run(_edi_scm_diff_thread_cb, _edi_scm_diff_thread_end_cb, _edi_scm_diff_thread_cancel_cb, pd);
}

static void
_edi_scm_ui_refresh_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Edi_Scm_Ui_Data *pd = data;

   if (pd->thread)
     ecore_thread_cancel(pd->thread);

   while ((ecore_thread_wait(pd->thread, 0.1)) != EINA_TRUE);

   if (pd->data)
     {
        free(pd->data);
        pd->data = NULL;
     }

   _edi_scm_ui_refresh(pd);
}

static Eina_Bool
_edi_scm_ui_file_changes_cb(void *data EINA_UNUSED, int type EINA_UNUSED,
                            void *event EINA_UNUSED)
{
   Edi_Scm_Ui_Data *pd = data;

   _edi_scm_ui_refresh(pd);

   return ECORE_CALLBACK_DONE;
}

static void
_item_menu_dismissed_cb(void *data EINA_UNUSED, Evas_Object *obj,
                        void *ev EINA_UNUSED)
{
   evas_object_del(obj);
}

static void
_item_menu_scm_stage_cb(void *data, Evas_Object *obj,
                        void *event_info EINA_UNUSED)
{
   Edi_Scm_Status *status;
   Edi_Scm_Ui_Data *pd = evas_object_data_get(obj, "edi_scm_ui");

   status = data;

   edi_scm_stage(status->path);

  _edi_scm_ui_refresh(pd);
}

static void
_item_menu_scm_unstage_cb(void *data, Evas_Object *obj,
                          void *event_info EINA_UNUSED)
{
   Edi_Scm_Status *status;
   Edi_Scm_Ui_Data *pd = evas_object_data_get(obj, "edi_scm_ui");

   status = data;

   edi_scm_unstage(status->path);

  _edi_scm_ui_refresh(pd);
}

static void
_item_menu_scm_staged_toggle(Edi_Scm_Status *status, Edi_Scm_Ui_Data *pd)
{
   if (status->staged)
     edi_scm_unstage(status->path);
   else
     edi_scm_stage(status->path);

  _edi_scm_ui_refresh(pd);
}

static Evas_Object *
_item_menu_create(Edi_Scm_Ui_Data *pd, Edi_Scm_Status *status)
{
   Evas_Object *menu, *parent;
   Elm_Object_Item *menu_it;

   parent = pd->parent;

   menu = elm_menu_add(parent);
   evas_object_data_set(menu, "edi_scm_ui", pd);
   evas_object_smart_callback_add(menu, "dismissed", _item_menu_dismissed_cb, NULL);

   menu_it = elm_menu_item_add(menu, NULL, "document-properties", basename((char *)status->path), NULL, NULL);
   elm_object_item_disabled_set(menu_it, EINA_TRUE);
   elm_menu_item_separator_add(menu, NULL);

   menu_it = elm_menu_item_add(menu, NULL, "document-save-as", _("Stage Changes"), _item_menu_scm_stage_cb, status);
   if (status->staged)
     elm_object_item_disabled_set(menu_it, EINA_TRUE);

   menu_it = elm_menu_item_add(menu, NULL, "edit-undo", _("Unstage Changes"), _item_menu_scm_unstage_cb, status);
   if (!status->staged)
     elm_object_item_disabled_set(menu_it, EINA_TRUE);

   return menu;
}

static void
_list_item_clicked_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                      void *event_info)
{
   Evas_Object *menu;
   Evas_Event_Mouse_Up *ev;
   Elm_Object_Item *it;
   Edi_Scm_Status *status;
   Edi_Scm_Ui_Data *pd = data;

   ev = event_info;
   it = elm_genlist_at_xy_item_get(obj, ev->output.x, ev->output.y, NULL);
   status = elm_object_item_data_get(it);

   if (!status)
     return;

   if (ev->button != 3)
     {
        if (ev->button == 1 && ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
          _item_menu_scm_staged_toggle(status, pd);
        return;
     }

   menu = _item_menu_create(pd, status);
   elm_menu_move(menu, ev->canvas.x, ev->canvas.y);
   evas_object_show(menu);
}

static void
_avatar_effect(Evas_Object *avatar)
{
   Evas_Map *map;
   int w, h;

   evas_object_move(avatar, 20 * elm_config_scale_get(), 20 * elm_config_scale_get());
   evas_object_resize(avatar, 72 * elm_config_scale_get(), 72 * elm_config_scale_get());
   evas_object_geometry_get(avatar, NULL, NULL, &w, &h);

   map = evas_map_new(4);
   evas_map_smooth_set(map, EINA_TRUE);
   evas_map_util_points_populate_from_object(map, avatar);
   evas_map_util_rotate(map, 8, w, h);
   evas_object_map_set(avatar, map);
   evas_object_map_enable_set(avatar, EINA_TRUE);
   evas_map_free(map);
}

void
edi_scm_ui_add(Evas_Object *parent)
{
   Evas_Object *box, *frame, *hbox, *cbox, *label, *avatar, *input, *button;
   Evas_Object *table, *rect, *list, *pbox, *check, *sep;
   Elm_Code_Widget *entry;
   Elm_Code *code;
   Eina_Strbuf *string;
   Edi_Scm_Engine *engine;
   Edi_Scm_Ui_Data *pd;
   const char *remote_name, *remote_email;
   Eina_Bool staged_changes;

   engine = edi_scm_engine_get();
   if (!engine)
     exit(1 << 1);

   pd = calloc(1, sizeof(Edi_Scm_Ui_Data));
   pd->workdir = engine->root_directory;
   pd->monitor = eio_monitor_add(pd->workdir);
   pd->parent = parent;
   pd->results_max = isatty(fileno(stdin));

   ecore_event_handler_add(EIO_MONITOR_FILE_CREATED, _edi_scm_ui_file_changes_cb, pd);
   ecore_event_handler_add(EIO_MONITOR_FILE_MODIFIED, _edi_scm_ui_file_changes_cb, pd);
   ecore_event_handler_add(EIO_MONITOR_FILE_DELETED, _edi_scm_ui_file_changes_cb, pd);
   ecore_event_handler_add(EIO_MONITOR_DIRECTORY_CREATED, _edi_scm_ui_file_changes_cb, pd);
   ecore_event_handler_add(EIO_MONITOR_DIRECTORY_MODIFIED, _edi_scm_ui_file_changes_cb, pd);
   ecore_event_handler_add(EIO_MONITOR_DIRECTORY_DELETED, _edi_scm_ui_file_changes_cb, pd);

   box = elm_box_add(parent);
   elm_box_horizontal_set(box, EINA_FALSE);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_content_set(parent, box);
   evas_object_show(box);

   frame = elm_frame_add(parent);
   elm_object_text_set(frame, _("User information"));
   evas_object_size_hint_weight_set(frame, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(frame, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(frame);

   hbox = elm_box_add(parent);
   elm_box_horizontal_set(hbox, EINA_TRUE);
   evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(hbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(hbox);

   remote_name = engine->remote_name_get();
   remote_email = engine->remote_email_get();

   if (remote_name[0] && remote_email[0])
     avatar = elm_photo_add(parent);
   else
     avatar = elm_icon_add(parent);

   evas_object_size_hint_min_set(avatar, 72 * elm_config_scale_get(), 72 * elm_config_scale_get());
   evas_object_size_hint_weight_set(avatar, 0.1, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(avatar, 1.0, EVAS_HINT_FILL);
   evas_object_show(avatar);
   elm_box_pack_end(hbox, avatar);

   /* General information */

   label = elm_label_add(hbox);
   evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, 1.0);
   evas_object_size_hint_align_set(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(label);
   elm_box_pack_end(hbox, label);

   pbox = elm_box_add(parent);
   elm_box_horizontal_set(pbox, EINA_TRUE);
   evas_object_size_hint_weight_set(pbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(pbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(pbox);
   elm_box_pack_end(hbox, pbox);

   string = eina_strbuf_new();

   if (!remote_name[0] && !remote_email[0])
     {
        eina_strbuf_append(string, _("Unable to obtain user information."));
        elm_icon_standard_set(avatar, DEFAULT_USER_ICON);
     }
   else
     {
        eina_strbuf_append_printf(string, "%s:<br><b>%s</b> &lt;%s&gt;", _("Author"),
                                  remote_name, remote_email);

        _edi_scm_ui_screens_avatar_load(avatar, engine->remote_email_get());
        _avatar_effect(avatar);
        pd->is_configured = EINA_TRUE;
     }

   elm_object_text_set(label, eina_strbuf_string_get(string));
   eina_strbuf_free(string);

   pd->check = check = elm_check_add(parent);
   elm_object_text_set(check, _("Show unstaged changes"));
   elm_check_state_set(check, pd->results_max);
   evas_object_show(check);
   evas_object_smart_callback_add(check, "changed",
                                  _edi_scm_ui_refresh_cb, pd);
   elm_box_pack_end(hbox, check);
   elm_object_content_set(frame, hbox);
   elm_box_pack_end(box, frame);

   /* File listing */
   hbox = elm_box_add(parent);
   elm_box_horizontal_set(hbox, EINA_TRUE);
   evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(hbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(hbox);

   pd->list = list = elm_genlist_add(box);
   elm_genlist_mode_set(list, ELM_LIST_SCROLL);
   elm_genlist_select_mode_set(list, ELM_OBJECT_SELECT_MODE_NONE);
   elm_scroller_bounce_set(list, EINA_TRUE, EINA_TRUE);
   elm_scroller_policy_set(list, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_ON);
   evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(list);
   evas_object_event_callback_add(list, EVAS_CALLBACK_MOUSE_UP, _list_item_clicked_cb, pd);

   table = elm_table_add(parent);
   evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(table, EVAS_HINT_FILL, EVAS_HINT_FILL);
   rect = evas_object_rectangle_add(table);
   evas_object_size_hint_weight_set(rect, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(rect, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_min_set(rect, 300 * elm_config_scale_get(), 100 * elm_config_scale_get());
   elm_table_pack(table, rect, 0, 0, 1, 1);
   evas_object_show(table);

   frame = elm_frame_add(parent);
   evas_object_size_hint_weight_set(frame, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(frame, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(frame, _("File changes"));
   evas_object_show(frame);
   elm_object_content_set(frame, table);
   elm_table_pack(table, list, 0, 0, 1, 1);
   elm_object_content_set(frame, table);
   elm_box_pack_end(hbox, frame);
   elm_box_pack_end(box, hbox);

   staged_changes = _edi_scm_ui_status_list_fill(pd);

   /* Commit entry */
   table = elm_table_add(parent);
   evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(table, EVAS_HINT_FILL, EVAS_HINT_FILL);
   rect = evas_object_rectangle_add(table);
   evas_object_size_hint_weight_set(rect, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(rect, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_min_set(rect, 300 * elm_config_scale_get(), 100 * elm_config_scale_get());
   elm_table_pack(table, rect, 0, 0, 1, 1);
   evas_object_show(table);

   frame = elm_frame_add(parent);
   evas_object_size_hint_weight_set(frame, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(frame, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(frame, _("Commit message"));
   evas_object_show(frame);
   elm_object_content_set(frame, table);

   pd->commit_entry = input = elm_entry_add(box);
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
   elm_box_pack_end(hbox, frame);
   elm_box_pack_end(box, hbox);

   /* Start of elm_code diff widget */
   frame = elm_frame_add(parent);
   evas_object_size_hint_weight_set(frame, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(frame, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(frame, _("Source changes"));
   evas_object_show(frame);

   cbox = elm_box_add(parent);
   evas_object_size_hint_weight_set(cbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(cbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_min_set(cbox, 350 * elm_config_scale_get(), 150 * elm_config_scale_get());
   evas_object_show(cbox);
   elm_object_content_set(frame, cbox);
   elm_box_pack_end(box, frame);

   pd->code = code = elm_code_create();
   entry = elm_code_widget_add(box, code);
   elm_code_parser_standard_add(code, ELM_CODE_PARSER_STANDARD_DIFF);
   elm_obj_code_widget_gravity_set(entry, 0.0, 0.0);
   elm_obj_code_widget_editable_set(entry, EINA_FALSE);
   elm_obj_code_widget_line_numbers_set(entry, EINA_FALSE);
   evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(entry);
   elm_box_pack_end(cbox, entry);

   ecore_thread_run(_edi_scm_diff_thread_cb, _edi_scm_diff_thread_end_cb, _edi_scm_diff_thread_cancel_cb, pd);

   sep = elm_separator_add(parent);
   elm_separator_horizontal_set(sep, EINA_TRUE);
   evas_object_show(sep);
   elm_box_pack_end(box, sep);

   /* Start of confirm and cancel buttons */
   hbox = elm_box_add(parent);
   evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(hbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_horizontal_set(hbox, EINA_TRUE);
   evas_object_show(hbox);

   button = elm_button_add(parent);
   evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(button, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(button);
   elm_object_text_set(button, _("Cancel"));
   evas_object_smart_callback_add(button, "clicked",
                                  _edi_scm_ui_screens_cancel_cb, pd);
   elm_box_pack_end(hbox, button);

   pd->commit_button = button = elm_button_add(parent);
   evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(button, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_data_set(button, "input", input);
   evas_object_show(button);
   elm_object_text_set(button, _("Commit"));
   elm_object_disabled_set(button, !staged_changes);
   evas_object_smart_callback_add(button, "clicked",
                                  _edi_scm_ui_screens_commit_cb, pd);

   elm_box_pack_end(hbox, button);
   elm_box_pack_end(box, hbox);
}

