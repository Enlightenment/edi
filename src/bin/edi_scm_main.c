#include <Edi.h>
#include "edi_scm_ui.h"

#define DEFAULT_WIDTH  460
#define DEFAULT_HEIGHT 280

static void
_win_del_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   evas_object_del(obj);
   ecore_main_loop_quit();
}

static Evas_Object *
_setup_win(void)
{
   Evas_Object *win, *icon;
   Eina_Strbuf *title;
   char *cwd;

   cwd = getcwd(NULL, PATH_MAX);

   title = eina_strbuf_new();
   eina_strbuf_append_printf(title, "Edi Source Control :: %s", cwd);

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

   win = elm_win_util_standard_add(eina_strbuf_string_get(title), eina_strbuf_string_get(title));
   icon = elm_icon_add(win);
   elm_icon_standard_set(icon, "edi");
   elm_win_icon_object_set(win, icon);

   evas_object_resize(win, DEFAULT_WIDTH * elm_config_scale_get(), DEFAULT_HEIGHT * elm_config_scale_get());
   evas_object_smart_callback_add(win, "delete,request", _win_del_cb, NULL);

   eina_strbuf_free(title);
   free(cwd);

   return win;
}

int main(int argc, char **argv)
{
   Evas_Object *win;

   ecore_init();
   elm_init(argc, argv);

   win = _setup_win();
   edi_scm_ui_add(win);
   elm_win_center(win, EINA_TRUE, EINA_TRUE);
   evas_object_show(win);

   ecore_main_loop_begin();

   ecore_shutdown();
   elm_shutdown();

   return EXIT_SUCCESS;
}
