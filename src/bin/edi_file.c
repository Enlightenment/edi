#include "Edi.h"
#include "mainview/edi_mainview.h"
#include "edi_file.h"
#include "edi_private.h"

static Evas_Object *_parent_obj, *_popup, *_popup_dir, *_edi_file_message_popup;
static const char *_directory_path;

Eina_Bool
edi_file_path_hidden(const char *path)
{
   Edi_Build_Provider *provider;

   provider = edi_build_provider_for_project_get();
   if (provider && provider->file_hidden_is(path))
     return EINA_TRUE;

   if (ecore_file_file_get(path)[0] == '.')
     return EINA_TRUE;

   return EINA_FALSE;
}

static void
_edi_file_message_close_cb(void *data EINA_UNUSED,
                     Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   Evas_Object *popup = data;
   evas_object_del(popup);
}

static void
_edi_file_message_open(const char *message)
{
   Evas_Object *popup, *button;

   _edi_file_message_popup = popup = elm_popup_add(_parent_obj);
   elm_object_part_text_set(popup, "title,text",
                           message);

   button = elm_button_add(popup);
   elm_object_text_set(button, "Ok");
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked",
                                 _edi_file_message_close_cb, popup);

   evas_object_show(popup);
}

static void
_edi_file_popup_cancel_cb(void *data, Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   evas_object_del((Evas_Object *)data);
   eina_stringshare_del(_directory_path);
   _directory_path = NULL;
}

static void
_edi_file_create_file_cb(void *data,
                             Evas_Object *obj EINA_UNUSED,
                             void *event_info EINA_UNUSED)
{
   const char *name;
   char *path;
   const char *directory = _directory_path;
   FILE *f;

   if (!ecore_file_is_dir(directory))
     return;

   name = elm_entry_entry_get((Evas_Object *) data);
   if (!name || strlen(name) == 0)
     {
        _edi_file_message_open("Please enter a file name.");
        return;
     }

   path = edi_path_append(directory, name);
   if ((ecore_file_exists(path) && ecore_file_is_dir(path)) ||
       !ecore_file_exists(path))
     {
        f = fopen(path, "w");
        if (f)
          {
             fclose(f);
             edi_mainview_open_path(path);
          }
        else
          _edi_file_message_open("Unable to write file.");
     }

   eina_stringshare_del(_directory_path);
   _directory_path = NULL;

   evas_object_del(_popup);
   free(path);
}

static void
_edi_file_create_dir_cb(void *data,
                             Evas_Object *obj EINA_UNUSED,
                             void *event_info EINA_UNUSED)
{
   const char *name;
   char *path;
   const char *directory = _directory_path;

   if (!ecore_file_is_dir(directory)) return;

   name = elm_entry_entry_get((Evas_Object *) data);
   if (!name || strlen(name) == 0)
     {
        _edi_file_message_open("Please enter a directory name.");
        return;
     }

   path = edi_path_append(directory, name);

   mkdir(path, 0755);
   
   eina_stringshare_del(_directory_path);
   _directory_path = NULL;

   evas_object_del(_popup_dir);
   free(path);
}

void
edi_file_create_file(Evas_Object *parent, const char *directory)
{
   Evas_Object *popup, *box, *input, *button;

   _parent_obj = parent;
   _popup = popup = elm_popup_add(parent);
   elm_object_part_text_set(popup, "title,text",
                            "Enter new file name");
   _directory_path = eina_stringshare_add(directory);

   box = elm_box_add(popup);
   elm_box_horizontal_set(box, EINA_FALSE);
   elm_object_content_set(popup, box);

   input = elm_entry_add(box);
   elm_entry_single_line_set(input, EINA_TRUE);
   evas_object_size_hint_weight_set(input, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_align_set(input, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(input);
   elm_box_pack_end(box, input);

   button = elm_button_add(popup);
   elm_object_text_set(button, "cancel");
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked",
                                       _edi_file_popup_cancel_cb, popup);

   button = elm_button_add(popup);
   elm_object_text_set(button, "create");
   elm_object_part_content_set(popup, "button2", button);
   evas_object_smart_callback_add(button, "clicked",
                                       _edi_file_create_file_cb, input);

   evas_object_show(popup);
   elm_object_focus_set(input, EINA_TRUE);
}

void
edi_file_create_dir(Evas_Object *parent, const char *directory)
{
   Evas_Object *popup, *box, *input, *button;

   _parent_obj = parent;
   _directory_path = eina_stringshare_add(directory);

   _popup_dir = popup = elm_popup_add(parent);
   elm_object_part_text_set(popup, "title,text",
                            "Enter new directory name");

   box = elm_box_add(popup);
   elm_box_horizontal_set(box, EINA_FALSE);
   elm_object_content_set(popup, box);

   input = elm_entry_add(box);
   elm_entry_single_line_set(input, EINA_TRUE);
   evas_object_size_hint_weight_set(input, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_align_set(input, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(input);
   elm_box_pack_end(box, input);

   button = elm_button_add(popup);
   elm_object_text_set(button, "cancel");
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked",
                                       _edi_file_popup_cancel_cb, popup);

   button = elm_button_add(popup);
   elm_object_text_set(button, "create");
   elm_object_part_content_set(popup, "button2", button);
   evas_object_smart_callback_add(button, "clicked",
                                       _edi_file_create_dir_cb, input);

   evas_object_show(popup);
   elm_object_focus_set(input, EINA_TRUE);
}

