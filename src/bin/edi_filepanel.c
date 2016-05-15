#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <libgen.h>
#include <regex.h>

#include <Eina.h>
#include <Eio.h>
#include <elm_widget_fileselector.h>

#include "Edi.h"

#include "edi_filepanel.h"
#include "edi_content_provider.h"

#include "edi_private.h"

static Elm_Genlist_Item_Class itc, itc2;
static Evas_Object *list;
static edi_filepanel_item_clicked_cb _open_cb;

static Evas_Object *menu, *_main_win, *_filepanel_box, *_filter_box, *_filter;
static const char *_menu_cb_path, *_root_path;
static regex_t _filter_regex;
static Eina_Bool _filter_set = EINA_FALSE;

static void
_populate(Evas_Object *obj, const char *path, Elm_Object_Item *parent_it, const char *selected);

static void
_item_menu_open_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   if (ecore_file_is_dir(_menu_cb_path))
     return;

   _open_cb(_menu_cb_path, NULL, EINA_FALSE);
}

static void
_item_menu_open_window_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   if (ecore_file_is_dir(_menu_cb_path))
     return;

   _open_cb(_menu_cb_path, NULL, EINA_TRUE);
}

static void
_item_menu_xdgopen_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
   char *cmd;
   int cmdlen;
   const char *format = "xdg-open \"%s\"";

   cmdlen = strlen(format) + strlen(_menu_cb_path) - 1;
   cmd = malloc(sizeof(char) * cmdlen);
   snprintf(cmd, cmdlen, format, _menu_cb_path);

   ecore_exe_run(cmd, NULL);
   free(cmd);
}

static void
_item_menu_open_as_text_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                           void *event_info EINA_UNUSED)
{
   _open_cb(_menu_cb_path, "text", EINA_FALSE);
}

static void
_item_menu_open_as_code_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                           void *event_info EINA_UNUSED)
{
   _open_cb(_menu_cb_path, "code", EINA_FALSE);
}

static void
_item_menu_open_as_image_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                            void *event_info EINA_UNUSED)
{
   _open_cb(_menu_cb_path, "image", EINA_FALSE);
}

static void
_item_menu_dismissed_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                        void *ev EINA_UNUSED)
{
   eina_stringshare_del(_menu_cb_path);
   _menu_cb_path = NULL;
}

static void
_item_menu_filetype_create(Evas_Object *menu, Elm_Object_Item *parent, const char *type,
                           Evas_Smart_Cb func)
{
   Edi_Content_Provider *provider;

   provider = edi_content_provider_for_id_get(type);
   if (!provider)
     return;

   elm_menu_item_add(menu, parent, provider->icon, provider->id, func, NULL);
}

static void
_item_menu_create(Evas_Object *win)
{
   Elm_Object_Item *menu_it;

   menu = elm_menu_add(win);
   evas_object_smart_callback_add(menu, "dismissed", _item_menu_dismissed_cb, NULL);

   elm_menu_item_add(menu, NULL, "fileopen", "open", _item_menu_open_cb, NULL);
   elm_menu_item_add(menu, NULL, "window-new", "open new window", _item_menu_open_window_cb, NULL);

   menu_it = elm_menu_item_add(menu, NULL, "gtk-execute", "open external",
                               _item_menu_xdgopen_cb, NULL);
   menu_it = elm_menu_item_add(menu, NULL, NULL, "open as", NULL, NULL);
   _item_menu_filetype_create(menu, menu_it, "text", _item_menu_open_as_text_cb);
   _item_menu_filetype_create(menu, menu_it, "code", _item_menu_open_as_code_cb);
   _item_menu_filetype_create(menu, menu_it, "image", _item_menu_open_as_image_cb);
}

static void
_item_clicked_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj,
                 void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   Elm_Object_Item *it;
   const char *path;

   ev = event_info;
   if (ev->button != 3) return;

   it = elm_genlist_at_xy_item_get(obj, ev->output.x, ev->output.y, NULL);
   path = elm_object_item_data_get(it);

   if (!menu)
     _item_menu_create(_main_win);

   _menu_cb_path = eina_stringshare_add(path);
   elm_menu_move(menu, ev->canvas.x, ev->canvas.y);
   evas_object_show(menu);
}

static char *
_text_get(void *data, Evas_Object *obj EINA_UNUSED, const char *source EINA_UNUSED)
{
   return strdup(basename(data));
}

static Eina_Hash *mime_entries = NULL;

static Edi_Content_Provider*
_get_provider_from_hashset(const char *filename)
{
   if ( mime_entries == NULL ) mime_entries = eina_hash_string_superfast_new(NULL);
   const char *mime = eina_hash_find(mime_entries, filename);
   if ( !mime )
     {
       mime = efreet_mime_type_get(filename);
       eina_hash_add(mime_entries, filename, strdup(mime));
     }
   return edi_content_provider_for_mime_get(mime);
}

static Evas_Object *
_content_get(void *data, Evas_Object *obj, const char *source)
{
   if (strcmp(source, "elm.swallow.icon"))
     return NULL;

   Edi_Content_Provider *provider;
   provider = _get_provider_from_hashset((const char *)data);

   Evas_Object *ic;
   ic = elm_icon_add(obj);
   if (provider)
     elm_icon_standard_set(ic, provider->icon);
   else
     elm_icon_standard_set(ic, "empty");

   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   evas_object_show(ic);

   return ic;
}

static void
_item_del(void *data, Evas_Object *obj EINA_UNUSED)
{
   eina_stringshare_del(data);
}

static void
_item_sel(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   const char *path = data;

   if (!ecore_file_is_dir(path))
     _open_cb(path, NULL, EINA_FALSE);
}

static Evas_Object *
_content_dir_get(void *data EINA_UNUSED, Evas_Object *obj, const char *source)
{
   Evas_Object *ic;

   if (strcmp(source, "elm.swallow.icon"))
     return NULL;

   ic = elm_icon_add(obj);
   elm_icon_standard_set(ic, "folder");
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   evas_object_show(ic);
   return ic;
}

static Eina_Bool
_ls_filter_cb(void *data EINA_UNUSED, Eio_File *handler EINA_UNUSED,
              const Eina_File_Direct_Info *info)
{
   return info->path[info->name_start] != '.';
}

static int
_file_list_cmp(const void *data1, const void *data2)
{
   const Elm_Object_Item *item1 = data1;
   const Elm_Object_Item *item2 = data2;
   const Elm_Genlist_Item_Class *ca = elm_genlist_item_item_class_get(item1);
   const Elm_Genlist_Item_Class *cb = elm_genlist_item_item_class_get(item2);

   if (ca == &itc2)
     {
        if (cb != &itc2)
          return -1;
     }
   else if (cb == &itc2)
     {
        return 1;
     }

   const char *name1 = elm_object_item_data_get(item1);
   const char *name2 = elm_object_item_data_get(item2);

   return strcasecmp(name1, name2);
}

static void
_listing_request_cleanup(Listing_Request *lreq)
{
   eina_stringshare_del(lreq->path);
   eina_stringshare_del(lreq->selected);
   free(lreq);
}

static void
_on_list_expand_req(void *data       EINA_UNUSED,
                    Evas_Object *obj EINA_UNUSED,
                    void *event_info)
{
   Elm_Object_Item *it = event_info;

   elm_genlist_item_expanded_set(it, EINA_TRUE);
}

static void
_on_list_contract_req(void *data       EINA_UNUSED,
                      Evas_Object *obj EINA_UNUSED,
                      void *event_info)
{
   Elm_Object_Item *it = event_info;

   elm_genlist_item_expanded_set(it, EINA_FALSE);
}

static void
_on_list_expanded(void *data,
                  Evas_Object *obj EINA_UNUSED,
                  void *event_info)
{
   Elm_Object_Item *it = event_info;
   const char *path = elm_object_item_data_get(it);

   _populate(data, path, it, NULL);
}

static void
_on_list_contracted(void *data       EINA_UNUSED,
                    Evas_Object *obj EINA_UNUSED,
                    void *event_info)
{
   Elm_Object_Item *it = event_info;

   elm_genlist_item_subitems_clear(it);
}

static void
_ls_main_cb(void *data,
            Eio_File *handler,
            const Eina_File_Direct_Info *info)
{
   Listing_Request *lreq = data;
   Elm_Genlist_Item_Class *clas = &itc;
   const char  *path;

   if (eio_file_check(handler)) return;

   //if (!lreq->sd->files_view || lreq->sd->current != handler)
     //{
        //eio_file_cancel(handler);
        //return;
     //}

//   _signal_first(lreq);

   if (info->type == EINA_FILE_DIR)
     {
        clas = &itc2;
     }

   path = eina_stringshare_add(info->path);
   (void)!elm_genlist_item_sorted_insert(list, clas, path, lreq->parent_it,
                                         (clas == &itc2) ? ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE,
                                         _file_list_cmp, _item_sel, path);
}

static void
_ls_done_cb(void *data, Eio_File *handler EINA_UNUSED)
{
   Listing_Request *lreq = data;

   _listing_request_cleanup(lreq);
}

static void
_ls_error_cb(void *data, Eio_File *handler EINA_UNUSED, int error EINA_UNUSED)
{
   Listing_Request *lreq = data;

   _listing_request_cleanup(lreq);
}

static void
_populate(Evas_Object *obj,
          const char *path,
          Elm_Object_Item *parent_it,
          const char *selected)
{
   if (!path) return;

   Listing_Request *lreq;

   //if (sd->monitor) eio_monitor_del(sd->monitor);
   //if (sd->current) eio_file_cancel(sd->current);

   lreq = malloc(sizeof (Listing_Request));
   if (!lreq) return;

   lreq->parent_it = parent_it;
   lreq->obj = obj;
   lreq->path = eina_stringshare_add(path);
   lreq->first = EINA_TRUE;

   if (selected)
     lreq->selected = eina_stringshare_add(selected);
   else
     lreq->selected = NULL;

   // FIXME re-enable the monitors once we have a less intrusive manner of refreshing the file tree
   /* TODO: keep track of monitors so they can be cleaned */
   //sd->monitor = eio_monitor_add(path);
   //sd->current = 
   eio_file_stat_ls(path, _ls_filter_cb, _ls_main_cb,
                                  _ls_done_cb, _ls_error_cb, lreq);
}

static void
_file_listing_reload(const char *dir)
{
   elm_genlist_clear(list);
   _populate(list, dir, NULL, NULL);
}

static void
_file_listing_updated(void *data, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   const char *dir;

   dir = (const char *)data;
   DBG("File created in %s\n", dir);

   _file_listing_reload(dir);
}

void
_edi_filepanel_reload()
{
   char *dir;

   dir = realpath(edi_project_get(), NULL);
   _file_listing_reload(dir);

   free(dir);
}

/* Panel filtering */

static Eina_Bool
_filter_get(void *data, Evas_Object *obj EINA_UNUSED, void *key EINA_UNUSED)
{
   const char *relative;

   if (!_filter_set) return EINA_TRUE;

   relative = (char *)data + strlen(_root_path);
   return !regexec(&_filter_regex, relative, 0, NULL, 0);
}

static void
_filter_clear(Evas_Object *filter)
{
   elm_object_text_set(filter, NULL);
   _filter_set = EINA_FALSE;
   regfree(&_filter_regex);
}

static void
_filter_clear_bt_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _filter_clear((Evas_Object *)data);
}

static void
_filter_cancel_bt_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _filter_clear((Evas_Object *)data);

   evas_object_hide(_filter_box);
   elm_box_unpack(_filepanel_box, _filter_box);
}

static void
_filter_key_down_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *tree;
   const char *match;

   tree = (Evas_Object *)data;
   match = elm_object_text_get(obj);

   regfree(&_filter_regex);
   _filter_set = !regcomp(&_filter_regex, match, REG_NOSUB | REG_ICASE);

   if (!match || strlen(match) == 0 || !_filter_set)
     elm_genlist_filter_set(tree, NULL);
   else
     elm_genlist_filter_set(tree, (void *)strdup(match));
}

void
edi_filepanel_search()
{
   elm_box_pack_start(_filepanel_box, _filter_box);
   evas_object_show(_filter_box);
   elm_object_focus_set(_filter, EINA_TRUE);
}

/* Panel setup */

void
edi_filepanel_add(Evas_Object *parent, Evas_Object *win,
                  const char *path, edi_filepanel_item_clicked_cb cb)
{
   Evas_Object *box, *hbox, *filter, *clear, *cancel, *icon;

   box = elm_box_add(parent);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_homogeneous_set(box, EINA_FALSE);
   evas_object_show(box);
   elm_box_pack_end(parent, box);
   _filepanel_box = box;

   hbox = elm_box_add(box);
   evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(hbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_horizontal_set(hbox, EINA_TRUE);
   elm_box_homogeneous_set(hbox, EINA_FALSE);
   _filter_box = hbox;

   filter = elm_entry_add(hbox);
   elm_entry_scrollable_set(filter, EINA_TRUE);
   elm_entry_single_line_set(filter, EINA_TRUE);
   elm_object_part_text_set(filter, "guide", "Find file");
   evas_object_size_hint_weight_set(filter, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(filter, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_entry_editable_set(filter, EINA_TRUE);
   evas_object_show(filter);
   elm_box_pack_end(hbox, filter);
   _filter = filter;

   clear = elm_button_add(hbox);
   evas_object_smart_callback_add(clear, "clicked", _filter_clear_bt_cb, filter);
   evas_object_show(clear);
   elm_box_pack_end(hbox, clear);

   icon = elm_icon_add(clear);
   evas_object_size_hint_min_set(icon, 14 * elm_config_scale_get(), 14 * elm_config_scale_get());
   elm_icon_standard_set(icon, "edit-clear");
   elm_object_part_content_set(clear, "icon", icon);

   cancel = elm_button_add(hbox);
   evas_object_smart_callback_add(cancel, "clicked", _filter_cancel_bt_cb, filter);
   evas_object_show(cancel);
   elm_box_pack_end(hbox, cancel);

   icon = elm_icon_add(cancel);
   evas_object_size_hint_min_set(icon, 14 * elm_config_scale_get(), 14 * elm_config_scale_get());
   elm_icon_standard_set(icon, "window-close");
   elm_object_part_content_set(cancel, "icon", icon);

   list = elm_genlist_add(parent);
   elm_genlist_homogeneous_set(list, EINA_TRUE);
   elm_genlist_select_mode_set(list, ELM_OBJECT_SELECT_MODE_ALWAYS);
   evas_object_size_hint_min_set(list, 100, -1);
   evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(list);
   elm_box_pack_end(box, list);

   _root_path = eina_stringshare_add(path);
   evas_object_event_callback_add(list, EVAS_CALLBACK_MOUSE_DOWN,
                                  _item_clicked_cb, NULL);
   ecore_event_handler_add(EIO_MONITOR_FILE_CREATED, (Ecore_Event_Handler_Cb)_file_listing_updated, _root_path);
   ecore_event_handler_add(EIO_MONITOR_FILE_DELETED, (Ecore_Event_Handler_Cb)_file_listing_updated, _root_path);
   ecore_event_handler_add(EIO_MONITOR_DIRECTORY_CREATED, (Ecore_Event_Handler_Cb)_file_listing_updated, _root_path);
   ecore_event_handler_add(EIO_MONITOR_DIRECTORY_DELETED, (Ecore_Event_Handler_Cb)_file_listing_updated, _root_path);

   evas_object_smart_callback_add(list, "expand,request", _on_list_expand_req, parent);
   evas_object_smart_callback_add(list, "contract,request", _on_list_contract_req, parent);
   evas_object_smart_callback_add(list, "expanded", _on_list_expanded, parent);
   evas_object_smart_callback_add(list, "contracted", _on_list_contracted, parent);

   itc.item_style = "default";
   itc.func.text_get = _text_get;
   itc.func.content_get = _content_get;
   itc.func.filter_get = _filter_get;
   itc.func.del = _item_del;

   itc2.item_style = "default";
   itc2.func.text_get = _text_get;
   itc2.func.content_get = _content_dir_get;
//   itc2.func.state_get = _state_get;
   itc2.func.del = _item_del;

   _open_cb = cb;
   _main_win = win;
   _populate(list, path, NULL, NULL);
   evas_object_smart_callback_add(filter, "changed", _filter_key_down_cb, list);
}

const char *
edi_filepanel_selected_path_get(Evas_Object *obj EINA_UNUSED)
{
   Elm_Object_Item *it;

   it = elm_genlist_selected_item_get(list);
   return elm_object_item_data_get(it);
}

