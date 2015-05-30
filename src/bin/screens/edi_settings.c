#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Elementary.h>
#include <Ecore.h>
#include <Elm_Code.h>

#include "edi_screens.h"
#include "edi_config.h"

#include "edi_private.h"

static Elm_Object_Item *_edi_settings_display, *_edi_settings_behaviour;

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
   elm_object_part_text_set(frame, "default", title);
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
   elm_naviframe_item_push(naviframe, "Font", NULL, NULL, box, NULL);
}

static Evas_Object *
_edi_settings_font_preview_add(Evas_Object *parent, const char *font_name, int font_size)
{
   Elm_Code_Widget *widget;
   Elm_Code *code;

   code = elm_code_create();
   elm_code_file_line_append(code->file, FONT_PREVIEW, 35, NULL);

   widget = eo_add(ELM_CODE_WIDGET_CLASS, parent,
                   elm_code_widget_code_set(code));
   eo_do(widget,
         elm_code_widget_font_set(font_name, font_size),
         elm_code_widget_line_numbers_set(EINA_TRUE),
         elm_code_widget_editable_set(EINA_FALSE));
   evas_object_size_hint_weight_set(widget, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget);

   return widget;
}

static Evas_Object *
_edi_settings_display_create(Evas_Object *parent)
{
   Evas_Object *box, *hbox, *frame, *label, *spinner, *check, *button, *preview;

   frame = _edi_settings_panel_create(parent, "Display");
   box = elm_object_part_content_get(frame, "default");

   hbox = elm_box_add(parent);
   elm_box_horizontal_set(hbox, EINA_TRUE);
   evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, 0.5);
   evas_object_size_hint_align_set(hbox, EVAS_HINT_FILL, 1.0);
   elm_box_pack_end(box, hbox);
   evas_object_show(hbox);

   label = elm_label_add(hbox);
   elm_object_text_set(label, "Font");
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

   check = elm_check_add(box);
   elm_object_text_set(check, "Display whitespace");
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
   elm_object_text_set(label, "Line width marker");
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
   elm_object_text_set(label, "Tabstop");
   evas_object_size_hint_align_set(label, EVAS_HINT_EXPAND, 0.5);
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
   elm_object_text_set(check, "Hide Toolbar");
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
_edi_settings_behaviour_autosave_cb(void *data EINA_UNUSED, Evas_Object *obj,
                                    void *event EINA_UNUSED)
{
   Evas_Object *check;

   check = (Evas_Object *)obj;
   _edi_config->autosave = elm_check_state_get(check);
   _edi_config_save();
}

static Evas_Object *
_edi_settings_behaviour_create(Evas_Object *parent)
{
   Evas_Object *box, *frame, *check;

   frame = _edi_settings_panel_create(parent, "Behaviour");
   box = elm_object_part_content_get(frame, "default");

   check = elm_check_add(box);
   elm_object_text_set(check, "Auto save files");
   elm_check_state_set(check, _edi_config->autosave);
   elm_box_pack_end(box, check);
   evas_object_size_hint_align_set(check, EVAS_HINT_FILL, 0.5);
   evas_object_smart_callback_add(check, "changed",
                                  _edi_settings_behaviour_autosave_cb, NULL);
   evas_object_show(check);

   return frame;
}

Evas_Object *
edi_settings_show(Evas_Object *mainwin)
{
   Evas_Object *win, *bg, *table, *naviframe, *tb;
   Elm_Object_Item *tb_it, *default_it;

   win = elm_win_add(mainwin, "settings", ELM_WIN_DIALOG_BASIC);
   if (!win) return NULL;

   elm_win_title_set(win, "Edi Settings");
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
   elm_toolbar_icon_order_lookup_set(tb, ELM_ICON_LOOKUP_FDO_THEME);
   evas_object_size_hint_weight_set(tb, 0.0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(tb, 0.0, EVAS_HINT_FILL);
   elm_table_pack(table, tb, 0, 0, 1, 5);
   evas_object_show(tb);

   naviframe = elm_naviframe_add(table);
   evas_object_size_hint_weight_set(naviframe, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(naviframe, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(table, naviframe, 1, 0, 4, 5);

   _edi_settings_display = elm_naviframe_item_push(naviframe, "", NULL, NULL,
                                                   _edi_settings_display_create(naviframe), NULL);
   elm_naviframe_item_title_enabled_set(_edi_settings_display, EINA_FALSE, EINA_FALSE);
   _edi_settings_behaviour = elm_naviframe_item_push(naviframe, "", NULL, NULL,
                                                   _edi_settings_behaviour_create(naviframe), NULL);
   elm_naviframe_item_title_enabled_set(_edi_settings_behaviour, EINA_FALSE, EINA_FALSE);

   elm_toolbar_item_append(tb, NULL, "Project", NULL, NULL);
   default_it = elm_toolbar_item_append(tb, "preferences-desktop", "Display",
                                        _edi_settings_category_cb, _edi_settings_display);

   tb_it = elm_toolbar_item_append(tb, NULL, NULL, NULL, NULL);
   elm_toolbar_item_separator_set(tb_it, EINA_TRUE);
   elm_toolbar_item_append(tb, NULL, "Global", NULL, NULL);
   elm_toolbar_item_append(tb, "preferences-other", "Behaviour",
                           _edi_settings_category_cb, _edi_settings_behaviour);
   elm_toolbar_item_selected_set(default_it, EINA_TRUE);

   evas_object_show(naviframe);
   evas_object_resize(win, 480 * elm_config_scale_get(), 320 * elm_config_scale_get());
   evas_object_show(win);

   return win;
}
