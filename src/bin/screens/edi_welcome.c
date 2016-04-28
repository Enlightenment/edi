#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#include <pwd.h>

#include <Elementary.h>

#include "edi_screens.h"
#include "edi_config.h"

#include "edi_private.h"

#define _EDI_WELCOME_PROJECT_NEW_TABLE_WIDTH 4

static Evas_Object *_welcome_window;
static Evas_Object *_edi_new_popup;
static Evas_Object *_edi_welcome_list;
static Evas_Object *_edi_project_box;
static Evas_Object *_create_inputs[5];

static Evas_Object *_edi_create_button, *_edi_open_button;

static const char *_edi_message_path;

static void _edi_welcome_add_recent_projects(Evas_Object *);

static void
_edi_on_close_message(void *data,
                       Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   evas_object_del(data);
   evas_object_del(_edi_new_popup);
}

static void
_edi_on_delete_message(void *data,
                       Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   _edi_config_project_remove(_edi_message_path);

   evas_object_del(_edi_welcome_list);
   _edi_welcome_add_recent_projects(_edi_project_box);
   evas_object_del(data);
   evas_object_del(_edi_new_popup);
}

static void
_edi_message_open(const char *message, Eina_Bool deletable)
{
   Evas_Object *popup, *button;

   popup = elm_popup_add(_welcome_window);
   _edi_new_popup = popup;
   elm_object_part_text_set(popup, "title,text", message);

   button = elm_button_add(popup);
   elm_object_text_set(button, "Ok");
   elm_object_part_content_set(popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked",
                                  _edi_on_close_message, NULL);

   if (deletable)
     {
        button = elm_button_add(popup);
        elm_object_text_set(button, "Delete");
        elm_object_part_content_set(popup, "button2", button);
        evas_object_smart_callback_add(button, "clicked",
                                       _edi_on_delete_message, NULL);
     }

   evas_object_show(popup);
}

static void
_edi_welcome_project_open(const char *path, const unsigned int _edi_creating)
{
   edi_project_set(path);

   if ((edi_open(edi_project_get()) == NULL) && !_edi_creating)
     {
       _edi_message_path = path;
       _edi_message_open("Apparently that project directory doesn't exist", EINA_TRUE);
     }
   else
     evas_object_del(_welcome_window);
}

static void
_edi_welcome_project_chosen_cb(void *data,
                       Evas_Object *obj EINA_UNUSED,
                       void *event_info)
{
   evas_object_del(data);
   elm_object_disabled_set(_edi_open_button, EINA_FALSE);
   elm_object_disabled_set(_edi_create_button, EINA_FALSE);

   if (event_info)
     {
        _edi_welcome_project_open((const char*)event_info, EINA_FALSE);
     }
}

static void
_edi_welcome_choose_exit(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   evas_object_del(data);
   elm_object_disabled_set(_edi_open_button, EINA_FALSE);
   elm_object_disabled_set(_edi_create_button, EINA_FALSE);
}

static void
_edi_welcome_project_choose_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *fs;

   elm_object_disabled_set(_edi_open_button, EINA_TRUE);
   elm_object_disabled_set(_edi_create_button, EINA_TRUE);

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
   elm_fileselector_path_set(fs, eina_environment_home_get());
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
_edi_welcome_project_new_create_done_cb(const char *path, Eina_Bool success)
{
   if (!success)
     {
        ERR("Unable to create project at path %s", path);

        return;
     }

   _edi_welcome_project_open(path, EINA_TRUE);
}

static void
_edi_welcome_project_new_create_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   const char *path, *name, *user, *email, *url;

   path = elm_fileselector_path_get(_create_inputs[0]);
   name = elm_object_text_get(_create_inputs[1]);
   url = elm_object_text_get(_create_inputs[2]);
   user = elm_object_text_get(_create_inputs[3]);
   email = elm_object_text_get(_create_inputs[4]);

   edi_create_efl_project(path, name, url, user, email, _edi_welcome_project_new_create_done_cb);
}

static int
_edi_welcome_user_fullname_get(const char *username, char *fullname, size_t max)
{
   struct passwd *p;
   char *pos;
   unsigned int n;

   if (!username)
     return -1;

   errno = 0;
   p = getpwnam(username);
   if (p == NULL || max == 0)
     {
        if (errno == 0)
          return 0;
        else
          return -1;
     }

   pos = strchr(p->pw_gecos, ',');
   if (!pos)
     n = strlen(p->pw_gecos);
   else
     n = pos - p->pw_gecos;

   if (n == 0)
     return 0;
   if (n > max - 1)
     n = max - 1;

   memcpy(fullname, p->pw_gecos, n);
   fullname[n] = '\0';
   return 1;
}

static void
_edi_welcome_project_new_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *content, *button, *naviframe = data;
   Elm_Object_Item *item;
   int row = 0;
   char fullname[1024];
   char *username;

   content = elm_table_add(naviframe);
   elm_table_homogeneous_set(content, EINA_TRUE);
   evas_object_size_hint_weight_set(content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(content);

   username = getenv("USER");
   if (!username)
     username = getenv("USERNAME");
   _edi_welcome_project_new_directory_row_add("Parent Path", NULL, row++, content);
   _edi_welcome_project_new_input_row_add("Project Name", NULL, row++, content);
   _edi_welcome_project_new_input_row_add("Project URL", NULL, row++, content);
   if (_edi_welcome_user_fullname_get(username, fullname, 1024) > 0)
      _edi_welcome_project_new_input_row_add("Creator Name", fullname, row++, content);
   else
      _edi_welcome_project_new_input_row_add("Creator Name", username, row++, content);
   _edi_welcome_project_new_input_row_add("Creator Email", NULL, row++, content);

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

static void
_recent_project_mouse_down(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj,
                           void *event_info)
{
   Evas_Coord w;
   Evas_Event_Mouse_Down *ev;

   ev = event_info;
   evas_object_geometry_get(obj, NULL, NULL, &w, NULL);

   if (ev->output.x > w - 20)
     {
        _edi_config_project_remove((const char *)data);
        evas_object_del(_edi_welcome_list);
        _edi_welcome_add_recent_projects(_edi_project_box);
     }
   else
     _edi_welcome_project_open((const char *)data, EINA_FALSE);
}

static void
_edi_welcome_add_recent_projects(Evas_Object *box)
{
   Evas_Object *list, *label, *ic, *icon_button;
   Elm_Object_Item *item;
   Eina_List *listitem;
   Edi_Config_Project *project;
   char *display, *format;
   int displen;

   list = elm_list_add(box);
   _edi_welcome_list = list;
   evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_list_mode_set(list, ELM_LIST_LIMIT);

   EINA_LIST_FOREACH(_edi_config->projects, listitem, project)
     {
        format = "<align=right><color=#ffffff><b>%s:   </b></color></align>";
        displen = strlen(project->path) + strlen(format) - 1;
        display = malloc(sizeof(char) * displen);
        snprintf(display, displen, format, project->name);

        // Add an 'edit-delete' icon that can be clicked to remove a project directory
        icon_button = elm_button_add(box);
        ic = elm_icon_add(icon_button);
        elm_icon_standard_set(ic, "edit-delete");
        elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);

        label = elm_label_add(box);
        elm_object_text_set(label, display);
        evas_object_color_set(label, 255, 255, 255, 255);
        evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_show(label);

        item = elm_list_item_append(list, project->path, label, ic, NULL, project->path);
        evas_object_event_callback_add(elm_list_item_object_get(item), EVAS_CALLBACK_MOUSE_DOWN,
                                       _recent_project_mouse_down, project->path);

        free(display);
     }

   elm_box_pack_end(box, list);
   evas_object_show(list);
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
   evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, label);
   evas_object_show(label);

   _edi_project_box = box;
   _edi_welcome_add_recent_projects(box);

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
   _edi_open_button = button;
   elm_object_text_set(button, "Open Existing Project");
   evas_object_smart_callback_add(button, "clicked",
                                       _edi_welcome_project_choose_cb, NULL);
   elm_box_pack_end(box, button);
   evas_object_show(button);

   button = elm_button_add(box);
   _edi_create_button = button;
   elm_object_text_set(button, "Create New Project");
   evas_object_smart_callback_add(button, "clicked",
                                       _edi_welcome_project_new_cb, naviframe);
   elm_box_pack_end(box, button);
   evas_object_show(button);

   item = elm_naviframe_item_push(naviframe,
                                "Choose Project",
                                NULL,
                                NULL,
                                hbx,
                                NULL);

   elm_naviframe_item_title_enabled_set(item, EINA_FALSE, EINA_FALSE);
   evas_object_show(win);
   evas_object_resize(win, 480 * elm_config_scale_get(), 260 * elm_config_scale_get());

   return win;
}
