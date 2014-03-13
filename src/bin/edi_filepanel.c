#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <libgen.h>

#include <Eina.h>
#include <Eio.h>
#include <elm_widget_fileselector.h>

#include "edi_filepanel.h"

#include "edi_private.h"

static Elm_Genlist_Item_Class itc, itc2;
static Evas_Object *list;
static edi_filepanel_item_clicked_cb _open_cb;

static Evas_Object *menu, *_main_win;
static const char *_menu_cb_path;

static void
_populate(Evas_Object *obj, const char *path, Elm_Object_Item *parent_it, const char *selected);

static void
_item_menu_open_cb(Elm_Object_Item *it EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   if (ecore_file_is_dir(_menu_cb_path))
     return;

   _open_cb(_menu_cb_path, NULL, EINA_FALSE);
}

static void
_item_menu_open_window_cb(Elm_Object_Item *it EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   if (ecore_file_is_dir(_menu_cb_path))
     return;

   _open_cb(_menu_cb_path, NULL, EINA_TRUE);
}

static void
_item_menu_xdgopen_cb(Elm_Object_Item *it EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
   int pid = fork();

   if (pid == 0)
     {
        execlp("/usr/bin/xdg-open", "xdg-open", _menu_cb_path, NULL);
        exit(0);
     }
}

static void
_item_menu_open_as_text_cb(Elm_Object_Item *it EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                           void *event_info EINA_UNUSED)
{
   _open_cb(_menu_cb_path, "text", EINA_FALSE);
}

static void
_item_menu_open_as_image_cb(Elm_Object_Item *it EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
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
   elm_menu_item_add(menu, menu_it, "txt", "text", _item_menu_open_as_text_cb, NULL);
   elm_menu_item_add(menu, menu_it, "image", "image", _item_menu_open_as_image_cb, NULL);
}

static void
_item_clicked_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj,
                 Evas_Event_Mouse_Down *ev)
{
   Elm_Object_Item *it;
   const char *path;

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

static Evas_Object *
_content_get(void *data, Evas_Object *obj, const char *source)
{
   if (!strcmp(source, "elm.swallow.icon"))
     {
        Evas_Object *ic;
        const char *iconpath;

        ic = elm_icon_add(obj);
        // TODO hook into the selected theme somehow (currently owned by E...)
        iconpath = efreet_mime_type_icon_get(data, "hicolor", 128);

        if (iconpath)
           elm_icon_file_set(ic, iconpath, NULL);
        else
          elm_icon_standard_set(ic, "file");

        evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
        evas_object_show(ic);
        return ic;
     }
   return NULL;
}

static void
_item_del(void *data, Evas_Object *obj)
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

static Eina_Bool
_ls_filter_cb(void *data,
              Eio_File *handler EINA_UNUSED,
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
   Elm_Object_Item *item;
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
   item = elm_genlist_item_sorted_insert(list, clas, path, lreq->parent_it,
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
_ls_error_cb(void *data, Eio_File *handler, int error EINA_UNUSED)
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

   /* TODO: sub directory should be monitored for expand mode */
   //sd->monitor = eio_monitor_add(path);
   //sd->current = 
   eio_file_stat_ls(path, _ls_filter_cb, _ls_main_cb,
                                  _ls_done_cb, _ls_error_cb, lreq);
}

void
edi_filepanel_add(Evas_Object *parent, Evas_Object *win,
                  const char *path, edi_filepanel_item_clicked_cb cb)
{
   list = elm_genlist_add(parent);
   evas_object_size_hint_min_set(list, 100, -1);
   evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(list);

   evas_object_event_callback_add(list, EVAS_CALLBACK_MOUSE_DOWN,
                                  _item_clicked_cb, NULL);

   evas_object_smart_callback_add(list, "expand,request", _on_list_expand_req, parent);
   evas_object_smart_callback_add(list, "contract,request", _on_list_contract_req, parent);
   evas_object_smart_callback_add(list, "expanded", _on_list_expanded, parent);
   evas_object_smart_callback_add(list, "contracted", _on_list_contracted, parent);

   itc.item_style = "default";
   itc.func.text_get = _text_get;
   itc.func.content_get = _content_get;
   itc.func.del = _item_del;

   itc2.item_style = "default";
   itc2.func.text_get = _text_get;
   itc2.func.content_get = _content_dir_get;
//   itc2.func.state_get = _state_get;
   itc2.func.del = _item_del;

   _open_cb = cb;
   _main_win = win;
   _populate(list, path, NULL, NULL);

   elm_box_pack_end(parent, list);
}
