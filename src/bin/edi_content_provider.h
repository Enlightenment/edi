#ifndef EDI_CONTENT_PROVIDER_H_
# define EDI_CONTENT_PROVIDER_H_

#include <Evas.h>

#include "mainview/edi_mainview_item.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for managing information about supported content.
 */

typedef struct _Edi_Content_Provider
{
   const char *id;
   const char *icon;

   Eina_Bool is_text;
   Eina_Bool is_editable;

   Evas_Object *(*content_ui_add)(Evas_Object *parent, Edi_Mainview_Item *item);
} Edi_Content_Provider;

/**
 * @brief Lookup information in content provider registry.
 * @defgroup Lookup
 *
 * @{
 *
 * Looking up content providers based on mime or id etc.
 *
 */

/**
 * Look up a content provider based on a mime type.
 *
 * @param mime the mime type of a file you wish to get a content provider for
 *
 * @return an Edi_Content_Provider if one is registered or NULL otherwise
 *
 * @ingroup Lookup
 */
Edi_Content_Provider *edi_content_provider_for_mime_get(const char *mime);

/**
 * Look up a content provider based on a provider id.
 * This is useful for overriding mime-type based lookup.
 *
 * @param mime the id of a provider you wish to get a handle for.
 *
 * @return an Edi_Content_Provider if one is registered or NULL otherwise
 *
 * @ingroup Lookup
 */
Edi_Content_Provider *edi_content_provider_for_id_get(const char *id);

/**
 * @}
 */



#ifdef __cplusplus
}
#endif

#endif /* EDI_CONTENT_PROVIDER_H_ */
