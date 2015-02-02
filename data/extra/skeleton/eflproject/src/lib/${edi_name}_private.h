#ifndef ${EDI_NAME}_PRIVATE_H
# define ${EDI_NAME}_PRIVATE_H

extern int _${edi_name}_lib_log_dom;

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_${edi_name}_lib_log_dom, __VA_ARGS__)
#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_${edi_name}_lib_log_dom, __VA_ARGS__)
#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_${edi_name}_lib_log_dom, __VA_ARGS__)
#ifdef CRIT
# undef CRIT
#endif
#define CRIT(...) EINA_LOG_DOM_CRIT(_${edi_name}_lib_log_dom, __VA_ARGS__)
#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_${edi_name}_lib_log_dom, __VA_ARGS__)

#endif
