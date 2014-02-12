#ifdef HAVE_CONFIG
# include "config.h"
#endif

#include "Edi.h"

#include "edi_private.h"

static int _edi_init = 0;
int _edi_lib_log_dom = -1;

EAPI int
edi_init(void)
{
   _edi_init++;
   if (_edi_init > 1) return _edi_init;

   eina_init();

   _edi_lib_log_dom = eina_log_domain_register("edi", EINA_COLOR_CYAN);
   if (_edi_lib_log_dom < 0)
     {
	EINA_LOG_ERR("Edi can not create its log domain.");
	goto shutdown_eina;
     }

   // Put here your initialization logic of your library

   eina_log_timing(_edi_lib_log_dom, EINA_LOG_STATE_STOP, EINA_LOG_STATE_INIT);

   return _edi_init;

 shutdown_eina:
   eina_shutdown();
   _edi_init--;

   return _edi_init;
}

EAPI int
edi_shutdown(void)
{
   _edi_init--;
   if (_edi_init != 0) return _edi_init;

   eina_log_timing(_edi_lib_log_dom,
		   EINA_LOG_STATE_START,
		   EINA_LOG_STATE_SHUTDOWN);

   // Put here your shutdown logic

   eina_log_domain_unregister(_edi_lib_log_dom);
   _edi_lib_log_dom = -1;

   eina_shutdown();

   return _edi_init;
}

EAPI void
edi_library_call(void)
{
   INF("Not really doing anything useful.");
}
