#ifndef _UI_H
#define _UI_H

#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for interacting with the main Edi editor view.
 */

/**
 * @brief Dialogs.
 * @defgroup Dialogs Functions that open other dialogs
 *
 * @{
 *
 */

/**
 * Start a search in the specified editor.
 *
 * @param entry The elm_entry that we are searching for text within.
 *
 * @ingroup Dialogs
 */
EAPI void edi_editor_search(Evas_Object *entry);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
