#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Elementary.h>
#include <Ecore.h>
#include <Elm_Code.h>

#include "edi_config.h"

#include "edi_private.h"

#define FONT_STEP (1.0 / (EDI_FONT_MAX - EDI_FONT_MIN))

static Evas_Object *op_fontslider, *op_fontlist, *op_fsml, *op_fbig;

typedef struct _Font Font;

struct _Font
{
   Elm_Object_Item *item;
   const char *pretty_name;
   const char *full_name;

   Evas_Object *widget;
};

static Eina_List *fonts = NULL;
static Eina_Hash *fonthash = NULL;

static Evas_Object *
_edi_settings_font_preview_add(Evas_Object *parent, const char *font_name, int font_size)
{
   Elm_Code_Widget *widget;
   Elm_Code *code;

   code = elm_code_create();
   elm_code_file_line_append(code->file, FONT_PREVIEW, 35, NULL);

   widget = eo_add(ELM_CODE_WIDGET_CLASS, parent,
                   elm_code_widget_code_set(code));
   eo_do(widget,
         elm_code_widget_font_set(font_name, font_size),
         elm_code_widget_line_numbers_set(EINA_TRUE),
         elm_code_widget_editable_set(EINA_FALSE),
         elm_code_widget_policy_set(ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF));
   evas_object_size_hint_weight_set(widget, 0.5, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget);

   return widget;
}

static int
_parse_font_name(const char *full_name,
                 const char **name, const char **pretty_name)
{
   char buf[4096];
   size_t style_len = 0;
   size_t font_len = 0;
   char *style = NULL;
   char *s;
   unsigned int len;

   *pretty_name = NULL;
   *name = NULL;

   font_len = strlen(full_name);
   if (font_len >= sizeof(buf))
     return -1;
   style = strchr(full_name, ':');
   if (style)
     font_len = (size_t)(style - full_name);

   if (elm_code_text_strnpos(full_name, font_len, "Mono", 0) == ELM_CODE_TEXT_NOT_FOUND &&
       elm_code_text_strnpos(full_name, font_len, "mono", 0) == ELM_CODE_TEXT_NOT_FOUND)
      return -1;

   s = strchr(full_name, ',');
   if (s && style && s < style)
     font_len = s - full_name;

#define STYLE_STR ":style="
   if (style && strncmp(style, STYLE_STR, strlen(STYLE_STR)) == 0)
     {
        style += strlen(STYLE_STR);
        s = strchr(style, ',');
        style_len = (s == NULL) ? strlen(style) : (size_t)(s - style);
     }

   s = buf;
   memcpy(s, full_name, font_len);
   s += font_len;
   len = font_len;
   if (style)
     {
        memcpy(s, STYLE_STR, strlen(STYLE_STR));
        s += strlen(STYLE_STR);
        len += strlen(STYLE_STR);

        memcpy(s, style, style_len);
        s += style_len;
        len += style_len;
     }
     *s = '\0';
   *name = eina_stringshare_add_length(buf, len);
#undef STYLE_STR

   /* unescape the dashes */
   s = buf;
   len = 0;
   while ( (size_t)(s - buf) < sizeof(buf) &&
           font_len > 0 )
     {
        if (*full_name != '\\')
          {
             *s++ = *full_name;
          }
        full_name++;
        font_len--;
        len++;
     }
   /* copy style */
   if (style_len > 0 && ((sizeof(buf) - (s - buf)) > style_len + 3 ))
     {
        *s++ = ' ';
        *s++ = '(';
        memcpy(s, style, style_len);
        s += style_len;
        *s++ = ')';

        len += 3 + style_len;
     }
     *s = '\0';

   *pretty_name = eina_stringshare_add_length(buf, len);
   return 0;
}

static void
_cb_op_font_sel(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Font *f = data;

   if ((_edi_project_config->font.name) && (!strcmp(f->full_name, _edi_project_config->font.name)))
     return;

   if (_edi_project_config->font.name) eina_stringshare_del(_edi_project_config->font.name);
   _edi_project_config->font.name = eina_stringshare_add(f->full_name);

   _edi_project_config_save();
}

static void
_cb_op_fontsize_sel(void *data, Evas_Object *obj, void *event EINA_UNUSED)
{
   int size = elm_slider_value_get(obj) + 0.5;

   if (_edi_project_config->font.size == size) return;

   _edi_project_config->font.size = size;
   elm_genlist_realized_items_update(op_fontlist);
   _edi_project_config_save();
}

static int
_cb_op_font_sort(const void *d1, const void *d2)
{
   return strcasecmp(d1, d2);
}

static Evas_Object *
_cb_op_font_content_get(void *data, Evas_Object *obj, const char *part)
{
   Evas_Object *box, *label;
   Font *f = data;

   if (strcmp(part, "elm.swallow.content"))
     return NULL;

   box = elm_box_add(obj);
   elm_box_horizontal_set(box, EINA_TRUE);
   elm_box_homogeneous_set(box, EINA_TRUE);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);

   label = elm_label_add(obj);
   elm_object_text_set(label, f->pretty_name);
   evas_object_size_hint_weight_set(label, 0.5, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(label, 0.0, EVAS_HINT_FILL);
   evas_object_show(label);
   elm_box_pack_end(box, label);

   f->widget = _edi_settings_font_preview_add(box, f->full_name,
                                              _edi_project_config->font.size);
   elm_box_pack_end(box, f->widget);
   return box;
}

static char *
_cb_op_font_text_get(void *data, Evas_Object *obj EINA_UNUSED, const char *part EINA_UNUSED)
{
   Font *f = data;
   return strdup(f->pretty_name);
}

void
options_font_clear(void)
{
   Font *f;

   op_fontslider = NULL;
   op_fontlist = NULL;
   op_fsml = NULL;
   op_fbig = NULL;

   EINA_LIST_FREE(fonts, f)
     {
        eina_stringshare_del(f->full_name);
        eina_stringshare_del(f->pretty_name);
        free(f);
     }
   if (fonthash)
     {
        eina_hash_free(fonthash);
        fonthash = NULL;
     }
}

void
edi_settings_font_add(Evas_Object *opbox)
{
   Evas_Object *o, *bx, *fr, *bx0;
   char *fname;
   Eina_List *fontlist, *l;
   Font *f;
   Elm_Object_Item *it, *sel_it = NULL;
   Elm_Genlist_Item_Class *it_class;

   options_font_clear();

   fr = elm_frame_add(opbox);
   elm_object_text_set(fr, "Font");
   evas_object_size_hint_weight_set(fr, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(fr, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(opbox, fr);
   evas_object_show(fr);

   bx0 = o = elm_box_add(opbox);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_content_set(fr, o);
   evas_object_show(o);

   bx = o = elm_box_add(opbox);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, 0.5);
   elm_box_horizontal_set(o, EINA_TRUE);

   op_fsml = o = elm_label_add(opbox);
   elm_object_text_set(o, "<font_size=6>A</font_size>");
   elm_box_pack_end(bx, o);
   evas_object_show(o);

   op_fontslider = o = elm_slider_add(opbox);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.5);
   elm_slider_unit_format_set(o, "%1.0f");
   elm_slider_indicator_format_set(o, "%1.0f");
   elm_slider_min_max_set(o, EDI_FONT_MIN, EDI_FONT_MAX);
   elm_slider_step_set(o, FONT_STEP);
   elm_slider_value_set(o, _edi_project_config->font.size);
   elm_box_pack_end(bx, o);
   evas_object_show(o);

   evas_object_smart_callback_add(o, "delay,changed",
                                  _cb_op_fontsize_sel, NULL);

   op_fbig = o = elm_label_add(opbox);
   elm_object_text_set(o, "<font_size=24>A</font_size>");
   elm_box_pack_end(bx, o);
   evas_object_show(o);

   elm_box_pack_end(bx0, bx);
   evas_object_show(bx);

   it_class = elm_genlist_item_class_new();
   it_class->item_style = "full";
   it_class->func.text_get = _cb_op_font_text_get;
   it_class->func.content_get = _cb_op_font_content_get;

   op_fontlist = o = elm_genlist_add(opbox);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_genlist_mode_set(o, ELM_LIST_COMPRESS);
   elm_genlist_homogeneous_set(o, EINA_TRUE);

   fontlist = evas_font_available_list(evas_object_evas_get(opbox));
   fontlist = eina_list_sort(fontlist, eina_list_count(fontlist),
                             _cb_op_font_sort);
   fonthash = eina_hash_string_superfast_new(NULL);
   EINA_LIST_FOREACH(fontlist, l, fname)
     {
        if (!eina_hash_find(fonthash, fname))
          {
             f = calloc(1, sizeof(Font));
             if (_parse_font_name(fname, &f->full_name, &f->pretty_name) < 0)
               {
                  free(f);
                  continue;
               }
             eina_hash_add(fonthash, eina_stringshare_add(fname), f);
             fonts = eina_list_append(fonts, f);
             f->item = it = elm_genlist_item_append(o, it_class, f, NULL,
                                          ELM_GENLIST_ITEM_NONE,
                                          _cb_op_font_sel, f);
             if ((!sel_it) && (_edi_project_config->font.name))
               {
                  if (!strcmp(_edi_project_config->font.name, f->full_name))
                    sel_it = it;
               }
          }
     }

   if (fontlist)
     evas_font_available_list_free(evas_object_evas_get(opbox), fontlist);

   if (sel_it)
     {
        elm_genlist_item_selected_set(sel_it, EINA_TRUE);
        elm_genlist_item_show(sel_it, ELM_GENLIST_ITEM_SCROLLTO_TOP);
     }

   elm_genlist_item_class_free(it_class);

   elm_box_pack_end(bx0, o);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(o);
}
