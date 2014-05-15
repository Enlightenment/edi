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
 * @brief Editor.
 * @defgroup Editor The main text editor functions
 *
 * @{
 *
 */

/**
 * Initialize a new Edi editor and add it to the parent panel.
 *
 * @param parent The panel into which the panel will be loaded.
 * @param path The file path to be loaded in the editor.
 * @return the created evas object that contains the editor.
 *
 * @ingroup Editor
 */
EAPI Evas_Object *edi_editor_add(Evas_Object *parent, const char *path);

/**
 * @}
 *
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
