#ifndef EDI_PRIVATE_H_
# define EDI_PRIVATE_H_

#include <Eina.h>
#include <Elementary.h>

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

#define EDI_CONTENT_SAVE_TIMEOUT 2

#define FONT_PREVIEW " Evas *dostuff(void) {...}"

Eina_Bool edi_open(const char *path);

void edi_close();

void edi_open_url();

Eina_Bool edi_noproject();

#endif
