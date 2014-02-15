#ifndef EDI_MAINVIEW_H_
# define EDI_MAINVIEW_H_

#include <Elementary.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for managing the main area of the Edi interface.
 */
 
/**
 * @brief UI management functions.
 * @defgroup UI
 *
 * @{
 *
 * Initialisation and management of the main view UI
 *
 */

/**
 * Initialize a new Edi main view and add it to the parent panel.
 *
 * @param win The window into which the panel will be loaded.
 *
 * @ingroup UI
 */
EAPI void edi_mainview_add(Evas_Object *parent);

/**
 * @}
 *
 * 
 * @brief Content management functions.
 * @defgroup Content
 *
 * @{
 *
 * Opening and managing content within the view.
 *
 */


EAPI void edi_mainview_open(const char *path);

/**
 * @}
 *
 */

#ifdef __cplusplus
}
#endif

#endif /* EDI_MAINVIEW_H_ */
