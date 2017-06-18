#ifndef EDI_SEARCHPANEL_H_
# define EDI_SEARCHPANEL_H_

#include <Elementary.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for managing the Edi search panel.
 */

/**
 * @brief UI management functions.
 * @defgroup UI
 *
 * @{
 *
 * Initialisation and management of the search panel UI
 *
 */


/**
 * Initialise a new Edi searchpanel and add it to the parent panel.
 *
 * @param parent The panel into which the panel will be loaded.
 *
 * @ingroup UI
 */
void edi_searchpanel_add(Evas_Object *parent);

/**
 * Show the Edi searchpanel - animating on to screen if required.
 *
 * @ingroup UI
 */
void edi_searchpanel_show();

/**
 * Search in project for text and print results to the panel.
 * 
 * @param text The search string to use when parsing project files.
 *
 * @ingroup UI
 */
void edi_searchpanel_find(const char *text);

/**
 * Initialise a new Edi taskspanel and add it to the parent pane.
 *
 * @ingroup UI
 */
void edi_taskspanel_add(Evas_Object *parent);

/**
 * Show the Edi taskspanel - animating on to the screen if require.
 *
 * @ingroup UI
 */
void edi_taskspanel_show();

/**
 * Find known labels in the text e.g. FIXME/TODO and print result to
 * the panel.
 *
 * @ingroup UI
 */
void edi_taskspanel_find();

/**
 * @}
 */



#ifdef __cplusplus
}
#endif

#endif /* EDI_LOGPANEL_H_ */
