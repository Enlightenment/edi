#ifndef EDI_PRIVATE_H_
# define EDI_PRIVATE_H_

#include <Eina.h>
#include <Elementary.h>

#ifdef HAVE_PO
# include <locale.h>
#endif

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(x) gettext(x)
#else
# define _(x) (x)
#endif

extern int _edi_log_dom;

#ifdef CRIT
# undef CRIT
#endif
#define CRIT(...) EINA_LOG_DOM_CRIT(_edi_log_dom, __VA_ARGS__)
#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_edi_log_dom, __VA_ARGS__)
#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_edi_log_dom, __VA_ARGS__)
#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_edi_log_dom, __VA_ARGS__)
#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_edi_log_dom, __VA_ARGS__)

extern int EDI_EVENT_TAB_CHANGED;
extern int EDI_EVENT_FILE_CHANGED;
extern int EDI_EVENT_FILE_SAVED;

#define EDI_CONTENT_SAVE_TIMEOUT 1

#define FONT_PREVIEW " Evas *dostuff(void) {...}"

Eina_Bool edi_open(const char *path);

Evas_Object *edi_main_win_get(void);

void edi_close();

void edi_open_url();

Eina_Bool edi_noproject();

void edi_launcher_config_missing();
void edi_debug_exe_missing(void);


Eina_Bool _edi_project_config_save_no_notify(void);

#endif
