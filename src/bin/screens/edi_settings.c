#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Elementary.h>
#include <Ecore.h>

#include "Edi.h"
#include "edi_screens.h"
#include "edi_config.h"
#include "edi_debug.h"

#include "edi_private.h"

static Elm_Object_Item *_edi_settings_display, *_edi_settings_builds,
                       *_edi_settings_behaviour, *_edi_settings_project;

static void
_edi_settings_exit(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   evas_object_del(data);
}

static void
_edi_settings_category_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Elm_Object_Item *item;

   item = (Elm_Object_Item *)data;
   elm_naviframe_item_promote(item);
}

static Evas_Object *
_edi_settings_panel_create(Evas_Object *parent, const char *title)
{
   Evas_Object *box, *frame;

   box = elm_box_add(parent);
   elm_box_horizontal_set(box, EINA_FALSE);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);

   frame = elm_frame_add(parent);
   elm_object_text_set(frame, title);
   elm_object_part_content_set(frame, "default", box);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);

   return frame;
}

static void
_edi_settings_display_whitespace_cb(void *data EINA_UNUSED, Evas_Object *obj,
                                    void *event EINA_UNUSED)
{
   Evas_Object *check;

   check = (Evas_Object *)obj;
   _edi_project_config->gui.show_whitespace = elm_check_state_get(check);
   _edi_project_config_save();
}

static void
_edi_settings_display_tab_inserts_spaces_cb(void *data EINA_UNUSED, Evas_Object *obj,
                                            void *event EINA_UNUSED)
{
   Evas_Object *check;

   check = (Evas_Object *)obj;
   _edi_project_config->gui.tab_inserts_spaces = elm_check_state_get(check);
   _edi_project_config_save();
}

static void
_edi_settings_display_widthmarker_cb(void *data EINA_UNUSED, Evas_Object *obj,
                                     void *event EINA_UNUSED)
{
   Evas_Object *spinner;

   spinner = (Evas_Object *)obj;
   _edi_project_config->gui.width_marker = (int) elm_spinner_value_get(spinner);
   _edi_project_config_save();
}

static void
_edi_settings_display_tabstop_cb(void *data EINA_UNUSED, Evas_Object *obj,
                                     void *event EINA_UNUSED)
{
   Evas_Object *spinner;

   spinner = (Evas_Object *)obj;
   _edi_project_config->gui.tabstop = (int) elm_spinner_value_get(spinner);
   _edi_project_config_save();
}

static void
_edi_settings_toolbar_hidden_cb(void *data EINA_UNUSED, Evas_Object *obj,
                                void *event EINA_UNUSED)
{
   Evas_Object *check;

   check = (Evas_Object *)obj;
   _edi_project_config->gui.toolbar_hidden = elm_check_state_get(check);
   _edi_project_config_save();
}

static void
_edi_settings_font_choose_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *naviframe, *box;

   naviframe = (Evas_Object *)data;
   box = elm_box_add(naviframe);
   elm_box_horizontal_set(box, EINA_FALSE);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);

   edi_settings_font_add(box);
   elm_naviframe_item_push(naviframe, _("Font"), NULL, NULL, box, NULL);
}

static Evas_Object *
_edi_settings_font_preview_add(Evas_Object *parent, const char *font_name, int font_size)
{
   Elm_Code_Widget *widget;
   Elm_Code *code;

   code = elm_code_create();
   elm_code_file_line_append(code->file, FONT_PREVIEW, 35, NULL);

   widget = elm_code_widget_add(parent, code);
   elm_obj_code_widget_font_set(widget, font_name, font_size);
   elm_obj_code_widget_line_numbers_set(widget, EINA_TRUE);
   elm_obj_code_widget_editable_set(widget, EINA_FALSE);
   elm_obj_code_widget_policy_set(widget, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
   evas_object_size_hint_weight_set(widget, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget);

   return widget;
}

static Evas_Object *
_edi_settings_display_create(Evas_Object *parent)
{
   Evas_Object *box, *hbox, *frame, *label, *spinner, *check, *button, *preview;

   frame = _edi_settings_panel_create(parent, _("Display"));
   box = elm_object_part_content_get(frame, "default");

   hbox = elm_box_add(parent);
   elm_box_horizontal_set(hbox, EINA_TRUE);
   evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, 0.5);
   evas_object_size_hint_align_set(hbox, EVAS_HINT_FILL, 1.0);
   elm_box_pack_end(box, hbox);
   evas_object_show(hbox);

   label = elm_label_add(hbox);
   elm_object_text_set(label, _("Font"));
   evas_object_size_hint_align_set(label, EVAS_HINT_EXPAND, 0.5);
   elm_box_pack_end(hbox, label);
   evas_object_show(label);

   button = elm_button_add(hbox);
   evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(button, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(button);
   preview = _edi_settings_font_preview_add(hbox, _edi_project_config->font.name,
                                            _edi_project_config->font.size);
   elm_layout_content_set(button, "elm.swallow.content", preview);
   elm_box_pack_end(hbox, button);
   evas_object_smart_callback_add(button, "clicked",
                                  _edi_settings_font_choose_cb, parent);

   elm_object_focus_set(button, EINA_TRUE);

   check = elm_check_add(box);
   elm_object_text_set(check, _("Display whitespace"));
   elm_check_state_set(check, _edi_project_config->gui.show_whitespace);
   elm_box_pack_end(box, check);
   evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(check, 0.0, 0.5);
   evas_object_smart_callback_add(check, "changed",
                                  _edi_settings_display_whitespace_cb, NULL);
   evas_object_show(check);

   hbox = elm_box_add(box);
   elm_box_horizontal_set(hbox, EINA_TRUE);
   evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(hbox, EVAS_HINT_FILL, 0.5);
   elm_box_pack_end(box, hbox);
   evas_object_show(hbox);

   label = elm_label_add(hbox);
   elm_object_text_set(label, _("Line width marker"));
   evas_object_size_hint_align_set(label, EVAS_HINT_EXPAND, 0.5);
   elm_box_pack_end(hbox, label);
   evas_object_show(label);

   spinner = elm_spinner_add(hbox);
   elm_spinner_value_set(spinner, _edi_project_config->gui.width_marker);
   elm_spinner_editable_set(spinner, EINA_TRUE);
   elm_spinner_step_set(spinner, 1);
   elm_spinner_wrap_set(spinner, EINA_FALSE);
   elm_spinner_min_max_set(spinner, 0, 1024);
   evas_object_size_hint_weight_set(spinner, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(spinner, 0.0, 0.95);
   evas_object_smart_callback_add(spinner, "changed",
                                  _edi_settings_display_widthmarker_cb, NULL);
   elm_box_pack_end(hbox, spinner);
   evas_object_show(spinner);

   hbox = elm_box_add(parent);
   elm_box_horizontal_set(hbox, EINA_TRUE);
   evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(hbox, EVAS_HINT_FILL, 0.5);
   elm_box_pack_end(box, hbox);
   evas_object_show(hbox);

   label = elm_label_add(hbox);
   elm_object_text_set(label, _("Tabstop"));
   evas_object_size_hint_align_set(label, 0.0, 0.5);
   elm_box_pack_end(hbox, label);
   evas_object_show(label);

   spinner = elm_spinner_add(hbox);
   elm_spinner_value_set(spinner, _edi_project_config->gui.tabstop);
   elm_spinner_editable_set(spinner, EINA_TRUE);
   elm_spinner_step_set(spinner, 1);
   elm_spinner_wrap_set(spinner, EINA_FALSE);
   elm_spinner_min_max_set(spinner, 1, 32);
   evas_object_size_hint_weight_set(spinner, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(spinner, 0.0, 0.95);
   evas_object_smart_callback_add(spinner, "changed",
                                  _edi_settings_display_tabstop_cb, NULL);
   elm_box_pack_end(hbox, spinner);
   evas_object_show(spinner);

   check = elm_check_add(box);
   elm_object_text_set(check, _("Insert spaces when tab is pressed"));
   elm_check_state_set(check, _edi_project_config->gui.tab_inserts_spaces);
   elm_box_pack_end(box, check);
   evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(check, 0.0, 0.5);
   evas_object_smart_callback_add(check, "changed",
                                  _edi_settings_display_tab_inserts_spaces_cb, NULL);
   evas_object_show(check);

   check = elm_check_add(box);
   elm_object_text_set(check, _("Hide Toolbar"));
   elm_check_state_set(check, _edi_project_config->gui.toolbar_hidden);
   elm_box_pack_end(box, check);
   evas_object_size_hint_weight_set(check, EVAS_HINT_EXPAND, 0.5);
   evas_object_size_hint_align_set(check, EVAS_HINT_FILL, 0.0);
   evas_object_smart_callback_add(check, "changed",
                                  _edi_settings_toolbar_hidden_cb, NULL);
   evas_object_show(check);

   return frame;
}

static void
_edi_settings_builds_binary_chosen_cb(void *data, Evas_Object *obj EINA_UNUSED,
                                      void *event_info)
{
   Evas_Object *label = data;
   const char *file = event_info;

   if (!file)
     return;

   if (_edi_project_config->launch.path)
     eina_stringshare_del(_edi_project_config->launch.path);

   elm_object_text_set(label, file);
   _edi_project_config->launch.path = eina_stringshare_add(file);
   _edi_project_config_save();
}

static void
_edi_settings_builds_args_cb(void *data EINA_UNUSED, Evas_Object *obj,
                             void *event EINA_UNUSED)
{
   Evas_Object *entry;

   entry = (Evas_Object *)obj;

   if (_edi_project_config->launch.args)
     eina_stringshare_del(_edi_project_config->launch.args);

   _edi_project_config->launch.args = eina_stringshare_add(elm_object_text_get(entry));
   _edi_project_config_save();
}

static char *
_edi_settings_builds_debug_tool_text_get_cb(void *data, Evas_Object *obj EINA_UNUSED, const char *part EINA_UNUSED)
{
   Edi_Debug_Tool *tool;
   int i;

   i = (int)(uintptr_t) data;

   tool = &edi_debug_tools_get()[i];

   return strdup(tool->name);
}

static void _edi_settings_builds_debug_pressed_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   const char *text = elm_object_item_text_get(event_info);

   if (_edi_project_config->debug_command)
     eina_stringshare_del(_edi_project_config->debug_command);

   _edi_project_config->debug_command = eina_stringshare_add(text);
   _edi_project_config_save();

   elm_object_text_set(obj, text);
   elm_combobox_hover_end(obj);
}

static Evas_Object *
_edi_settings_builds_create(Evas_Object *parent)
{
   Evas_Object *box, *frame, *table, *label, *ic, *selector, *file, *entry;
   Evas_Object *combobox;
   Elm_Genlist_Item_Class *itc;
   Edi_Debug_Tool *tools;
   int i;

   frame = _edi_settings_panel_create(parent, _("Builds"));
   box = elm_object_part_content_get(frame, "default");

   table = elm_table_add(parent);
   elm_table_padding_set(table, 5, 5);
   evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(table, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(table);
   elm_box_pack_end(box, table);

   label = elm_label_add(box);
   elm_object_text_set(label, _("Runtime binary:"));
   evas_object_size_hint_weight_set(label, 0.0, 0.0);
   evas_object_size_hint_align_set(label, 0.0, EVAS_HINT_FILL);
   elm_table_pack(table, label, 0, 0, 1, 1);
   evas_object_show(label);

   ic = elm_icon_add(box);
   elm_icon_standard_set(ic, "file");
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   evas_object_show(ic);

   selector = elm_fileselector_button_add(box);
   elm_object_text_set(selector, _("Select"));
   elm_object_part_content_set(selector, "icon", ic);
   elm_fileselector_path_set(selector, edi_project_get());
   evas_object_size_hint_align_set(selector, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(table, selector, 1, 0, 1, 1);
   evas_object_show(selector);

   file = elm_entry_add(box);
   elm_entry_editable_set(file, EINA_FALSE);
   elm_entry_single_line_set(file, EINA_TRUE);
   elm_entry_scrollable_set(file, EINA_TRUE);
   elm_object_text_set(file, _edi_project_config->launch.path);
   evas_object_size_hint_weight_set(file, 0.75, 0.0);
   evas_object_size_hint_align_set(file, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(table, file, 2, 0, 1, 1);
   evas_object_show(file);

   evas_object_smart_callback_add(selector, "file,chosen",
                                  _edi_settings_builds_binary_chosen_cb, file);

   label = elm_label_add(box);
   elm_object_text_set(label, _("Runtime arguments:"));
   evas_object_size_hint_weight_set(label, 0.0, 0.0);
   evas_object_size_hint_align_set(label, 0.0, EVAS_HINT_FILL);
   elm_table_pack(table, label, 0, 1, 1, 1);
   evas_object_show(label);

   entry = elm_entry_add(box);
   elm_object_text_set(entry, _edi_project_config->launch.args);
   elm_entry_editable_set(entry, EINA_TRUE);
   elm_entry_single_line_set(entry, EINA_TRUE);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   evas_object_size_hint_weight_set(entry, 0.75, 0.0);
   evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(table, entry, 1, 1, 2, 1);
   evas_object_show(entry);
   evas_object_smart_callback_add(entry, "changed",
                                  _edi_settings_builds_args_cb, NULL);

   label = elm_label_add(box);
   elm_object_text_set(label, _("Default debugger:"));
   evas_object_size_hint_weight_set(label, 0.0, 0.0);
   evas_object_size_hint_align_set(label, 0.0, EVAS_HINT_FILL);
   elm_table_pack(table, label, 0, 2, 1, 1);
   evas_object_show(label);

   combobox = elm_combobox_add(box);
   if (_edi_project_config->debug_command)
     elm_object_part_text_set(combobox, "guide", _edi_project_config->debug_command);
   else
     elm_object_part_text_set(combobox, "guide", _("Please choose ..."));

   evas_object_size_hint_weight_set(combobox, 0.75, 0.0);
   evas_object_size_hint_align_set(combobox, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(combobox);
   evas_object_smart_callback_add(combobox, "item,pressed",
                                 _edi_settings_builds_debug_pressed_cb, NULL);

   elm_table_pack(table, combobox, 1, 2, 2, 1);

   itc = elm_genlist_item_class_new();
   itc->item_style = "default";
   itc->func.text_get = _edi_settings_builds_debug_tool_text_get_cb;

   tools = edi_debug_tools_get();
   for (i = 0; tools[i].name; i++)
     {
        if (ecore_file_app_installed(tools[i].exec))
          elm_genlist_item_append(combobox, itc, (void *)(uintptr_t) i, NULL, ELM_GENLIST_ITEM_NONE, NULL, (void *)(uintptr_t) i);
     }

   elm_genlist_realized_items_update(combobox);
   elm_genlist_item_class_free(itc);

   return frame;
}

static void
_edi_settings_project_remote_cb(void *data EINA_UNUSED, Evas_Object *obj,
                                void *event EINA_UNUSED)
{
   Evas_Object *entry;
   const char *url;

   entry = (Evas_Object *) obj;
   url = elm_object_text_get(entry);

   if (!url || strlen(url) == 0)
     return;

   if (!edi_scm_enabled() || edi_scm_remote_enabled())
     return;

   edi_scm_remote_add(elm_object_text_get(entry));
   elm_object_disabled_set(entry, EINA_TRUE);
}

static void
_edi_settings_project_email_cb(void *data EINA_UNUSED, Evas_Object *obj,
                             void *event EINA_UNUSED)
{
   Evas_Object *entry;

   entry = (Evas_Object *)obj;

   if (_edi_project_config->user_email)
     eina_stringshare_del(_edi_project_config->user_email);

   _edi_project_config->user_email = eina_stringshare_add(elm_object_text_get(entry));
   _edi_project_config_save();
}

static void
_edi_settings_project_name_cb(void *data EINA_UNUSED, Evas_Object *obj,
                             void *event EINA_UNUSED)
{
   Evas_Object *entry;

   entry = (Evas_Object *)obj;

   if (_edi_project_config->user_fullname)
     eina_stringshare_del(_edi_project_config->user_fullname);

   _edi_project_config->user_fullname = eina_stringshare_add(elm_object_text_get(entry));
   _edi_project_config_save();
}

static Evas_Object *
_edi_settings_project_create(Evas_Object *parent)
{
   Edi_Scm_Engine *engine = NULL;
   Evas_Object *box, *frames, *frame, *hbox, *label, *entry_name, *entry_email;
   Evas_Object *entry_remote;
   Eina_Strbuf *text;

   frames = elm_box_add(parent);
   frame = _edi_settings_panel_create(frames, _("Project Settings"));
   evas_object_size_hint_weight_set(frame, EVAS_HINT_EXPAND, 0.5);
   evas_object_size_hint_align_set(frame, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(frames, frame);
   evas_object_show(frame);
   box = elm_object_part_content_get(frame, "default");

   hbox = elm_box_add(parent);
   elm_box_horizontal_set(hbox, EINA_TRUE);
   evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(hbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, hbox);
   evas_object_show(hbox);

   label = elm_label_add(hbox);
   elm_object_text_set(label, _("Author Name"));
   evas_object_size_hint_weight_set(label, 0.0, 0.0);
   evas_object_size_hint_align_set(label, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(hbox, label);
   evas_object_show(label);

   entry_name = elm_entry_add(hbox);
   elm_object_text_set(entry_name, _edi_project_config->user_fullname);
   elm_entry_single_line_set(entry_name, EINA_TRUE);
   elm_entry_scrollable_set(entry_name, EINA_TRUE);
   evas_object_size_hint_weight_set(entry_name, 0.75, 0.0);
   evas_object_size_hint_align_set(entry_name, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(hbox, entry_name);
   evas_object_show(entry_name);
   evas_object_smart_callback_add(entry_name, "changed",
                                  _edi_settings_project_name_cb, NULL);

   hbox = elm_box_add(parent);
   elm_box_horizontal_set(hbox, EINA_TRUE);
   evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(hbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, hbox);
   evas_object_show(hbox);

   label = elm_label_add(hbox);
   elm_object_text_set(label, _("Author E-mail"));
   evas_object_size_hint_weight_set(label, 0.0, 0.0);
   evas_object_size_hint_align_set(label, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(hbox, label);
   evas_object_show(label);

   entry_email = elm_entry_add(hbox);
   elm_object_text_set(entry_email, _edi_project_config->user_email);
   elm_entry_single_line_set(entry_email, EINA_TRUE);
   elm_entry_scrollable_set(entry_email, EINA_TRUE);
   evas_object_size_hint_weight_set(entry_email, 0.75, 0.0);
   evas_object_size_hint_align_set(entry_email, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(hbox, entry_email);
   evas_object_show(entry_email);
   evas_object_smart_callback_add(entry_email, "changed",
                                  _edi_settings_project_email_cb, NULL);

   if (!edi_scm_enabled())
     return frames;

   engine = edi_scm_engine_get();
   text = eina_strbuf_new();
   eina_strbuf_append(text, _("Source Control"));
   eina_strbuf_append_printf(text, " (%s)", engine->name);

   frame = _edi_settings_panel_create(frames, eina_strbuf_string_get(text));
   eina_strbuf_free(text);
   evas_object_size_hint_weight_set(frame, EVAS_HINT_EXPAND, 0.5);
   evas_object_size_hint_align_set(frame, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(frames, frame);
   evas_object_show(frame);
   box = elm_object_part_content_get(frame, "default");

   hbox = elm_box_add(parent);
   elm_box_horizontal_set(hbox, EINA_TRUE);
   evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(hbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, hbox);
   evas_object_show(hbox);

   label = elm_label_add(hbox);
   elm_object_text_set(label, _("Remote URL"));
   evas_object_size_hint_weight_set(label, 0.0, 0.0);
   evas_object_size_hint_align_set(label, 0.0, EVAS_HINT_FILL);
   elm_box_pack_end(hbox, label);
   evas_object_show(label);

   entry_remote = elm_entry_add(hbox);
   elm_object_text_set(entry_remote, engine->remote_url_get());
   elm_entry_single_line_set(entry_remote, EINA_TRUE);
   elm_entry_scrollable_set(entry_remote, EINA_TRUE);
   elm_object_disabled_set(entry_remote, edi_scm_remote_enabled());
   evas_object_size_hint_weight_set(entry_remote, 0.75, 0.0);
   evas_object_size_hint_align_set(entry_remote, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(hbox, entry_remote);
   evas_object_show(entry_remote);
   evas_object_smart_callback_add(entry_remote, "changed",
                                  _edi_settings_project_remote_cb, NULL);

   return frames;
}

static void
_edi_settings_behaviour_autosave_cb(void *data EINA_UNUSED, Evas_Object *obj,
                                    void *event EINA_UNUSED)
{
   Evas_Object *check;

   check = (Evas_Object *)obj;
   _edi_config->autosave = elm_check_state_get(check);
   _edi_config_save();
}

static void
_edi_settings_behaviour_trim_whitespace_cb(void *data EINA_UNUSED, Evas_Object *obj,
                                           void *event EINA_UNUSED)
{
   Evas_Object *check;

   check = (Evas_Object *)obj;
   _edi_config->trim_whitespace = elm_check_state_get(check);
   _edi_config_save();
}

static Evas_Object *
_edi_settings_behaviour_create(Evas_Object *parent)
{
   Evas_Object *box, *frame, *check;

   frame = _edi_settings_panel_create(parent, _("Behaviour"));
   box = elm_object_part_content_get(frame, "default");

   check = elm_check_add(box);
   elm_object_text_set(check, _("Auto save files"));
   elm_check_state_set(check, _edi_config->autosave);
   elm_box_pack_end(box, check);
   evas_object_size_hint_align_set(check, EVAS_HINT_FILL, 0.5);
   evas_object_smart_callback_add(check, "changed",
                                  _edi_settings_behaviour_autosave_cb, NULL);
   evas_object_show(check);

   elm_object_focus_set(check, EINA_TRUE);

   check = elm_check_add(box);
   elm_object_text_set(check, _("Trim trailing whitespace"));
   elm_check_state_set(check, _edi_config->trim_whitespace);
   elm_box_pack_end(box, check);
   evas_object_size_hint_align_set(check, EVAS_HINT_FILL, 0.5);
   evas_object_smart_callback_add(check, "changed",
                                  _edi_settings_behaviour_trim_whitespace_cb, NULL);
   evas_object_show(check);

   return frame;
}

Evas_Object *
edi_settings_show(Evas_Object *mainwin)
{
   Evas_Object *win, *bg, *table, *naviframe, *tb;
   Elm_Object_Item *tb_it, *default_it;

   win = elm_win_add(mainwin, "settings", ELM_WIN_BASIC);
   if (!win) return NULL;

   elm_win_title_set(win, _("Edi Settings"));
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   evas_object_smart_callback_add(win, "delete,request", _edi_settings_exit, win);

   bg = elm_bg_add(win);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bg, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   table = elm_table_add(bg);
   evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(table, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_win_resize_object_add(win, table);
   evas_object_show(table);

   tb = elm_toolbar_add(table);
   elm_toolbar_homogeneous_set(tb, EINA_FALSE);
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_SCROLL);
   elm_toolbar_select_mode_set(tb, ELM_OBJECT_SELECT_MODE_ALWAYS);
   elm_toolbar_align_set(tb, 0.0);
   elm_toolbar_horizontal_set(tb, EINA_FALSE);
   evas_object_size_hint_weight_set(tb, 0.0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(tb, 0.0, EVAS_HINT_FILL);
   elm_table_pack(table, tb, 0, 0, 1, 5);
   evas_object_show(tb);

   naviframe = elm_naviframe_add(table);
   evas_object_size_hint_weight_set(naviframe, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(naviframe, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(table, naviframe, 1, 0, 4, 5);

   _edi_settings_project = elm_naviframe_item_push(naviframe, "", NULL, NULL,
                                                  _edi_settings_project_create(naviframe), NULL);
   elm_naviframe_item_title_enabled_set(_edi_settings_project, EINA_FALSE, EINA_FALSE);
   _edi_settings_display = elm_naviframe_item_push(naviframe, "", NULL, NULL,
                                                   _edi_settings_display_create(naviframe), NULL);
   elm_naviframe_item_title_enabled_set(_edi_settings_display, EINA_FALSE, EINA_FALSE);
   _edi_settings_builds = elm_naviframe_item_push(naviframe, "", NULL, NULL,
                                                   _edi_settings_builds_create(naviframe), NULL);
   elm_naviframe_item_title_enabled_set(_edi_settings_builds, EINA_FALSE, EINA_FALSE);
   _edi_settings_behaviour = elm_naviframe_item_push(naviframe, "", NULL, NULL,
                                                   _edi_settings_behaviour_create(naviframe), NULL);
   elm_naviframe_item_title_enabled_set(_edi_settings_behaviour, EINA_FALSE, EINA_FALSE);


   elm_toolbar_item_append(tb, "applications-development", _("Project"),_edi_settings_category_cb, _edi_settings_project);
   default_it = elm_toolbar_item_append(tb, "preferences-desktop", _("Display"),
                                        _edi_settings_category_cb, _edi_settings_display);
   elm_toolbar_item_append(tb, "system-run", "Builds",
                           _edi_settings_category_cb, _edi_settings_builds);

   tb_it = elm_toolbar_item_append(tb, NULL, NULL, NULL, NULL);
   elm_toolbar_item_separator_set(tb_it, EINA_TRUE);
   elm_toolbar_item_append(tb, "application-internet", _("Global"), NULL, NULL);
   elm_toolbar_item_append(tb, "preferences-other", _("Behaviour"),
                           _edi_settings_category_cb, _edi_settings_behaviour);
   elm_toolbar_item_selected_set(default_it, EINA_TRUE);

   evas_object_show(naviframe);
   evas_object_resize(win, 480 * elm_config_scale_get(), 320 * elm_config_scale_get());
   evas_object_show(win);

   return win;
}
