#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>

#include "edi_language_provider.h"

#include "edi_config.h"

#include "edi_private.h"

void
_edi_language_go_add(Edi_Editor *editor EINA_UNUSED)
{
}

void
_edi_language_go_refresh(Edi_Editor *editor EINA_UNUSED)
{
}

void
_edi_language_go_del(Edi_Editor *editor EINA_UNUSED)
{
}

const char *
_edi_language_go_mime_name(const char *mime)
{
   if (!strcasecmp(mime, "text/x-go"))
     return _("Go source");

   return NULL;
}

const char *
_edi_language_go_snippet_get(const char *key)
{
   (void) key;
   return NULL;
}

