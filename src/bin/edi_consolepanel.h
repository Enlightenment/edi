#ifndef EDI_CONSOLEPANEL_H_
# define EDI_CONSOLEPANEL_H_

#include <Elementary.h>

#include "Edi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for managing the Edi console panel.
 */

/**
 * @brief UI management functions.
 * @defgroup UI Initialisation and management of the console panel UI
 *
 * @{
 *
 */

/**
 * Initialise a new Edi consolepanel and add it to the parent panel.
 *
 * @param parent The panel into which the panel will be loaded.
 *
 * @ingroup UI
 */
void edi_consolepanel_add(Evas_Object *parent);

/**
 * Show the Edi consolepanel - animating on to screen if required.
 *
 * @ingroup UI
 */
void edi_consolepanel_show();

/**
 * Initialise a new Edi testpanel and add it to the parent panel.
 *
 * @param parent The panel into which the panel will be loaded.
 *
 * @ingroup UI
 */
void edi_testpanel_add(Evas_Object *parent);

/**
 * Show the Edi testpanel - animating on to screen if required.
 *
 * @ingroup UI
 */
void edi_testpanel_show();

/**
 * @}
 */

/**
 * @brief Console management functions.
 * @defgroup Console Manipulation of console output in Edi
 *
 * @{
 *
 */

/**
 * Append a new line to the console.
 *
 * @param line The line of text to append to the console.
 *
 * @ingroup Console
 */
void edi_consolepanel_append_line(const char *line);

/**
 * Append a new error line to the console.
 *
 * @param line The line of text to append to the console.
 *
 * @ingroup Console
 */
void edi_consolepanel_append_error_line(const char *line);

/**
 * Clear all lines from the console.
 *
 * @ingroup Console
 */
void edi_consolepanel_clear();

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* EDI_CONSOLEPANEL_H_ */
