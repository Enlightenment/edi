#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Elementary.h>

#include "edi_welcome.h"
#include "edi_private.h"

#include <stdlib.h>
#include <sys/wait.h>

#define _EDI_WELCOME_PROJECT_NEW_TABLE_WIDTH 4

static Evas_Object *_welcome_window;
static Evas_Object *_create_inputs[5];

static void
_edi_welcome_project_open(const char *path)
{
   evas_object_del(_welcome_window);

   edi_project_set(path);
   edi_open(edi_project_get());
}

static void
_edi_welcome_project_chosen_cb(void *data,
                       Evas_Object *obj EINA_UNUSED,
                       void *event_info)
{
   evas_object_del(data);

   if (event_info)
     {
        _edi_welcome_project_open((const char*)event_info);
     }
}

static void
_edi_welcome_choose_exit(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   evas_object_del(data);
}

static void
_edi_welcome_project_choose_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *fs;

   elm_need_ethumb();
   elm_need_efreet();

   win = elm_win_util_standard_add("projectselector", "Choose a Project Folder");
   if (!win) return;

   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   evas_object_smart_callback_add(win, "delete,request", _edi_welcome_choose_exit, win);

   fs = elm_fileselector_add(win);
   evas_object_size_hint_weight_set(fs, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(fs, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(fs, "done", _edi_welcome_project_chosen_cb, win);
   elm_win_resize_object_add(win, fs);
   evas_object_show(fs);

   elm_fileselector_expandable_set(fs, EINA_TRUE);
   elm_fileselector_folder_only_set(fs, EINA_TRUE);
   elm_fileselector_path_set(fs, getenv("HOME"));
   elm_fileselector_sort_method_set(fs, ELM_FILESELECTOR_SORT_BY_FILENAME_ASC);

   evas_object_resize(win, 380 * elm_config_scale_get(), 260 * elm_config_scale_get());
   evas_object_show(win);
}

static void
_edi_welcome_project_new_directory_row_add(const char *text, const char *placeholder, int row,
   Evas_Object *parent)
{
   Evas_Object *label, *input;

   label = elm_label_add(parent);
   elm_object_text_set(label, text);
   evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(parent, label, 0, row, 1, 1);
   evas_object_show(label);

   input = elm_fileselector_entry_add(parent);
   elm_object_text_set(input, "Select folder");
   elm_fileselector_folder_only_set(input, EINA_TRUE);
   evas_object_size_hint_weight_set(input, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(input, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(parent, input, 1, row, _EDI_WELCOME_PROJECT_NEW_TABLE_WIDTH - 1, 1);
   evas_object_show(input);

   if (placeholder)
     {
        elm_object_text_set(input, placeholder);
     }
   _create_inputs[row] = input;
}

static void
_edi_welcome_project_new_input_row_add(const char *text, const char *placeholder, int row,
   Evas_Object *parent)
{
   Evas_Object *label, *input;

   label = elm_label_add(parent);
   elm_object_text_set(label, text);
   evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(parent, label, 0, row, 1, 1);
   evas_object_show(label);

   input = elm_entry_add(parent);
   elm_entry_scrollable_set(input, EINA_TRUE);
   elm_entry_single_line_set(input, EINA_TRUE);
   evas_object_size_hint_weight_set(input, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(input, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(parent, input, 1, row, _EDI_WELCOME_PROJECT_NEW_TABLE_WIDTH - 1, 1);
   evas_object_show(input);

   if (placeholder)
     {
        elm_object_text_set(input, placeholder);
     }
   _create_inputs[row] = input;
}

static void
_edi_welcome_project_new_create_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   const char *path, *name, *user, *email, *url;
   char script[PATH_MAX], fullpath[PATH_MAX];

   path = elm_fileselector_path_get(_create_inputs[0]);
   name = elm_object_text_get(_create_inputs[1]);
   url = elm_object_text_get(_create_inputs[2]);
   user = elm_object_text_get(_create_inputs[3]);
   email = elm_object_text_get(_create_inputs[4]);

   snprintf(script, sizeof(script), "%s/skeleton/eflprj", elm_app_data_dir_get());
   snprintf(fullpath, sizeof(fullpath), "%s/%s", path, name);
   int pid = fork();

   if (pid == 0)
     {
        printf("Creating project \"%s\" at path %s for %s<%s>\n", name, fullpath, user, email);

        execlp(script, script, fullpath, name, user, email, url, NULL);
        exit(0);
     }
   waitpid(pid, NULL, 0);
   _edi_welcome_project_open(fullpath);
}

static void
_edi_welcome_project_new_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *content, *button, *naviframe = data;
   Elm_Object_Item *item;
   int row = 0;

   content = elm_table_add(naviframe);
   elm_table_homogeneous_set(content, EINA_TRUE);
   evas_object_size_hint_weight_set(content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(content);

   _edi_welcome_project_new_directory_row_add("Parent Path", NULL, row++, content);
   _edi_welcome_project_new_input_row_add("Project Name", NULL, row++, content);
   _edi_welcome_project_new_input_row_add("Project URL", NULL, row++, content);
   _edi_welcome_project_new_input_row_add("Username", getenv("USER"), row++, content);
   _edi_welcome_project_new_input_row_add("Email", NULL, row++, content);

   button = elm_button_add(content);
   elm_object_text_set(button, "Create");
   evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(button, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(button);
   elm_table_pack(content, button, _EDI_WELCOME_PROJECT_NEW_TABLE_WIDTH - 2, row, 2, 1);
   evas_object_smart_callback_add(button, "clicked", _edi_welcome_project_new_create_cb, NULL);

   item = elm_naviframe_item_push(naviframe,
                                "Create New Project",
                                NULL,
                                NULL,
                                content,
                                NULL);

   elm_naviframe_item_title_enabled_set(item, EINA_TRUE, EINA_TRUE);

}

static void
_edi_welcome_exit(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   evas_object_del(data);
   edi_close();
}

Evas_Object *edi_welcome_show()
{
   Evas_Object *win, *hbx, *box, *button, *label, *image, *naviframe;
   Elm_Object_Item *item;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("main", "Welcome to Edi");
   if (!win) return NULL;

   _welcome_window = win;
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   evas_object_smart_callback_add(win, "delete,request", _edi_welcome_exit, win);

   naviframe = elm_naviframe_add(win);
   evas_object_size_hint_weight_set(naviframe, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, naviframe);
   evas_object_show(naviframe);

   hbx = elm_box_add(naviframe);
   elm_box_horizontal_set(hbx, EINA_TRUE);
   evas_object_size_hint_weight_set(hbx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(hbx);

   /* Existing projects area */
   box = elm_box_add(hbx);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(hbx, box);
   evas_object_show(box);

   label = elm_label_add(box);
   elm_object_text_set(label, "Recent Projects:");
   evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, label);
   evas_object_show(label);

   button = elm_button_add(box);
   elm_object_text_set(button, "Open Existing Project");
   evas_object_smart_callback_add(button, "clicked",
                                       _edi_welcome_project_choose_cb, NULL);
   elm_box_pack_end(box, button);
   evas_object_show(button);

   /* New project area */
   box = elm_box_add(hbx);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(hbx, box);
   evas_object_show(box);

   snprintf(buf, sizeof(buf), "%s/images/welcome.png", elm_app_data_dir_get());
   image = elm_image_add(box);
   elm_image_file_set(image, buf, NULL);
   evas_object_size_hint_weight_set(image, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(image, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, image);
   evas_object_show(image);
   button = elm_button_add(box);
   elm_object_text_set(button, "Create New Project");
   evas_object_smart_callback_add(button, "clicked",
                                       _edi_welcome_project_new_cb, naviframe);
   elm_box_pack_end(box, button);
   evas_object_show(button);

   evas_object_resize(win, 320 * elm_config_scale_get(), 180 * elm_config_scale_get());
   evas_object_show(win);

   item = elm_naviframe_item_push(naviframe,
                                "Choose Project",
                                NULL,
                                NULL,
                                hbx,
                                NULL);

   elm_naviframe_item_title_enabled_set(item, EINA_FALSE, EINA_FALSE);

   return win;
}
