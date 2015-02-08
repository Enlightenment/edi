#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Elementary.h>
#include <Ecore.h>

#include "edi_private.h"

static void
_edi_about_exit(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   evas_object_del(data);
}

static void
_edi_about_url_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   const char *url, *format;
   char *cmd;

   url = (const char *)data;
   format = "xdg-open \"%s\"";

   cmd = malloc(sizeof(char) * (strlen(format) + strlen(url) - 1));
   sprintf(cmd, format, url);
   ecore_exe_run(cmd, NULL);

   free(cmd);
}

Evas_Object *edi_about_show()
{
   Evas_Object *win, *vbox, *box, *table, *bg;
   Evas_Object *text, *title, *authors, *buttonbox, *button, *space;
   int alpha, r, g, b;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("about", "About Edi");
   if (!win) return NULL;

   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   evas_object_smart_callback_add(win, "delete,request", _edi_about_exit, win);

   table = elm_table_add(win);
   evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(table, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_win_resize_object_add(win, table);
   evas_object_show(table);

   snprintf(buf, sizeof(buf), "%s/images/welcome.png", elm_app_data_dir_get());
   bg = elm_bg_add(win);
   elm_bg_option_set(bg, ELM_BG_OPTION_CENTER);
   elm_bg_file_set(bg, buf, NULL);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bg, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(table, bg, 0, 0, 1, 1);
   evas_object_show(bg);

   evas_object_color_get(bg, &r, &g, &b, &alpha);
   evas_color_argb_unpremul(alpha, &r, &g, &b);
   alpha = 64;

   evas_color_argb_premul(alpha, &r, &g, &b);
   evas_object_color_set(bg, r, g, b, alpha);

   vbox = elm_box_add(win);
   elm_box_padding_set(vbox, 25, 0);
   elm_box_horizontal_set(vbox, EINA_TRUE);
   evas_object_size_hint_weight_set(vbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(vbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(table, vbox, 0, 0, 1, 1);
   evas_object_show(vbox);

   elm_box_pack_end(vbox, elm_box_add(vbox));
   box = elm_box_add(win);
   elm_box_padding_set(box, 10, 0);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(vbox, box);
   evas_object_show(box);
   elm_box_pack_end(vbox, elm_box_add(vbox));

   text = elm_label_add(box);
   elm_object_text_set(text, "<br>EDI is an IDE designed to get people into coding for Linux.<br>" \
                             "It's based on the <b>EFL</b> and written completely natively<br>" \
                             "to provide a <i>reponsive</i> and <i>beautiful</i> UI.<br>");
   evas_object_size_hint_weight_set(text, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(text, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, text);
   evas_object_show(text);

   title = elm_entry_add(box);
   elm_object_text_set(title, "<hilight><align=center>EDI's lovely authors</align></hilight>");
   elm_entry_editable_set(title, EINA_FALSE);
   elm_object_focus_allow_set(title, EINA_FALSE);
   evas_object_size_hint_weight_set(title, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(title, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, title);
   evas_object_show(title);

   /* Authors screen */
   authors = elm_entry_add(box);
   elm_entry_file_set(authors, PACKAGE_DOC_DIR "/AUTHORS", ELM_TEXT_FORMAT_PLAIN_UTF8);
   elm_entry_editable_set(authors, EINA_FALSE);
   elm_object_focus_allow_set(authors, EINA_FALSE);
   evas_object_size_hint_weight_set(authors, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(authors, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_resize(authors, 320 * elm_config_scale_get(), 160 * elm_config_scale_get());
   elm_box_pack_end(box, authors);
   evas_object_show(authors);

   buttonbox = elm_box_add(box);
   elm_box_horizontal_set(buttonbox, EINA_TRUE);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, buttonbox);
   evas_object_show(buttonbox);

   button = elm_button_add(box);
   elm_object_text_set(button, "Visit Website");
   evas_object_smart_callback_add(button, "clicked", _edi_about_url_cb,
                                  "http://edi-ide.com");
   elm_box_pack_end(buttonbox, button);
   evas_object_show(button);

   space = elm_box_add(box);
   evas_object_size_hint_min_set(space, 20 * elm_config_scale_get(), 0.0);
   elm_box_pack_end(buttonbox, space);
   evas_object_show(space);

   button = elm_button_add(box);
   elm_object_text_set(button, "Report Issue");
   evas_object_smart_callback_add(button, "clicked", _edi_about_url_cb,
                                  "https://phab.enlightenment.org/maniphest/task/create/?projects=PHID-PROJ-geg2fyscqgjjxt3fider");
   elm_box_pack_end(buttonbox, button);
   evas_object_show(button);

   space = elm_box_add(box);
   evas_object_size_hint_min_set(space, 20 * elm_config_scale_get(), 0.0);
   elm_box_pack_end(buttonbox, space);
   evas_object_show(space);

   button = elm_button_add(box);
   elm_object_text_set(button, "Donate Now");
   evas_object_smart_callback_add(button, "clicked", _edi_about_url_cb,
                                  "https://www.bountysource.com/teams/edi-ide");
   elm_box_pack_end(buttonbox, button);
   evas_object_show(button);

   evas_object_resize(table, 360 * elm_config_scale_get(), 220 * elm_config_scale_get());
   evas_object_resize(win, 360 * elm_config_scale_get(), 220 * elm_config_scale_get());
   evas_object_show(win);

   return win;
}
