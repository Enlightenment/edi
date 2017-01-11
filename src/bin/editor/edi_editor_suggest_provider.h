#ifndef EDI_EDITOR_SUGGEST_PROVIDER_H_
# define EDI_EDITOR_SUGGEST_PROVIDER_H_

#include "editor/edi_editor.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for managing content suggestion providers.
 * i.e. like autosuggest in visual studio.
 */

/**
 * @typedef Edi_Editor_Suggest_Item
 * A handle for passig a suggest item to the ui and back
 */
typedef struct _Edi_Editor_Suggest_Item
{
   const char *summary;
   const char *detail;
} Edi_Editor_Suggest_Item;

/**
 * @struct Edi_Editor_Suggest_Provider
 * A description of the requirements for a suggestion provider.
 * This handles the set up and teardown of a provider as well as the lookup and
 * description lookup functions
 */
typedef struct _Edi_Editor_Suggest_Provider
{
   const char *id;

   void (*add)(Edi_Editor *editor);
   void (*del)(Edi_Editor *editor);
   Eina_List *(*lookup)(Edi_Editor *editor, unsigned int row, unsigned int col);
} Edi_Editor_Suggest_Provider;

/**
 * @brief Lookup information in suggest provider registry.
 * @defgroup Lookup
 *
 * @{
 *
 * Look up a suggest provider based on the provided editor.
 *
 * @param editor the editor session for a file you wish to get a suggestion provider for
 *
 * @return an Edi_Editor_Suggest_Provider if one is registered or NULL otherwise
 *
 * @ingroup Lookup
 */
Edi_Editor_Suggest_Provider *edi_editor_suggest_provider_get(Edi_Editor *editor);

/**
 * Query whether a suggest provider is available for the spcified editor session.
 *
 * @param editor the editor session for a file you wish to get a suggestion provider for
 *
 * @ingroup Lookup
 */
Eina_Bool edi_editor_suggest_provider_has(Edi_Editor *editor);

/**
 * Free a suggest item.
 *
 * @param item the suggest item to free
 *
 * @ingroup Lookup
 */
void edi_editor_suggest_item_free(Edi_Editor_Suggest_Item *item);

/**
 * @}
 */



#ifdef __cplusplus
}
#endif

#endif /* EDI_EDITOR_SUGGEST_PROVIDER_H_ */
