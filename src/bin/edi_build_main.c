#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* NOTE: Respecting header order is important for portability.
 * Always put system first, then EFL, then your public header,
 * and finally your private one. */

#if ENABLE_NLS
# include <libintl.h>
#endif

#include <Ecore_Getopt.h>
#include <Eio.h>

#include "Edi.h"

#include "edi_private.h"

#define COPYRIGHT "Copyright © 2014 Andy Williams <andy@andyilliams.me> and various contributors (see AUTHORS)."
#define EXIT_NOACTION -2

static int _exit_code;

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

static void
_edi_build_create_done_cb(const char *path, Eina_Bool success)
{
   if (success)
     fprintf(stdout, "Project created at path %s\n", path);
   else
     {
        fprintf(stderr, "Unable to create project at path %s\n", path);
        _exit_code = EXIT_FAILURE;
     }

   ecore_main_loop_quit();
   elm_shutdown();
}

static void
_edi_build_print_start(Edi_Build_Provider *provider, const char *action)
{
   printf("Building \"%s\" target [%s] using [%s].\n", edi_project_name_get(), action, provider->id);
}

static void
_edi_build_print_noop(Edi_Build_Provider *provider, const char *action)
{
   printf("Target [%s] not supported for builder [%s].\n", action, provider->id);
}

static int
_edi_build_action_try(Edi_Build_Provider *provider, void (*action)(void), const char *name, const char *request)
{
   if (strncmp(name, request, strlen(name)))
     return EXIT_NOACTION;

   if (action)
     {
        _edi_build_print_start(provider, name);
        action();
     }
   else
     {
        _edi_build_print_noop(provider, name);
        return EXIT_FAILURE;
     }

   return EXIT_SUCCESS;
}

static void
_edi_build_create_start(int argc, int arg0, char **argv)
{
   elm_init(argc, argv);
   edi_create_efl_project(argv[arg0 + 1], argv[arg0 + 2], argv[arg0 + 3],
                          argv[arg0 + 4], argv[arg0 + 5], argv[arg0 + 6],
                          _edi_build_create_done_cb);
}

EAPI_MAIN int
main(int argc, char **argv)
{
   int args, ret;
   char path[PATH_MAX], *build_type = NULL;
   Eina_Bool quit_option = EINA_FALSE;
   Edi_Build_Provider *provider;

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

   _exit_code = EXIT_SUCCESS;
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

   if (args < argc)
     build_type = argv[args];
   if (!build_type)
     build_type = "build";

   if (!strncmp("create", build_type, 6))
     {
        if (argc - args != 7)
          {
             fprintf(stderr, "create requires 6 additional parameters:\n");
             fprintf(stderr, "  skeleton, parent_path, project_name, "
                             "project_url, creator_name, creator_email\n");
             goto end;
          }

        _edi_build_create_start(argc, args, argv);

        ecore_main_loop_begin();
        goto end;
     }

   if (!edi_builder_can_build())
     {
        fprintf(stderr, "Cowardly refusing to build unknown project type.\n");
        ecore_shutdown();
        goto exit;
     }

   provider = edi_build_provider_for_project_get();

   ecore_event_handler_add(ECORE_EXE_EVENT_DATA, _exe_data, NULL);
   ecore_event_handler_add(ECORE_EXE_EVENT_ERROR, _exe_data, NULL);
   ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _exe_del, NULL);

   if (
       ((ret = _edi_build_action_try(provider, provider->clean, "clean", build_type)) == EXIT_NOACTION) &&
       ((ret = _edi_build_action_try(provider, provider->test, "test", build_type)) == EXIT_NOACTION) &&
       ((ret = _edi_build_action_try(provider, provider->build, "build", build_type)) == EXIT_NOACTION))
     {
        fprintf(stderr, "Unrecognised build type - try build, clean, create or test.\n");
        goto end;
     }
   if (ret != EXIT_SUCCESS)
     return ret;
   ecore_main_loop_begin();

   end:
   edi_shutdown();
   ecore_shutdown();
   return _exit_code;

   exit:
   return EXIT_FAILURE;
}

