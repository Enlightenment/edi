#ifndef EDI_WELCOME_H_
# define EDI_WELCOME_H_

#include <Elementary.h>

#include "Edi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines control the intro wizard for Edi.
 */

/**
 * @brief UI management functions.
 * @defgroup UI Initialisation and management of the welcome UI
 *
 * @{
 *
 */

/**
 * Initialize a new Edi welcome window and display it.
 *
 * @return The welcome window that is created
 * * @ingroup UI
 */
EAPI Evas_Object *edi_welcome_show();

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* EDI_CONSOLEPANEL_H_ */
