#ifdef HAVE_CONFIG
# include "config.h"
#endif

#include <unistd.h>
#include <Ecore.h>

#include "Edi.h"

#include "edi_private.h"

EAPI Eina_Bool
edi_builder_can_build(void)
{
   Edi_Build_Provider *provider;

   provider = edi_build_provider_for_project_get();

   return !!provider;
}

EAPI void
edi_builder_build(void)
{
   Edi_Build_Provider *provider;

   provider = edi_build_provider_for_project_get();
   if (!provider)
     return;

   provider->build();
}

EAPI void
edi_builder_test(void)
{
   Edi_Build_Provider *provider;

   provider = edi_build_provider_for_project_get();
   if (!provider)
     return;

   provider->test();
}

EAPI void
edi_builder_clean(void)
{
      Edi_Build_Provider *provider;

   provider = edi_build_provider_for_project_get();
   if (!provider)
     return;

   provider->clean();
}


