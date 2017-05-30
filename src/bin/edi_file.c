#include "Edi.h"
#include "edi_file.h"
#include "edi_private.h"

Eina_Bool
edi_file_path_hidden(const char *path)
{
   Edi_Build_Provider *provider;

   provider = edi_build_provider_for_project_get();
   if (provider && provider->file_hidden_is(path))
     return EINA_TRUE;

   if (ecore_file_file_get(path)[0] == '.')
     return EINA_TRUE;

   return EINA_FALSE;
}

