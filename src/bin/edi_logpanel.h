#ifndef EDI_LOGPANEL_H_
# define EDI_LOGPANEL_H_

#include <Elementary.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for managing the Edi file panel.
 */

/**
 * @brief UI management functions.
 * @defgroup UI
 *
 * @{
 *
 * Initialisation and management of the log panel UI
 *
 */

/**
 * Initialize a new Edi logpanel and add it to the parent panel.
 *
 * @param parent The panel into which the panel will be loaded.
 *
 * @ingroup UI
 */
void edi_logpanel_add(Evas_Object *parent);

/**
 * @}
 */



#ifdef __cplusplus
}
#endif

#endif /* EDI_LOGPANEL_H_ */
