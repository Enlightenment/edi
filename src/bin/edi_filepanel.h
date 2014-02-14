#ifndef EDI_H_
# define EDI_H_

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
 * Initialisation and management of the file panel UI
 *
 */

/**
 * Initialize a new Edi filepanel and add it to the parent panel.
 *
 * @param win The window into which the panel will be loaded.
 *
 * @ingroup UI
 */
void edi_filepanel_add(Evas_Object *parent);

/**
 * @}
 */



#ifdef __cplusplus
}
#endif

#endif /* EDI_H_ */
