#ifndef EDI_DEBUGPANEL_H_
# define EDI_DEBUGPANEL_H_

#include <Elementary.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for managing the Edi debug panel.
 */

/**
 * @brief UI management functions.
 * @defgroup UI
 *
 * @{
 *
 * Initialisation and management of the debugpanel UI
 *
 */

/**
 * Initialise a new Edi debugpanel and add it to the parent panel.
 *
 * @param parent The panel into which the panel will be loaded.
 *
 * @ingroup UI
 */
void edi_debugpanel_add(Evas_Object *parent);

/**
 * Start a new debugging session 
 *
 * @param The toolname to do debugging with.
 *
 * @ingroup UI
 */
void edi_debugpanel_start(const char *toolname);

/**
 * Stop existing debugging session
 *
 * @ingroup UI
 */
void edi_debugpanel_stop();

/**
 * Perform a poll to check the state of any debug session
 *
 * @ingroup UI
 */
void edi_debugpanel_active_check(void);


/**
 * @}
 */



#ifdef __cplusplus
}
#endif

#endif /* EDI_DEBUGPANEL_H_ */
