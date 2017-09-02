#include "Edi.h"
#include "mainview/edi_mainview.h"
#include "edi_file_screens.h"
#include "edi_private.h"

static Evas_Object *_parent_obj, *_popup, *_popup_dir, *_edi_file_screens_message_popup;
static const char *_directory_path;

static void
_edi_file_screens_message_close_cb(void *data EINA_UNUSED,
                     Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   Evas_Object *popup = data;
   evas_object_del(popup);
}

static void
_edi_file_screens_message_open(const char *message)
{
   Evas_Object *popup, *button;

   _edi_file_screens_message_popup = popup = elm_popup_add(_parent_obj);
   elm_object_part_text_set(popup, "title,text",
                           message);

   button = elm_button_add(popup);
   elm_object_text_set(button, "Ok");
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked",
                                 _edi_file_screens_message_close_cb, popup);

   evas_object_show(popup);
}

static void
_edi_file_screens_popup_cancel_cb(void *data, Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   evas_object_del((Evas_Object *)data);
   eina_stringshare_del(_directory_path);
   _directory_path = NULL;
}

static void
_edi_file_screens_create_file_cb(void *data,
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
        _edi_file_screens_message_open("Please enter a file name.");
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
          _edi_file_screens_message_open("Unable to write file.");
     }

   eina_stringshare_del(_directory_path);
   _directory_path = NULL;

   evas_object_del(_popup);
   free(path);
}

static void
_edi_file_screens_create_dir_cb(void *data,
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
        _edi_file_screens_message_open("Please enter a directory name.");
        return;
     }

   path = edi_path_append(directory, name);

   mkdir(path, 0755);
   
   eina_stringshare_del(_directory_path);
   _directory_path = NULL;

   evas_object_del(_popup_dir);
   free(path);
}

static void
_edi_file_screens_rename_cb(void *data,
                    Evas_Object *obj,
                    void *event_info EINA_UNUSED)
{
   Evas_Object *entry;
   const char *name, *existing_path, *directory;
   char *path;

   directory = _directory_path;
   existing_path = (char *) data;

   entry = evas_object_data_get(obj, "input");

   name = elm_entry_entry_get(entry);
   if (!name || strlen(name) == 0)
     {
        _edi_file_screens_message_open("Please enter a valid name.");
        return;
     }

   path = edi_path_append(directory, name);

   if (ecore_file_exists(path))
     {
        if (ecore_file_is_dir(path))
          _edi_file_screens_message_open("Directory with that name already exists.");
        else
          _edi_file_screens_message_open("File with that name already exists.");
        return;
     }

   eina_stringshare_del(_directory_path);
   _directory_path = NULL;

   if (strcmp(existing_path, path))
     {
        if (!ecore_file_is_dir(existing_path))
          edi_mainview_item_close_path(existing_path);

        if (!edi_scm_enabled())
          ecore_file_mv(existing_path, path);
        else
          edi_scm_move(existing_path, path);
     }

   evas_object_del(_popup);
   free(path);
}

void
edi_file_screens_rename(Evas_Object *parent, const char *path)
{
   Evas_Object *table, *popup, *box, *input, *button, *sep, *label, *icon;
   const char *leaf;

   _parent_obj = parent;
   _popup = popup = elm_popup_add(parent);

   if (ecore_file_is_dir(path))
     elm_object_part_text_set(popup, "title,text",
                                     "Rename directory");
   else
     elm_object_part_text_set(popup, "title,text",
                                     "Rename file");
   leaf = ecore_file_file_get(path);
   _directory_path = eina_stringshare_add(ecore_file_dir_get(path));


   table = elm_table_add(popup);
   icon = elm_icon_add(table);
   elm_icon_standard_set(icon, "dialog-information");
   evas_object_size_hint_min_set(icon, 48 * elm_config_scale_get(), 48 * elm_config_scale_get());
   evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(icon);
   elm_table_pack(table, icon, 0, 0, 1, 1);

   box = elm_box_add(popup);
   label = elm_label_add(popup);
   if (ecore_file_is_dir(path))
     elm_object_text_set(label, "Please enter a new name for this directory.");
   else
     elm_object_text_set(label, "Please enter a new name for this file.");
   evas_object_show(label);

   elm_box_pack_end(box, label);

   input = elm_entry_add(box);
   elm_entry_single_line_set(input, EINA_TRUE);
   elm_entry_editable_set(input, EINA_TRUE);
   elm_entry_scrollable_set(input, EINA_TRUE);
   elm_object_text_set(input, leaf);
   evas_object_size_hint_weight_set(input, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(input, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(input);
   elm_box_pack_end(box, input);
   evas_object_show(box);

   sep = elm_separator_add(box);
   elm_separator_horizontal_set(sep, EINA_TRUE);
   evas_object_show(sep);
   elm_box_pack_end(box, sep);
   elm_table_pack(table, box, 1, 0, 1, 1);
   evas_object_show(table);

   elm_object_content_set(popup, table);

   button = elm_button_add(popup);
   elm_object_text_set(button, "Cancel");
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked",
                                  _edi_file_screens_popup_cancel_cb, popup);

   button = elm_button_add(popup);
   evas_object_data_set(button, "input", input);
   elm_object_text_set(button, "Rename");
   elm_object_part_content_set(popup, "button2", button);
   evas_object_smart_callback_add(button, "clicked",
                                  _edi_file_screens_rename_cb, path);

   evas_object_show(popup);
   elm_object_focus_set(input, EINA_TRUE);
}

void
edi_file_screens_create_file(Evas_Object *parent, const char *directory)
{
   Evas_Object *table, *popup, *box, *input, *button, *sep, *label, *icon;

   _parent_obj = parent;
   _popup = popup = elm_popup_add(parent);
   elm_object_part_text_set(popup, "title,text",
                            "Enter new file name");
   _directory_path = eina_stringshare_add(directory);

   table = elm_table_add(popup);
   icon = elm_icon_add(table);
   elm_icon_standard_set(icon, "document-new");
   evas_object_size_hint_min_set(icon, 48 * elm_config_scale_get(), 48 * elm_config_scale_get());
   evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(icon);
   elm_table_pack(table, icon, 0, 0, 1, 1);

   box = elm_box_add(popup);
   label = elm_label_add(popup);
   elm_object_text_set(label, "Please enter a name for this new file.");
   evas_object_show(label);

   elm_box_pack_end(box, label);

   input = elm_entry_add(box);
   elm_entry_single_line_set(input, EINA_TRUE);
   elm_entry_editable_set(input, EINA_TRUE);
   elm_entry_scrollable_set(input, EINA_TRUE);
   evas_object_size_hint_weight_set(input, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(input, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(input);
   elm_box_pack_end(box, input);
   evas_object_show(box);

   sep = elm_separator_add(box);
   elm_separator_horizontal_set(sep, EINA_TRUE);
   evas_object_show(sep);

   elm_box_pack_end(box, sep);
   elm_table_pack(table, box, 1, 0, 1, 1);
   evas_object_show(table);

   elm_object_content_set(popup, table);

   button = elm_button_add(popup);
   elm_object_text_set(button, "Cancel");
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked",
                                       _edi_file_screens_popup_cancel_cb, popup);

   button = elm_button_add(popup);
   elm_object_text_set(button, "Create");
   elm_object_part_content_set(popup, "button2", button);
   evas_object_smart_callback_add(button, "clicked",
                                       _edi_file_screens_create_file_cb, input);

   evas_object_show(popup);
   elm_object_focus_set(input, EINA_TRUE);
}

void
edi_file_screens_create_dir(Evas_Object *parent, const char *directory)
{
   Evas_Object *table, *popup, *box, *input, *button, *sep, *label, *icon;

   _parent_obj = parent;
   _directory_path = eina_stringshare_add(directory);

   _popup_dir = popup = elm_popup_add(parent);
   elm_object_part_text_set(popup, "title,text",
                            "Enter new directory name");

   table = elm_table_add(popup);
   icon = elm_icon_add(table);
   elm_icon_standard_set(icon, "folder-new");
   evas_object_size_hint_min_set(icon, 48 * elm_config_scale_get(), 48 * elm_config_scale_get());
   evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(icon);
   elm_table_pack(table, icon, 0, 0, 1, 1);

   box = elm_box_add(popup);
   label = elm_label_add(popup);
   elm_object_text_set(label, "Please enter a name for this new directory.");
   evas_object_show(label);

   elm_box_pack_end(box, label);

   input = elm_entry_add(box);
   elm_entry_single_line_set(input, EINA_TRUE);
   elm_entry_editable_set(input, EINA_TRUE);
   elm_entry_scrollable_set(input, EINA_TRUE);
   evas_object_size_hint_weight_set(input, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(input, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(input);
   elm_box_pack_end(box, input);
   evas_object_show(box);

   sep = elm_separator_add(box);
   elm_separator_horizontal_set(sep, EINA_TRUE);
   evas_object_show(sep);

   elm_box_pack_end(box, sep);
   elm_table_pack(table, box, 1, 0, 1, 1);
   evas_object_show(table);

   elm_object_content_set(popup, table);

   button = elm_button_add(popup);
   elm_object_text_set(button, "Cancel");
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked",
                                       _edi_file_screens_popup_cancel_cb, popup);

   button = elm_button_add(popup);
   elm_object_text_set(button, "Create");
   elm_object_part_content_set(popup, "button2", button);
   evas_object_smart_callback_add(button, "clicked",
                                       _edi_file_screens_create_dir_cb, input);

   evas_object_show(popup);
   elm_object_focus_set(input, EINA_TRUE);
}

