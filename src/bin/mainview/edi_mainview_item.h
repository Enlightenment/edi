#ifndef EDI_MAINVIEW_ITEM_H_
# define EDI_MAINVIEW_ITEM_H_

#include <Elementary.h>
#include <Evas.h>

#include "Edi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for managing views within the main area of the Edi interface.
 */

/**
 * @struct _Edi_Mainview_Item
 * An item being displayed in the mainview.
 */
typedef struct _Edi_Mainview_Item
{
   const char *path; /**< The path of the file in this mainview item */

   Elm_Object_Item *tab; /**< The tab object connected to this view */
   Elm_Object_Item *view; /**< The naviframe item that contains the view for this item */
   Evas_Object *win; /**< The window for the item if it's displayed in a seperate window */

   const char *mimetype; /**< The detected mime type for the item */
   const char *editortype; /**< The requested editor type for this item */

   /* Private */

   /* Add new members here. */
} Edi_Mainview_Item;

#ifdef __cplusplus
}
#endif

#endif /* EDI_MAINVIEW_ITEM_H_ */
