#include <Edi.h>

#include "edi_scm_ui.h"
#include "edi_private.h"

#define DEFAULT_WIDTH  480
#define DEFAULT_HEIGHT 240

static void
_win_del_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   evas_object_del(obj);
   ecore_main_loop_quit();
}

static void
_win_title_set(Evas_Object *win, Edi_Scm_Engine *engine)
{
   Eina_Strbuf *title;

   title = eina_strbuf_new();
   eina_strbuf_append_printf(title, _("Edi Source Control :: %s (%s)"),
                             basename((char *)engine->root_directory),
                             engine->name ?: _("unknown"));
   elm_win_title_set(win, eina_strbuf_string_get(title));
   eina_strbuf_free(title);
}

static Evas_Object *
_win_add(Edi_Scm_Engine *engine)
{
   Evas_Object *win, *icon;

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

   win = elm_win_util_standard_add("edi_scm", "edi_scm");
   icon = elm_icon_add(win);
   elm_icon_standard_set(icon, "edi");
   elm_win_icon_object_set(win, icon);

   evas_object_resize(win, DEFAULT_WIDTH * elm_config_scale_get(), DEFAULT_HEIGHT * elm_config_scale_get());
   evas_object_smart_callback_add(win, "delete,request", _win_del_cb, NULL);

   _win_title_set(win, engine);

   return win;
}

int main(int argc, char **argv)
{
   Evas_Object *win;
   Edi_Scm_Engine *engine;
   const char *arg, *root;

   ecore_init();
   elm_init(argc, argv);
   root = NULL;

   if (argc >= 2)
     {
        arg = argv[1];
        if (!strcmp("-h", arg) || !strcmp("--help", arg))
          {
             printf("Usage: edi_scm [directory]\n\n");
             printf("The Enlightened IDE Source Control\n\n");

             printf("Options:\n");
             printf("  -c, --commit\t\topen with the commit screen.\n");
             printf("  -h, --help\t\tshow this message.\n");
             return 0;
          }

        if (!strcmp("-c", arg) || !strcmp("--commit", arg))
          {
             if (argc >= 3)
               root = argv[2];
          }
        else
          {
             root = arg;
          }
     }

   if (root)
     {
        if (!ecore_file_is_dir(root))
          {
             fprintf(stderr, _("Root path must be a directory\n"));
             exit(1 << 0);
          }
        engine = edi_scm_init_path(realpath(root, NULL));
     }
   else
     {
        engine = edi_scm_init();
     }

   if (!engine)
     exit(1 << 2);

   win = _win_add(engine);
   edi_scm_ui_add(win);
   elm_win_center(win, EINA_TRUE, EINA_TRUE);
   evas_object_show(win);

   ecore_main_loop_begin();

   edi_scm_shutdown();
   ecore_shutdown();
   elm_shutdown();

   return EXIT_SUCCESS;
}
