# include "config.h"

#if ENABLE_NLS
# include <libintl.h>
#endif

#include <Ecore_Getopt.h>
#include <Elementary.h>

#include "${edi_name}_private.h"

#define COPYRIGHT "Copyright © ${Edi_Year} ${Edi_User} <${Edi_Email}> and various contributors (see AUTHORS)."

static void
_${edi_name}_win_del(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_exit();
}

static Evas_Object *
${edi_name}_win_setup(void)
{
   Evas_Object *win;
   Evas_Object *label;

   win = elm_win_util_standard_add("main", "${Edi_Name}");
   if (!win) return NULL;

   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   evas_object_smart_callback_add(win, "delete,request", _${edi_name}_win_del, NULL);

   label = elm_label_add(win);
   elm_object_text_set(label, " Hello World !");
   evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(label);

   elm_win_resize_object_add(win, label);
   evas_object_resize(win, 300 * elm_config_scale_get(),
                           200 * elm_config_scale_get());
   elm_win_center(win, EINA_TRUE, EINA_TRUE);

   evas_object_show(win);

   return win;
}

static const Ecore_Getopt optdesc = {
  "${edi_name}",
  "%prog [options]",
  PACKAGE_VERSION,
  COPYRIGHT,
  "3 clause BSD license",
  "An EFL ${edi_name} program",
  0,
  {
    ECORE_GETOPT_LICENSE('L', "license"),
    ECORE_GETOPT_COPYRIGHT('C', "copyright"),
    ECORE_GETOPT_VERSION('V', "version"),
    ECORE_GETOPT_HELP('h', "help"),
    ECORE_GETOPT_SENTINEL
  }
};

EAPI_MAIN int
elm_main(int argc EINA_UNUSED, char **argv EINA_UNUSED)
{
   Evas_Object *win;
   int args;
   Eina_Bool quit_option = EINA_FALSE;

   Ecore_Getopt_Value values[] = {
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_NONE
   };

#if ENABLE_NLS
   setlocale(LC_ALL, "");
   bindtextdomain(PACKAGE, LOCALEDIR);
   bind_textdomain_codeset(PACKAGE, "UTF-8");
   textdomain(PACKAGE);
#endif

   args = ecore_getopt_parse(&optdesc, values, argc, argv);
   if (args < 0)
     {
	EINA_LOG_CRIT("Could not parse arguments.");
	goto end;
     }
   else if (quit_option)
     {
	goto end;
     }

   elm_app_info_set(elm_main, "${edi_name}", "images/${edi_name}.png");

   if (!(win = ${edi_name}_win_setup()))
     goto end;

   elm_run();

 end:
   elm_shutdown();

   return 0;
}
ELM_MAIN()
