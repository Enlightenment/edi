#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* NOTE: Respecting header order is important for portability.
 * Always put system first, then EFL, then your public header,
 * and finally your private one. */

#include <Ecore_Getopt.h>
#include <Eio.h>

#include "gettext.h"

#include "Edi.h"

#include "edi_private.h"

#define COPYRIGHT "Copyright © 2014 Andy Williams <andy@andyilliams.me> and various contributors (see AUTHORS)."

static Eina_Bool
_exe_data(void *d EINA_UNUSED, int t EINA_UNUSED, void *event_info)
{
   Ecore_Exe_Event_Data *ev;
   Ecore_Exe_Event_Data_Line *el;

   ev = event_info;
   for (el = ev->lines; el && el->line; el++)
     fprintf(stdout, "%s\n", el->line);

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_exe_del(void *d EINA_UNUSED, int t EINA_UNUSED, void *event_info EINA_UNUSED)
{
   ecore_main_loop_quit();

   return ECORE_CALLBACK_DONE;
}

static const Ecore_Getopt optdesc = {
  "edi_build",
  "%prog [options] [build|clean|create|test]",
  PACKAGE_VERSION,
  COPYRIGHT,
  "BSD with advertisement clause",
  "The EFL IDE builder",
  EINA_TRUE,
  {
    ECORE_GETOPT_LICENSE('L', "license"),
    ECORE_GETOPT_COPYRIGHT('C', "copyright"),
    ECORE_GETOPT_VERSION('V', "version"),
    ECORE_GETOPT_HELP('h', "help"),
    ECORE_GETOPT_SENTINEL
  }
};

EAPI_MAIN int
main(int argc EINA_UNUSED, char **argv EINA_UNUSED)
{
   int args;
   char path[PATH_MAX], *build_type = NULL;
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

   if (!ecore_init())
     goto exit;
   edi_init();

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

   getcwd(path, PATH_MAX);
   edi_project_set(path);

   if (!edi_builder_can_build())
     {
        fprintf(stderr, "Cowardly refusing to build unknown project type.\n");
        ecore_shutdown();
        goto exit;
     }

   if (args < argc)
     build_type = argv[args];
   if (!build_type)
     build_type = "build";

   ecore_event_handler_add(ECORE_EXE_EVENT_DATA, _exe_data, NULL);
   ecore_event_handler_add(ECORE_EXE_EVENT_ERROR, _exe_data, NULL);
   ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _exe_del, NULL); 

   if (!strncmp("clean", build_type, 5))
     edi_builder_clean();
   else if (!strncmp("test", build_type, 4))
     edi_builder_test();
   else if (!strncmp("build", build_type, 5))
     edi_builder_build();
   else if (!strncmp("create", build_type, 6))
fprintf(stderr, "cannot yet make projects on command line");
//     edi_create_efl_project(...);
   else
     {
        fprintf(stderr, "Unrecognised build type - try build, clean, create or test.\n");
        goto end;
     }
   ecore_main_loop_begin();

   end:
   edi_shutdown();
   ecore_shutdown();
   return EXIT_SUCCESS;

   exit:
   return EXIT_FAILURE;
}

