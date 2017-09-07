#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>

#include "edi_language_provider.h"

#include "edi_config.h"

#include "edi_private.h"

void
_edi_language_python_add(Edi_Editor *editor EINA_UNUSED)
{
}

void
_edi_language_python_refresh(Edi_Editor *editor EINA_UNUSED)
{
}

void
_edi_language_python_del(Edi_Editor *editor EINA_UNUSED)
{
}

const char *
_edi_language_python_mime_name(const char *mime)
{
   if (!strcasecmp(mime, "text/x-python"))
     return "Python source";

   return NULL;
}

const char *
_edi_language_python_snippet_get(const char *key EINA_UNUSED)
{
   return NULL;
}

