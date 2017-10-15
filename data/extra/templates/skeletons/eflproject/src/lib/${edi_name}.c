#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "${Edi_Name}.h"

#include "${edi_name}_private.h"

static int _${edi_name}_init = 0;
int _${edi_name}_lib_log_dom = -1;

EAPI int
${edi_name}_init(void)
{
   _${edi_name}_init++;
   if (_${edi_name}_init > 1) return _${edi_name}_init;

   eina_init();

   _${edi_name}_lib_log_dom = eina_log_domain_register("${edi_name}", EINA_COLOR_CYAN);
   if (_${edi_name}_lib_log_dom < 0)
     {
	EINA_LOG_ERR("${Edi_Name} can not create its log domain.");
	goto shutdown_eina;
     }

   // Put here your initialization logic of your library

   eina_log_timing(_${edi_name}_lib_log_dom, EINA_LOG_STATE_STOP, EINA_LOG_STATE_INIT);

   return _${edi_name}_init;

 shutdown_eina:
   eina_shutdown();
   _${edi_name}_init--;

   return _${edi_name}_init;
}

EAPI int
${edi_name}_shutdown(void)
{
   _${edi_name}_init--;
   if (_${edi_name}_init != 0) return _${edi_name}_init;

   eina_log_timing(_${edi_name}_lib_log_dom,
		   EINA_LOG_STATE_START,
		   EINA_LOG_STATE_SHUTDOWN);

   // Put here your shutdown logic

   eina_log_domain_unregister(_${edi_name}_lib_log_dom);
   _${edi_name}_lib_log_dom = -1;

   eina_shutdown();

   return _${edi_name}_init;
}

EAPI void
${edi_name}_library_call(void)
{
   INF("Not really doing anything useful.");
}
