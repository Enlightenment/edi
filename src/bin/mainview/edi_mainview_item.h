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

typedef struct _Edi_Mainview_Item_Tab {
   Evas_Object     *toolbar;
   Elm_Object_Item *button;
   Evas_Object     *drag_btn;
   char            *path;
} Edi_Mainview_Item_Tab;

/**
 * @struct _Edi_Mainview_Item
 * An item being displayed in the mainview.
 */
typedef struct _Edi_Mainview_Item
{
   const char *path; /**< The path of the file in this mainview item */

   Edi_Mainview_Item_Tab *tab;  /**< The tab object connected to this view */
   Elm_Object_Item       *view; /**< The naviframe item that contains the view for this item */
   Evas_Object           *win;  /**< The window for the item if it's displayed in a seperate window */

   const char *mimetype;   /**< The detected mime type for the item */
   const char *editortype; /**< The requested editor type for this item */

   /* Private */

   Evas_Object *container; /**< The visual container that the item will display within */
   Evas_Object *pos;       /**< The object pointing to the item's statusbar in the editor */
   Eina_Bool loaded;
} Edi_Mainview_Item;

Edi_Mainview_Item * edi_mainview_item_add(Edi_Path_Options *path, const char *mime,
                                          Elm_Object_Item *tab, Evas_Object *win);
                                          
#ifdef __cplusplus
}
#endif

#endif /* EDI_MAINVIEW_ITEM_H_ */
