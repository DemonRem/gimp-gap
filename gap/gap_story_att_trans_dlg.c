/*  gap_story_att_trans_dlg.c
 *
 *  This module handles GAP storyboard dialog transition attribute properties window
 */
/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* revision history:
 * version 2.2.1-214; 2006/03/31  hof: created
 */


#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gap_story_main.h"
#include "gap_story_dialog.h"
#include "gap_story_file.h"
#include "gap_story_att_trans_dlg.h"
#include "gap_pview_da.h"
#include "gap_stock.h"
#include "gap_lib.h"
#include "gap_vin.h"
#include "gap_timeconv.h"
#include "gap_layer_copy.h"


#include "gap-intl.h"


#define ATTW_COMMENT_WIDTH      480
#define CONVERT_TO_100PERCENT   100.0

#define GAP_STORY_ATTR_PROP_HELP_ID  "plug-in-gap-storyboard-attr-prop"
#define GAP_STORY_ATT_RESPONSE_RESET 1

#define PVIEW_SIZE 256
#define LAYERNAME_ORIG "orig_layer"
#define LAYERNAME_CURR "curr_layer"
#define LAYERNAME_DECO "deco_layer"
#define LAYERNAME_BASE "base_layer"

#define LAYERSTACK_TOP  -1
#define LAYERSTACK_CURR 1
#define LAYERSTACK_BASE 0

#define OBJ_DATA_KEY_ATTW     "attw"
#define OBJ_DATA_KEY_IMG_IDX  "img_idx"

#define PVIEW_TO_MASTER_SCALE  0.5

extern int gap_debug;  /* 1 == print debug infos , 0 dont print debug infos */

static void     p_attw_prop_response(GtkWidget *widget
                  , gint       response_id
                  , GapStbAttrWidget *attw
                  );
static void     p_attw_prop_reset_all(GapStbAttrWidget *attw);
static void     p_attw_timer_job(GapStbAttrWidget *attw);
static void     p_attw_update_properties(GapStbAttrWidget *attw);
static void     p_attw_update_sensitivity(GapStbAttrWidget *attw);
static gdouble  p_get_default_attribute(GapStbAttrWidget *attw
                     , GdkEventButton  *bevent
                     , gint att_type_idx
                     , gboolean form_value);

static void     p_attw_start_button_clicked_callback(GtkWidget *widget
                  , GdkEventButton  *bevent
                  , gint att_type_idx);
static void     p_attw_end_button_clicked_callback(GtkWidget *widget
                  , GdkEventButton  *bevent
                  , gint att_type_idx);
static void     p_attw_dur_button_clicked_callback(GtkWidget *widget
                  , GdkEventButton  *bevent
                  , gint att_type_idx);
static void     p_attw_gdouble_adjustment_callback(GtkObject *obj, gdouble *val);
static void     p_duration_dependent_refresh(GapStbAttrWidget *attw);
static void     p_attw_duration_adjustment_callback(GtkObject *obj, gint32 *val);
static void     p_attw_enable_toggle_update_callback(GtkWidget *widget, gboolean *val);

static void     p_attw_auto_update_toggle_update_callback(GtkWidget *widget, gboolean *val);

static gboolean p_attw_preview_events_cb (GtkWidget *widget
                       , GdkEvent  *event
                       , GapStbAttrWidget *attw);
static void     p_calculate_render_attributes(GapStbAttrWidget *attw
                       , gint img_idx
                       , GapStoryCalcAttr  *calc_attr
                       );
static void     p_check_and_make_orig_default_layer(GapStbAttrWidget *attw, gint img_idx);
static gint32   p_create_color_layer(GapStbAttrWidget *attw, gint32 image_id
                    , const char *name, gint stackposition, gdouble opacity
                    , gdouble red, gdouble green, gdouble blue);
static gint32   p_create_base_layer(GapStbAttrWidget *attw, gint32 image_id);
static gint32   p_create_deco_layer(GapStbAttrWidget *attw, gint32 image_id);
static void     p_create_gfx_image(GapStbAttrWidget *attw, gint img_idx);
static void     p_delete_gfx_images(GapStbAttrWidget *attw);
static void     p_render_gfx(GapStbAttrWidget *attw, gint img_idx);

static void     p_update_framenr_labels(GapStbAttrWidget *attw, gint img_idx, gint32 framenr);
static gint32   p_get_relevant_duration(GapStbAttrWidget *attw, gint img_idx);
static void     p_update_full_preview_gfx(GapStbAttrWidget *attw);

static gboolean p_stb_reg_equals_orig_layer(GapStbAttrWidget *attw
                         , gint   img_idx
                         , GapStoryLocateRet *stb_ret
                         , const char *filename);
static void     p_destroy_orig_layer(GapStbAttrWidget *attw
                         , gint   img_idx);
static gboolean p_check_orig_layer_up_to_date(GapStbAttrWidget *attw
                         , gint   img_idx
                         , GapStoryLocateRet *stb_ret
                         , const char *filename);
static void     p_create_or_replace_orig_layer (GapStbAttrWidget *attw
                         , gint   img_idx
                         , gint32 duration);
static gint32   p_fetch_video_frame_as_layer(GapStbMainGlobalParams *sgpp
                   , const char *video_filename
                   , gint32 framenumber
                   , gint32 seltrack
                   , const char *preferred_decoder
                   , gint32 image_id
                   );

static gint32   p_fetch_imagefile_as_layer (const char *img_filename
                   , gint32 image_id
                   );


static void     p_attw_comment_entry_update_cb(GtkWidget *widget, GapStbAttrWidget *attw);

static void     p_create_and_attach_pv_widgets(GapStbAttrWidget *attw
                 , GtkWidget *table
                 , gint row
                 , gint col_start
                 , gint col_end
                 , gint img_idx
                 );
static void     p_create_and_attach_att_arr_widgets(const char *row_title
                 , GapStbAttrWidget *attw
                 , GtkWidget *table
                 , gint row
                 , gint column
                 , gint att_type_idx
                 , gdouble lower_constraint
                 , gdouble upper_constraint
                 , gdouble step_increment
                 , gdouble page_increment
                 , gdouble page_size
                 , gint    digits
                 , const char *enable_tooltip
                 , gboolean   *att_arr_enable_ptr
                 , const char *from_tooltip
                 , gdouble    *att_arr_value_from_ptr
                 , const char *to_tooltip
                 , gdouble    *att_arr_value_to_ptr
                 , const char *dur_tooltip
                 , gint32     *att_arr_value_dur_ptr
                 );

/* ---------------------------------
 * p_attw_prop_response
 * ---------------------------------
 */
static void
p_attw_prop_response(GtkWidget *widget
                  , gint       response_id
                  , GapStbAttrWidget *attw
                  )
{
  GtkWidget *dlg;
  GapStbMainGlobalParams  *sgpp;

  sgpp = attw->sgpp;
  switch (response_id)
  {
    case GAP_STORY_ATT_RESPONSE_RESET:
      if((attw->stb_elem_bck)
      && (attw->stb_elem_refptr))
      {
        p_attw_prop_reset_all(attw);
      }
      break;
    case GTK_RESPONSE_CLOSE:
    default:
      dlg = attw->attw_prop_dialog;
      if(dlg)
      {
        p_delete_gfx_images(attw);
        if(attw->go_timertag >= 0)
        {
          g_source_remove(attw->go_timertag);
        }
        attw->go_timertag = -1;
        attw->attw_prop_dialog = NULL;
        gtk_widget_destroy(dlg);
      }
      break;
  }
}  /* end p_attw_prop_response */


/* ---------------------------------
 * p_attw_prop_reset_all
 * ---------------------------------
 */
static void
p_attw_prop_reset_all(GapStbAttrWidget *attw)
{
  gboolean comment_set;
  gint ii;

  if(attw)
  {
    if(attw->stb_elem_refptr)
    {
      attw->stb_refptr->unsaved_changes = TRUE;
      gap_story_elem_copy(attw->stb_elem_refptr, attw->stb_elem_bck);

      comment_set = FALSE;
      if(attw->stb_elem_refptr->comment)
      {
        if(attw->stb_elem_refptr->comment->orig_src_line)
        {
          gtk_entry_set_text(GTK_ENTRY(attw->comment_entry), attw->stb_elem_refptr->comment->orig_src_line);
          comment_set = TRUE;
        }
      }
      if(!comment_set)
      {
          gtk_entry_set_text(GTK_ENTRY(attw->comment_entry), "");
      }

      for(ii=0; ii < GAP_STB_ATT_TYPES_ARRAY_MAX; ii++)
      {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (attw->att_rows[ii].enable_toggle)
                                , attw->stb_elem_refptr->att_arr_enable[ii]);

        gtk_adjustment_set_value(GTK_ADJUSTMENT(attw->att_rows[ii].spinbutton_from_adj)
                                , attw->stb_elem_refptr->att_arr_value_from[ii] * CONVERT_TO_100PERCENT);
        gtk_adjustment_set_value(GTK_ADJUSTMENT(attw->att_rows[ii].spinbutton_to_adj)
                                , attw->stb_elem_refptr->att_arr_value_to[ii] * CONVERT_TO_100PERCENT);
        gtk_adjustment_set_value(GTK_ADJUSTMENT(attw->att_rows[ii].spinbutton_dur_adj)
                                , attw->stb_elem_refptr->att_arr_value_dur[ii]);

      }
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (attw->fit_width_toggle)
             , attw->stb_elem_refptr->att_fit_width);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (attw->fit_height_toggle)
             , attw->stb_elem_refptr->att_fit_height);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (attw->keep_proportions_toggle)
             , attw->stb_elem_refptr->att_keep_proportions);

      p_update_full_preview_gfx(attw);
      p_attw_update_properties(attw);
    }
  }

}  /* end p_attw_prop_reset_all */


/* ------------------
 * p_attw_timer_job
 * ------------------
 * render both graphical previews
 */
static void
p_attw_timer_job(GapStbAttrWidget *attw)
{
  GapStbMainGlobalParams  *sgpp;

  /*if(gap_debug) printf("\np_attw_timer_job: START\n");*/
  sgpp = attw->sgpp;

  if((attw)
  && (sgpp))
  {

    if(attw->go_timertag >= 0)
    {
      g_source_remove(attw->go_timertag);
    }
    attw->go_timertag = -1;

    p_render_gfx(attw, 0);
    p_render_gfx(attw, 1);
  }
}  /* end p_attw_timer_job */


/* ------------------------------------
 * p_attw_update_properties
 * ------------------------------------
 * render graphical view respecting all enabled attributes
 * opacity, move X/Y and Zoom X/Y
 */
static void
p_attw_update_properties(GapStbAttrWidget *attw)
{
  gap_story_dlg_tabw_update_frame_label(attw->tabw, attw->sgpp);

  if(attw->go_timertag < 0)
  {
    gint32 delay_millisec;

    delay_millisec = 100;
    attw->go_timertag = (gint32) g_timeout_add(delay_millisec
                                             , (GtkFunction)p_attw_timer_job
                                             , attw
                                             );
  }

}  /* end p_attw_update_properties */

/* ------------------------------------
 * p_attw_update_sensitivity
 * ------------------------------------
 * set sensitivity of all 5 from/to, dur attribute widgets
 * according to their enable flag.
 */
static void
p_attw_update_sensitivity(GapStbAttrWidget *attw)
{
  gint ii;
  gboolean sensitive;

  if(attw == NULL)
  {
    return;
  }
  if(attw->stb_elem_refptr == NULL)
  {
    return;
  }

  for(ii=0; ii < GAP_STB_ATT_TYPES_ARRAY_MAX; ii++)
  {
    sensitive = attw->stb_elem_refptr->att_arr_enable[ii];

    gtk_widget_set_sensitive(attw->att_rows[ii].spinbutton_from, sensitive);
    gtk_widget_set_sensitive(attw->att_rows[ii].spinbutton_to, sensitive);
    gtk_widget_set_sensitive(attw->att_rows[ii].spinbutton_dur, sensitive);
    gtk_widget_set_sensitive(attw->att_rows[ii].button_from, sensitive);
    gtk_widget_set_sensitive(attw->att_rows[ii].button_to, sensitive);
    gtk_widget_set_sensitive(attw->att_rows[ii].button_dur, sensitive);

  }

  sensitive = ((attw->stb_elem_refptr->att_fit_width == TRUE)
            || (attw->stb_elem_refptr->att_fit_height == TRUE));
  gtk_widget_set_sensitive(attw->keep_proportions_toggle, sensitive);

}  /* end p_attw_update_sensitivity */


/* --------------------------------
 * p_get_default_attribute
 * --------------------------------
 * get the typical default value depending
 * on the attribute type (specified via att_type_idx)
 * and depending on MODIFIER Keys held down when button was clicked
 * (specified via bevent)
 *
 * SHIFT: reset to start value (at dialog creation time)
 * CTRL:  left/top outside,  Half size,   50% opaque
 * ALT:   right/bot outside, Double size, 75% opaque
 */
static gdouble
p_get_default_attribute(GapStbAttrWidget *attw
                     , GdkEventButton  *bevent
                     , gint att_type_idx
                     , gboolean form_value)
{
  if(bevent)
  {
    if (bevent->state & GDK_CONTROL_MASK)
    {
      if ((att_type_idx == GAP_STB_ATT_TYPE_MOVE_X)
      ||  (att_type_idx == GAP_STB_ATT_TYPE_MOVE_Y))
      {
        return (-1.0);  /* -1.0 indicates left/top outside */
      }
      if ((att_type_idx == GAP_STB_ATT_TYPE_ZOOM_X)
      ||  (att_type_idx == GAP_STB_ATT_TYPE_ZOOM_Y))
      {
        return (0.5);  /* 1.0 scale to half size */
      }
      return (0.5);    /* indicates 50% opacity */
    }

    if(bevent->state & GDK_MOD1_MASK)  /* ALT */
    {
      if ((att_type_idx == GAP_STB_ATT_TYPE_MOVE_X)
      ||  (att_type_idx == GAP_STB_ATT_TYPE_MOVE_Y))
      {
        return (1.0);  /* 1.0 indicates right/bottom outside */
      }
      if ((att_type_idx == GAP_STB_ATT_TYPE_ZOOM_X)
      ||  (att_type_idx == GAP_STB_ATT_TYPE_ZOOM_Y))
      {
        return (2.0);  /* 1.0 scale to doble size */
      }
      return (0.75);  /* indicates 75% opacity */
    }

    if(bevent->state & GDK_SHIFT_MASK)
    {
      if(attw->stb_elem_bck)
      {
        if(form_value)
        {
          return (attw->stb_elem_bck->att_arr_value_from[att_type_idx]);
        }
        return(attw->stb_elem_bck->att_arr_value_to[att_type_idx]);
      }
    }
  }

  return (gap_story_get_default_attribute(att_type_idx));

}  /* end p_get_default_attribute */

/* -----------------------------------------
 * p_attw_start_button_clicked_callback
 * -----------------------------------------
 */
static void
p_attw_start_button_clicked_callback(GtkWidget *widget
               , GdkEventButton  *bevent
               , gint att_type_idx)
{
  GapStbAttrWidget *attw;
  attw = g_object_get_data( G_OBJECT(widget), OBJ_DATA_KEY_ATTW );
  if(attw)
  {
    if(attw->stb_elem_refptr)
    {
      gdouble attr_value;

      attr_value = CONVERT_TO_100PERCENT * p_get_default_attribute(attw
                        , bevent
                        , att_type_idx
                        , TRUE   /* use from value for reset */
                        );

      gtk_adjustment_set_value(GTK_ADJUSTMENT(attw->att_rows[att_type_idx].spinbutton_from_adj)
                                , attr_value);

    }
  }
}  /* end p_attw_start_button_clicked_callback */

/* -----------------------------------------
 * p_attw_end_button_clicked_callback
 * -----------------------------------------
 */
static void
p_attw_end_button_clicked_callback(GtkWidget *widget
               , GdkEventButton  *bevent
               , gint att_type_idx)
{
  GapStbAttrWidget *attw;
  attw = g_object_get_data( G_OBJECT(widget), OBJ_DATA_KEY_ATTW );
  if(attw)
  {
    if(attw->stb_elem_refptr)
    {
      gdouble attr_value;

      attr_value = CONVERT_TO_100PERCENT * p_get_default_attribute(attw
                        , bevent
                        , att_type_idx
                        , FALSE   /* use to value for reset */
                        );

      gtk_adjustment_set_value(GTK_ADJUSTMENT(attw->att_rows[att_type_idx].spinbutton_to_adj)
                                , attr_value);

    }
  }
}  /* end p_attw_end_button_clicked_callback */


/* -----------------------------------------
 * p_attw_dur_button_clicked_callback
 * -----------------------------------------
 */
static void
p_attw_dur_button_clicked_callback(GtkWidget *widget
               , GdkEventButton  *bevent
               , gint att_type_idx)
{
  GapStbAttrWidget *attw;
  attw = g_object_get_data( G_OBJECT(widget), OBJ_DATA_KEY_ATTW );
  if(attw)
  {
    if(attw->stb_elem_refptr)
    {
      gint ii;
      gint32 duration;

      duration = attw->stb_elem_refptr->att_arr_value_dur[att_type_idx];

      for(ii = 0; ii < GAP_STB_ATT_TYPES_ARRAY_MAX; ii++)
      {
        if(attw->stb_elem_refptr->att_arr_enable[ii] == TRUE)
        {
          gtk_adjustment_set_value(GTK_ADJUSTMENT(attw->att_rows[ii].spinbutton_dur_adj)
                                  , duration);
        }
      }

    }
  }
}  /* end p_attw_dur_button_clicked_callback */


/* ----------------------------------
 * p_attw_gdouble_adjustment_callback
 * ----------------------------------
 *
 * Notes: the value we get from the spinbutton is divided by 100
 *        because the GUI presents 100 percent as 100.0
 *        but internal representation uses the value 1.0
 *        for 100 percent.
 */
static void
p_attw_gdouble_adjustment_callback(GtkObject *obj, gdouble *val)
{
  GapStbAttrWidget *attw;
  gdouble l_val;


  attw = g_object_get_data( G_OBJECT(obj), OBJ_DATA_KEY_ATTW );
  if(attw)
  {
    if(attw->stb_elem_refptr)
    {
      l_val = (GTK_ADJUSTMENT(obj)->value) / CONVERT_TO_100PERCENT;
      if(gap_debug)
      {
        printf("gdouble_adjustment_callback: old_val:%f val:%f\n"
          , (float)*val
          , (float)l_val
          );
      }
      if(l_val != *val)
      {
        *val = l_val;
        attw->stb_refptr->unsaved_changes = TRUE;
        p_attw_update_properties(attw);
      }
    }
  }

}  /* end p_attw_gdouble_adjustment_callback */


/* ---------------------------------
 * p_duration_dependent_refresh
 * ---------------------------------
 */
static void
p_duration_dependent_refresh(GapStbAttrWidget *attw)
{
  gint img_idx;

  img_idx = 1;
  if(attw->gfx_tab[img_idx].auto_update)
  {
    gint32 duration;

    duration = p_get_relevant_duration(attw, img_idx);
    p_create_or_replace_orig_layer (attw, img_idx, duration);
    p_render_gfx (attw, img_idx);
  }
}  /* end p_duration_dependent_refresh */

/* ---------------------------------
 * p_attw_duration_adjustment_callback
 * ---------------------------------
 */
static void
p_attw_duration_adjustment_callback(GtkObject *obj, gint32 *val)
{
  GapStbAttrWidget *attw;
  gint32 l_val;


  attw = g_object_get_data( G_OBJECT(obj), OBJ_DATA_KEY_ATTW );
  if(attw)
  {
    if(attw->stb_elem_refptr)
    {
      l_val = RINT (GTK_ADJUSTMENT(obj)->value);
      if(gap_debug) printf("gint32_adjustment_callback: old_val:%d val:%d\n", (int)*val ,(int)l_val );
      if(l_val != *val)
      {
        *val = l_val;
        attw->stb_refptr->unsaved_changes = TRUE;
        p_duration_dependent_refresh(attw);
      }
    }
  }

}  /* end p_attw_duration_adjustment_callback */


/* ------------------------------------
 * p_attw_enable_toggle_update_callback
 * ------------------------------------
 */
static void
p_attw_enable_toggle_update_callback(GtkWidget *widget, gboolean *val)
{
  GapStbAttrWidget *attw;

  attw = g_object_get_data( G_OBJECT(widget), OBJ_DATA_KEY_ATTW );
  if((attw) && (val))
  {
    if(attw->stb_elem_refptr)
    {
      attw->stb_refptr->unsaved_changes = TRUE;

      if (GTK_TOGGLE_BUTTON (widget)->active)
      {
        *val = TRUE;
      }
      else
      {
        *val = FALSE;
      }
      p_attw_update_sensitivity(attw);
      p_duration_dependent_refresh(attw);
      p_attw_update_properties(attw);
    }
  }
}  /* end p_attw_enable_toggle_update_callback */



/* -----------------------------------------
 * p_attw_auto_update_toggle_update_callback
 * -----------------------------------------
 */
static void
p_attw_auto_update_toggle_update_callback(GtkWidget *widget, gboolean *val)
{
  GapStbAttrWidget *attw;

  attw = g_object_get_data( G_OBJECT(widget), OBJ_DATA_KEY_ATTW );
  if((attw) && (val))
  {
    if(attw->stb_elem_refptr)
    {
      attw->stb_refptr->unsaved_changes = TRUE;

      if (GTK_TOGGLE_BUTTON (widget)->active)
      {
        *val = TRUE;
        p_update_full_preview_gfx(attw);
      }
      else
      {
        *val = FALSE;
      }
    }
  }
}  /* end p_attw_auto_update_toggle_update_callback */


/* ---------------------------------
 * p_attw_preview_events_cb
 * ---------------------------------
 */
static gboolean
p_attw_preview_events_cb (GtkWidget *widget
                       , GdkEvent  *event
                       , GapStbAttrWidget *attw)
{
  GdkEventExpose *eevent;
  GdkEventButton *bevent;
  GapStbMainGlobalParams  *sgpp;
  gint                     img_idx;

  if ((attw->stb_elem_refptr == NULL)
  ||  (attw->stb_refptr == NULL))
  {
    /* the attribute widget is not initialized no action allowed */
    return FALSE;
  }
  sgpp = attw->sgpp;

  img_idx = (gint)g_object_get_data( G_OBJECT(widget), OBJ_DATA_KEY_IMG_IDX );
  if(img_idx != 0)
  {
    img_idx = 1;
  }


  switch (event->type)
  {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      // TODO: define actions when button pressed.

      /* debug code to display a copy of the image */
      if(1==0)
      {
          gint32 dup_id;

          dup_id = gimp_image_duplicate(attw->gfx_tab[img_idx].image_id);
          gimp_display_new(dup_id);
      }

      return FALSE;
      break;

    case GDK_EXPOSE:
      if(gap_debug)
      {
        printf("p_attw_preview_events_cb GDK_EXPOSE widget:%d  img_idx:%d\n"
                              , (int)widget
                              , (int)img_idx
                              );
      }

      eevent = (GdkEventExpose *) event;

      gap_pview_repaint(attw->gfx_tab[img_idx].pv_ptr);
      gdk_flush ();

      break;

    default:
      break;
  }

  return FALSE;
}       /* end  p_attw_preview_events_cb */


/* -----------------------------------------
 * p_calculate_render_attributes
 * -----------------------------------------
 * NOTE: filtermacro processing is ignored,
 *       the preview
 */
static void
p_calculate_render_attributes(GapStbAttrWidget *attw
    , gint img_idx
    , GapStoryCalcAttr  *calc_attr_ptr
    )
{
  gint ii;
  gint32  pv_master_width;   /* master width scaled to preview size */
  gint32  pv_master_height;  /* master height scaled to preview size */
  gdouble master_scale;
  gdouble att_tab[GAP_STB_ATT_GFX_ARRAY_MAX][GAP_STB_ATT_TYPES_ARRAY_MAX];
  gdouble pv_frame_width;
  gdouble pv_frame_height;

  pv_master_width = gimp_image_width(attw->gfx_tab[img_idx].image_id) * PVIEW_TO_MASTER_SCALE;
  pv_master_height = gimp_image_height(attw->gfx_tab[img_idx].image_id) * PVIEW_TO_MASTER_SCALE;

  master_scale = (gdouble)pv_master_width
               / (gdouble)(MAX( 1, attw->stb_refptr->master_width));

  pv_frame_width = (gdouble)gimp_drawable_width(attw->gfx_tab[img_idx].orig_layer_id) * master_scale;
  pv_frame_height = (gdouble)gimp_drawable_height(attw->gfx_tab[img_idx].orig_layer_id) * master_scale;

  for(ii=0; ii < GAP_STB_ATT_TYPES_ARRAY_MAX; ii++)
  {
    if(attw->stb_elem_refptr->att_arr_enable[ii] == TRUE)
    {
      att_tab[0][ii] =  attw->stb_elem_refptr->att_arr_value_from[ii];
      att_tab[1][ii] =  attw->stb_elem_refptr->att_arr_value_to[ii];
    }
    else
    {
      att_tab[0][ii] =  gap_story_get_default_attribute(ii);
      att_tab[1][ii] =  gap_story_get_default_attribute(ii);
    }
  }

  gap_story_file_calculate_render_attributes(calc_attr_ptr
    , gimp_image_width(attw->gfx_tab[img_idx].image_id)
    , gimp_image_height(attw->gfx_tab[img_idx].image_id)
    , pv_master_width
    , pv_master_height
    , (gint32)rint(pv_frame_width)
    , (gint32)rint(pv_frame_height)
    , attw->stb_elem_refptr->att_keep_proportions
    , attw->stb_elem_refptr->att_fit_width
    , attw->stb_elem_refptr->att_fit_height
    , att_tab [img_idx][GAP_STB_ATT_TYPE_OPACITY]
    , att_tab [img_idx][GAP_STB_ATT_TYPE_ZOOM_X]
    , att_tab [img_idx][GAP_STB_ATT_TYPE_ZOOM_Y]
    , att_tab [img_idx][GAP_STB_ATT_TYPE_MOVE_X]
    , att_tab [img_idx][GAP_STB_ATT_TYPE_MOVE_Y]
    );


}  /* end p_calculate_render_attributes */



/* -----------------------------------------
 * p_check_and_make_orig_default_layer
 * -----------------------------------------
 * check if there is a valid orig_layer_id (>= 0)
 * if NOT create a faked orig layers at master size.
 */
static void
p_check_and_make_orig_default_layer(GapStbAttrWidget *attw, gint img_idx)
{
  if(attw->gfx_tab[img_idx].orig_layer_id < 0)
  {
    gint32  image_id;
    gint32  layer_id;
    gdouble red, green, blue, alpha;

    image_id = attw->gfx_tab[img_idx].image_id;
    layer_id = gimp_layer_new(image_id
                  , LAYERNAME_ORIG
                  , attw->stb_refptr->master_width
                  , attw->stb_refptr->master_height
                  , GIMP_RGBA_IMAGE
                  , 100.0   /* full opacity */
                  , 0       /* normal mode */
                  );

    gimp_image_add_layer (image_id, layer_id, LAYERSTACK_TOP);
    red   = 0.42;
    green = 0.90;
    blue  = 0.35;
    alpha = 1.0;
    gap_layer_clear_to_color(layer_id, red, green, blue, alpha);

    attw->gfx_tab[img_idx].orig_layer_id = layer_id;
    attw->gfx_tab[img_idx].orig_layer_is_fake = TRUE;
  }
  gimp_drawable_set_visible(attw->gfx_tab[img_idx].orig_layer_id, FALSE);

}  /* end p_check_and_make_orig_default_layer */


/* -----------------------------------------
 * p_create_color_layer
 * -----------------------------------------
 * create a layer with specified color at image size (== preview size)
 * with a transparent rectangle at scaled master size in the center.
 */
static gint32
p_create_color_layer(GapStbAttrWidget *attw, gint32 image_id
    , const char *name, gint stackposition, gdouble opacity
    , gdouble red, gdouble green, gdouble blue)
{
  gint32 layer_id;
  gdouble ofs_x;
  gdouble ofs_y;
  gdouble pv_master_width;   /* master width scaled to preview size */
  gdouble pv_master_height;  /* master height scaled to preview size */

  layer_id = gimp_layer_new(image_id
                  , name
                  , gimp_image_width(image_id)
                  , gimp_image_height(image_id)
                  , GIMP_RGBA_IMAGE
                  , opacity
                  , 0       /* normal mode */
                  );
  gimp_image_add_layer (image_id, layer_id, stackposition);
  gap_layer_clear_to_color(layer_id, red, green, blue, 1.0);
  gimp_drawable_set_visible(layer_id, TRUE);

  pv_master_width = (gdouble)gimp_image_width(image_id) * PVIEW_TO_MASTER_SCALE;
  pv_master_height = (gdouble)gimp_image_height(image_id) * PVIEW_TO_MASTER_SCALE;
  ofs_x = ((gdouble)gimp_image_width(image_id) - pv_master_width) / 2.0;
  ofs_y = ((gdouble)gimp_image_height(image_id) - pv_master_height) / 2.0;

  /* green border */
  {
    GimpRGB  color;
    GimpRGB  bck_color;

    color.r = 0.25;
    color.g = 0.80;
    color.b = 0.17;
    color.a = 1.0;

    /* selection for green rectangle (2 pixels border around master size) */
    gimp_rect_select(image_id
                  , ofs_x -2
                  , ofs_y -2
                  , pv_master_width +4
                  , pv_master_height +4
                  , GIMP_CHANNEL_OP_REPLACE
                  , 0       /* gint32 feather */
                  , 0.0     /* gdouble feather radius */
                  );

    gimp_context_get_background(&bck_color);
    gimp_context_set_background(&color);
    /* fill the drawable (ignoring selections) */
    gimp_edit_fill(layer_id, GIMP_BACKGROUND_FILL);


    /* restore BG color in the context */
    gimp_context_set_background(&bck_color);
  }

  gimp_rect_select(image_id
                  , ofs_x
                  , ofs_y
                  , pv_master_width
                  , pv_master_height
                  , GIMP_CHANNEL_OP_REPLACE
                  , 0       /* gint32 feather */
                  , 0.0     /* gdouble feather radius */
                  );

  gimp_edit_clear(layer_id);
  gimp_selection_none(image_id);


  return (layer_id);
}  /* end p_create_color_layer */


/* -----------------------------------------
 * p_create_base_layer
 * -----------------------------------------
 */
static gint32
p_create_base_layer(GapStbAttrWidget *attw, gint32 image_id)
{
  gint32 layer_id;
  gdouble  red, green, blue, alpha;

  red   = 0.86;
  green = 0.85;
  blue  = 0.84;
  alpha = 1.0;

  layer_id = p_create_color_layer(attw
    , image_id
    , LAYERNAME_BASE
    , LAYERSTACK_BASE
    , 100.0            /* full opaque */
    , red
    , green
    , blue
    );

  return (layer_id);
}  /* end p_create_base_layer */

/* -----------------------------------------
 * p_create_deco_layer
 * -----------------------------------------
 */
static gint32
p_create_deco_layer(GapStbAttrWidget *attw, gint32 image_id)
{
  gint32 layer_id;
  gdouble  red, green, blue, alpha;

  red   = 0.86;
  green = 0.85;
  blue  = 0.84;
  alpha = 1.0;

  layer_id = p_create_color_layer(attw
    , image_id
    , LAYERNAME_DECO
    , LAYERSTACK_TOP
    , 60.0            /* 70% opaque */
    , red
    , green
    , blue
    );

  return (layer_id);
}  /* end p_create_deco_layer */

/* -----------------------------------------
 * p_create_gfx_image
 * -----------------------------------------
 */
static void
p_create_gfx_image(GapStbAttrWidget *attw, gint img_idx)
{
  gint32 image_id;

  image_id = gimp_image_new( attw->gfx_tab[img_idx].pv_ptr->pv_width
                           , attw->gfx_tab[img_idx].pv_ptr->pv_height
                           , GIMP_RGB
                           );
  gimp_image_undo_disable (image_id);

  attw->gfx_tab[img_idx].base_layer_id = p_create_base_layer(attw, image_id);
  attw->gfx_tab[img_idx].deco_layer_id = p_create_deco_layer(attw, image_id);
  attw->gfx_tab[img_idx].orig_layer_id = -1;  /* to be created later */
  attw->gfx_tab[img_idx].curr_layer_id = -1;  /* to be created later */
  attw->gfx_tab[img_idx].image_id = image_id;
  attw->gfx_tab[img_idx].orig_layer_is_fake = TRUE;
}  /* end p_create_gfx_image */

/* -----------------------------------------
 * p_delete_gfx_images
 * -----------------------------------------
 */
static void
p_delete_gfx_images(GapStbAttrWidget *attw)
{
  gimp_image_delete(attw->gfx_tab[0].image_id);
  gimp_image_delete(attw->gfx_tab[1].image_id);

  attw->gfx_tab[0].image_id = -1;
  attw->gfx_tab[1].image_id = -1;

}  /* end p_delete_gfx_images */


/* -----------------------------------------
 * p_render_gfx
 * -----------------------------------------
 * render graphical preview.
 * This procedure does not include fetching the refered frame
 * It assumes that the corresponding frame has already been fetched
 * into the org layer of the internal gimp image that will be used for
 * rendering of the preview.
 * If that image doe not contain such a valid orig layer,
 * it creates an empty default representation, where master size is assumed.
 */
static void
p_render_gfx(GapStbAttrWidget *attw, gint img_idx)
{
  GapStoryCalcAttr  calculate_attributes;
  GapStoryCalcAttr  *calculated;
  gint32  image_id;

  image_id = attw->gfx_tab[img_idx].image_id;
  calculated = &calculate_attributes;

  p_check_and_make_orig_default_layer(attw, img_idx);

  p_calculate_render_attributes (attw
     , img_idx
     , calculated
     );


  /* if size is not equal calculated size remove curr layer
   * to force recreation in desired size
   */
  if (attw->gfx_tab[img_idx].curr_layer_id >= 0)
  {
     if ((gimp_drawable_width(attw->gfx_tab[img_idx].curr_layer_id)  != calculated->width)
     ||  (gimp_drawable_height(attw->gfx_tab[img_idx].curr_layer_id) != calculated->height))
     {
        gimp_image_remove_layer( image_id
                               , attw->gfx_tab[img_idx].curr_layer_id);
        attw->gfx_tab[img_idx].curr_layer_id = -1;
     }
  }

  /* check and recreate the current layer at stackposition 1 */
  if (attw->gfx_tab[img_idx].curr_layer_id < 0)
  {
    gint32 new_layer_id;


    new_layer_id = gimp_layer_copy(attw->gfx_tab[img_idx].orig_layer_id);
    gimp_image_add_layer (image_id, new_layer_id, LAYERSTACK_CURR);
    gimp_drawable_set_name(new_layer_id, LAYERNAME_CURR);
    gimp_layer_scale(new_layer_id, calculated->width, calculated->height, 0);
    gimp_drawable_set_visible(new_layer_id, TRUE);

    attw->gfx_tab[img_idx].curr_layer_id = new_layer_id;
  }


  {
    gint32 layer_id;
    gint   ii;


    /* adjust stack position */
    layer_id = attw->gfx_tab[img_idx].curr_layer_id;
    gimp_image_lower_layer_to_bottom (image_id, layer_id);
    for (ii=0; ii < LAYERSTACK_CURR; ii++)
    {
      gimp_image_raise_layer (image_id, layer_id);
    }
  }


  /* set offsets and opacity */
  gimp_layer_set_offsets(attw->gfx_tab[img_idx].curr_layer_id
                        , calculated->x_offs
                        , calculated->y_offs
                        );
  gimp_layer_set_opacity(attw->gfx_tab[img_idx].curr_layer_id
                        , calculated->opacity
                        );

  gimp_drawable_set_visible(attw->gfx_tab[img_idx].orig_layer_id, FALSE);
  gimp_drawable_set_visible(attw->gfx_tab[img_idx].deco_layer_id, TRUE);
  gimp_drawable_set_visible(attw->gfx_tab[img_idx].curr_layer_id, TRUE);
  gimp_drawable_set_visible(attw->gfx_tab[img_idx].base_layer_id, TRUE);

  /* render the preview from image */
  gap_pview_render_from_image_duplicate (attw->gfx_tab[img_idx].pv_ptr
                                , image_id
                                );
  gtk_widget_queue_draw(attw->gfx_tab[img_idx].pv_ptr->da_widget);
  gdk_flush();
}  /* end p_render_gfx */


/* -----------------------------------
 * p_update_framenr_labels
 * -----------------------------------
 * update both frame number and time label 000001 00:00:000
 * for the specified start or end gfx_preview (via img_idx)
 */
static void
p_update_framenr_labels(GapStbAttrWidget *attw, gint img_idx, gint32 framenr)
{
  char    txt_buf[100];
  gdouble l_speed_fps;

  if(attw == NULL) { return; }
  if(attw->stb_elem_refptr == NULL) { return; }


  g_snprintf(txt_buf, sizeof(txt_buf), "%d  "
            ,(int)framenr
            );
  gtk_label_set_text ( GTK_LABEL(attw->gfx_tab[img_idx].framenr_label), txt_buf);

  l_speed_fps = GAP_STORY_DEFAULT_FRAMERATE;
  if(attw->stb_refptr)
  {
    if(attw->stb_refptr->master_framerate > 0)
    {
      l_speed_fps = attw->stb_refptr->master_framerate;
    }
  }

  gap_timeconv_framenr_to_timestr(framenr -1
                         , l_speed_fps
                         , txt_buf
                         , sizeof(txt_buf)
                         );
  gtk_label_set_text ( GTK_LABEL(attw->gfx_tab[img_idx].frametime_label), txt_buf);

}  /* end p_update_framenr_labels */


/* -----------------------------------------
 * p_get_relevant_duration
 * -----------------------------------------
 */
static gint32
p_get_relevant_duration(GapStbAttrWidget *attw, gint img_idx)
{
  gint32 duration;

  /* for the start frame ( img_idx ==0) always use duration 0 */
  duration = 0;
  if (img_idx != 0)
  {
    gint ii;

    /* for the end frame pick the duration from the first enabled attribute */
    for(ii=0; ii < GAP_STB_ATT_TYPES_ARRAY_MAX; ii++)
    {
      if(attw->stb_elem_refptr->att_arr_enable[ii] == TRUE)
      {
        duration = attw->stb_elem_refptr->att_arr_value_dur[ii];
        break;
      }
    }

  }

  return(duration);
}  /* end p_get_relevant_duration */


/* -----------------------------------------
 * p_update_full_preview_gfx
 * -----------------------------------------
 * update the previews (both start and end) with the corresponding auto_update flag set.
 * for rendering of the preview(s)
 * and fetching of the refered frame at orig size into orig_layer.
 */
static void
p_update_full_preview_gfx(GapStbAttrWidget *attw)
{
   gint img_idx;

   for(img_idx = 0; img_idx < GAP_STB_ATT_GFX_ARRAY_MAX; img_idx++)
   {
     if(attw->gfx_tab[img_idx].auto_update)
     {
       gint32 duration;

       duration = p_get_relevant_duration(attw, img_idx);
       p_create_or_replace_orig_layer (attw, img_idx, duration);
       p_render_gfx (attw, img_idx);
     }
   }

}  /* end p_update_full_preview_gfx */




/* ---------------------------------
 * p_stb_reg_equals_orig_layer
 * ---------------------------------
 * check if required position (specified via stb_ret and filename)
 * refers to the same frame as the orig layer.
 * the relevant infornmation of the orig_layer is stored in
 *      orig_layer_record_type;
 *      orig_layer_local_framenr;
 *      orig_layer_seltrack;
 *     *orig_layer_filename;
 *
 */
static gboolean
p_stb_reg_equals_orig_layer(GapStbAttrWidget *attw
                         , gint   img_idx
                         , GapStoryLocateRet *stb_ret
                         , const char *filename)
{
  if (stb_ret->stb_elem->record_type
  != attw->gfx_tab[img_idx].orig_layer_record_type)
  {
    return(FALSE);
  }
  if (stb_ret->stb_elem->seltrack != attw->gfx_tab[img_idx].orig_layer_seltrack)
  {
    return(FALSE);
  }

  if (stb_ret->ret_framenr != attw->gfx_tab[img_idx].orig_layer_local_framenr)
  {
    return(FALSE);
  }

  if (attw->gfx_tab[img_idx].orig_layer_filename == NULL)
  {
     if(filename != NULL)
     {
       return(FALSE);
     }
  }
  else
  {
     if(filename == NULL)
     {
       return(FALSE);
     }
     if(strcmp(filename, attw->gfx_tab[img_idx].orig_layer_filename) != 0)
     {
       return(FALSE);
     }
  }

  return(TRUE);
}

/* ---------------------------------
 * p_destroy_orig_layer
 * ---------------------------------
 */
static void
p_destroy_orig_layer(GapStbAttrWidget *attw
                    , gint   img_idx)
{
   if(attw->gfx_tab[img_idx].orig_layer_id >= 0)
   {
     /* not up to date: destroy orig layer */
     gimp_image_remove_layer(attw->gfx_tab[img_idx].image_id
                            , attw->gfx_tab[img_idx].orig_layer_id);
     attw->gfx_tab[img_idx].orig_layer_id = -1;

   }

   attw->gfx_tab[img_idx].orig_layer_record_type = GAP_STBREC_VID_UNKNOWN;
   attw->gfx_tab[img_idx].orig_layer_local_framenr = -1;
   attw->gfx_tab[img_idx].orig_layer_seltrack = -1;
   if (attw->gfx_tab[img_idx].orig_layer_filename != NULL)
   {
     g_free(attw->gfx_tab[img_idx].orig_layer_filename);
     attw->gfx_tab[img_idx].orig_layer_filename = NULL;
   }

   /* remove curr layer if already exists */
   if(attw->gfx_tab[img_idx].curr_layer_id >= 0)
   {
     gimp_image_remove_layer(attw->gfx_tab[img_idx].image_id
                            , attw->gfx_tab[img_idx].curr_layer_id);
     attw->gfx_tab[img_idx].curr_layer_id = -1;
   }

}  /* end p_destroy_orig_layer */

/* ---------------------------------
 * p_check_orig_layer_up_to_date
 * ---------------------------------
 * check if there is an orig layer and if it is still
 * up to date. (return TRUE in that case)
 * else return FALSE.
 *
 * if there is a non-up to date orig layer destroy this outdated
 * orig layer and the curr_layer too.
 */
static gboolean
p_check_orig_layer_up_to_date(GapStbAttrWidget *attw
                         , gint   img_idx
                         , GapStoryLocateRet *stb_ret
                         , const char *filename)
{
   /* remove orig layer if already exists */
   if(attw->gfx_tab[img_idx].orig_layer_id >= 0)
   {

     if (p_stb_reg_equals_orig_layer(attw, img_idx, stb_ret, filename) == TRUE)
     {
       return (TRUE);  /* OK orig layer already up to date */
     }
   }
   p_destroy_orig_layer(attw, img_idx);


   return (FALSE);
}  /* end p_check_orig_layer_up_to_date */


/* ---------------------------------
 * p_create_or_replace_orig_layer
 * ---------------------------------
 * fetch the frame that is refered by the stb_elem_refptr + duration (frames)
 * If there is no frame found, use a default image.
 * (possible reasons: duration refers behind valid frame range,
 *  video support is turned off at compile time, but reference points
 *  into a movie clip)
 *
 * the fetched (or default) frame is added as invisible layer with alpha channel
 * on top of the layerstack in the specified image as orig_layer_id.
 * (an already existing orig_layer_id will be replaced if there exists one)
 *
 * If the orig_layer is up to date the fetch is not performed.
 */
static void
p_create_or_replace_orig_layer (GapStbAttrWidget *attw
                         , gint   img_idx
                         , gint32 duration)
{
   gint32     l_layer_id;
   char      *l_filename;
   gint32     l_framenr;


   l_layer_id = -1;
   l_filename = NULL;


   /* calculate the absolute frame number for the frame to access */
   l_framenr = gap_story_get_framenr_by_story_id(attw->stb_refptr
                                                ,attw->stb_elem_refptr->story_id
                                                ,attw->stb_elem_refptr->track
                                                );
   /* stb_elem_refptr refers to the transition attribute stb_elem
    */
   l_framenr += duration;
   p_update_framenr_labels(attw, img_idx, l_framenr);

   {
     GapStoryLocateRet *stb_ret;

     stb_ret = gap_story_locate_framenr(attw->stb_refptr
                                    , l_framenr
                                    , attw->stb_elem_refptr->track
                                    );
     if(stb_ret)
     {
       if ((stb_ret->locate_ok)
       && (stb_ret->stb_elem))
       {
         l_filename = gap_story_get_filename_from_elem_nr(stb_ret->stb_elem
                                                , stb_ret->ret_framenr
                                               );

         if(p_check_orig_layer_up_to_date(attw
                         , img_idx
                         , stb_ret
                         , l_filename) == TRUE)
         {
           g_free(stb_ret);
           if(l_filename)
           {
             g_free(l_filename);
           }
           return;  /* orig_layer is still up to date, done */
         }

         if(stb_ret->stb_elem->record_type == GAP_STBREC_VID_MOVIE)
         {
           if(l_filename)
           {
             /* fetch_full_sized_frame */
             l_layer_id = p_fetch_video_frame_as_layer(attw->sgpp
                ,l_filename
                ,stb_ret->ret_framenr
                ,stb_ret->stb_elem->seltrack
                ,gap_story_get_preferred_decoder(attw->stb_refptr
                                                 ,stb_ret->stb_elem
                                                 )
                ,attw->gfx_tab[img_idx].image_id
                );
             if(l_layer_id > 0)
             {
                attw->gfx_tab[img_idx].orig_layer_record_type = stb_ret->stb_elem->record_type;
                attw->gfx_tab[img_idx].orig_layer_local_framenr = stb_ret->ret_framenr;
                attw->gfx_tab[img_idx].orig_layer_seltrack = stb_ret->stb_elem->seltrack;
                if (attw->gfx_tab[img_idx].orig_layer_filename != NULL)
                {
                  g_free(attw->gfx_tab[img_idx].orig_layer_filename);
                  attw->gfx_tab[img_idx].orig_layer_filename = g_strdup(l_filename);
                }
             }
           }
         }
         else
         {
           /* record type is IMAGE or FRAME, fetch full size image
            * TODO: anim frame and COLOR not handled
            */
           l_layer_id = p_fetch_imagefile_as_layer(l_filename
                            ,attw->gfx_tab[img_idx].image_id
                            );
           if(l_layer_id > 0)
           {
              attw->gfx_tab[img_idx].orig_layer_record_type = stb_ret->stb_elem->record_type;
              attw->gfx_tab[img_idx].orig_layer_local_framenr = 0;
              attw->gfx_tab[img_idx].orig_layer_seltrack = 1;
              if (attw->gfx_tab[img_idx].orig_layer_filename != NULL)
              {
                g_free(attw->gfx_tab[img_idx].orig_layer_filename);
                attw->gfx_tab[img_idx].orig_layer_filename = g_strdup(l_filename);
              }
           }
         }
       }
       g_free(stb_ret);
     }
   }

   attw->gfx_tab[img_idx].orig_layer_id = l_layer_id;
   if(l_layer_id >= 0)
   {
     attw->gfx_tab[img_idx].orig_layer_is_fake = FALSE;
   }
   else
   {
     p_destroy_orig_layer(attw, img_idx);
   }

   /* check if we have a valid orig layer, create it if have not
    * (includes setting invisible)
    */
   p_check_and_make_orig_default_layer(attw, img_idx);

   if(l_filename)
   {
     g_free(l_filename);
   }

}       /* end p_create_or_replace_orig_layer */


/* ---------------------------------
 * p_fetch_video_frame_as_layer
 * ---------------------------------
 */
static gint32
p_fetch_video_frame_as_layer(GapStbMainGlobalParams *sgpp
                   , const char *video_filename
                   , gint32 framenumber
                   , gint32 seltrack
                   , const char *preferred_decoder
                   , gint32 image_id
                   )
{
  guchar *img_data;
  gint32 img_width;
  gint32 img_height;
  gint32 img_bpp;
  gboolean do_scale;
  gint32 l_new_layer_id;

  l_new_layer_id  = -1;
  do_scale = FALSE;

  /* Fetch the wanted Frame from the Videofile */
  img_data = gap_story_dlg_fetch_videoframe(sgpp
                   , video_filename
                   , framenumber
                   , seltrack
                   , preferred_decoder
                   , 1.5               /* delace */
                   , &img_bpp
                   , &img_width
                   , &img_height
                   , do_scale
                   );

  if(img_data != NULL)
  {
    GimpDrawable *drawable;
    GimpPixelRgn pixel_rgn;
    GimpImageType type;

    /* Fill in the alpha channel (for RGBA type) */
    if(img_bpp == 4)
    {
       gint i;

       for (i=(img_width * img_height)-1; i>=0; i--)
       {
         img_data[3+(i*4)] = 255;
       }
    }


    type = GIMP_RGB_IMAGE;
    if(img_bpp == 4)
    {
      type = GIMP_RGBA_IMAGE;
    }

    l_new_layer_id = gimp_layer_new (image_id
                                , LAYERNAME_ORIG
                                , img_width
                                , img_height
                                , type
                                , 100.0
                                , GIMP_NORMAL_MODE);

    drawable = gimp_drawable_get (l_new_layer_id);
    gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0
                      , drawable->width, drawable->height
                      , TRUE      /* dirty */
                      , FALSE     /* shadow */
                       );
    gimp_pixel_rgn_set_rect (&pixel_rgn, img_data
                          , 0
                          , 0
                          , img_width
                          , img_height);

    if(! gimp_drawable_has_alpha (l_new_layer_id))
    {
      /* implicite add an alpha channel before we try to raise */
      gimp_layer_add_alpha(l_new_layer_id);
    }

    gimp_drawable_flush (drawable);
    gimp_image_add_layer (image_id,l_new_layer_id, LAYERSTACK_TOP);
  }

  return(l_new_layer_id);

}  /* end p_fetch_video_frame_as_layer */


/* ---------------------------------
 * p_fetch_imagefile_as_layer
 * ---------------------------------
 */
static gint32
p_fetch_imagefile_as_layer (const char *img_filename
                   , gint32 image_id
                   )
{
  gint32 l_tmp_image_id;
  gint32 l_new_layer_id;

  l_new_layer_id  = -1;

  {
    char  *l_filename;
    l_filename = g_strdup(img_filename);
    l_tmp_image_id = gap_lib_load_image(l_filename);
    g_free(l_filename);
  }

  if(l_tmp_image_id >= 0)
  {
    gint32  l_layer_id;
    gint    l_src_offset_x, l_src_offset_y;

    gimp_image_undo_disable (l_tmp_image_id);
    l_layer_id = gimp_image_flatten(l_tmp_image_id);
    if(l_layer_id < 0)
    {
      l_layer_id = gimp_layer_new(l_tmp_image_id, LAYERNAME_ORIG,
                             1,
                             1,
                             ((gint)(gimp_image_base_type(l_tmp_image_id)) * 2),
                             100.0,     /* Opacity full opaque */
                             0);        /* NORMAL */
      gimp_image_add_layer(l_tmp_image_id, l_layer_id, LAYERSTACK_TOP);
      gimp_layer_set_offsets(l_layer_id, -1, -1);
      l_layer_id = gimp_image_flatten(l_tmp_image_id);
    }
    gimp_layer_add_alpha(l_layer_id);

    /* copy the layer from the temp image to the preview multilayer image */
    l_new_layer_id = gap_layer_copy_to_dest_image(image_id,
                                   l_layer_id,
                                   100.0,       /* opacity full */
                                   0,           /* NORMAL */
                                   &l_src_offset_x,
                                   &l_src_offset_y
                                   );

    gimp_image_add_layer (image_id, l_new_layer_id, LAYERSTACK_TOP);

    /* destroy the tmp image */
    gimp_image_delete(l_tmp_image_id);
  }


  return(l_new_layer_id);
}  /* end p_fetch_imagefile_as_layer */




/* ------------------------------
 * p_attw_comment_entry_update_cb
 * ------------------------------
 */
static void
p_attw_comment_entry_update_cb(GtkWidget *widget, GapStbAttrWidget *attw)
{
  if(attw == NULL) { return; }
  if(attw->stb_elem_refptr == NULL) { return; }
  if(attw->stb_refptr)
  {
    attw->stb_refptr->unsaved_changes = TRUE;
  }

  if(attw->stb_elem_refptr->comment == NULL)
  {
    attw->stb_elem_refptr->comment = gap_story_new_elem(GAP_STBREC_VID_COMMENT);
  }

  if(attw->stb_elem_refptr->comment)
  {
    if(attw->stb_elem_refptr->comment->orig_src_line)
    {
      g_free(attw->stb_elem_refptr->comment->orig_src_line);
    }
    attw->stb_elem_refptr->comment->orig_src_line = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
  }
}  /* end p_attw_comment_entry_update_cb */


/* -----------------------------------
 * p_create_and_attach_pv_widgets
 * -----------------------------------
 * creates preview widgets
 */
static void
p_create_and_attach_pv_widgets(GapStbAttrWidget *attw
   , GtkWidget *table
   , gint row
   , gint col_start
   , gint col_end
   , gint img_idx
  )
{
  GtkWidget *vbox2;
  GtkWidget *hbox;
  GtkWidget *alignment;
  GtkWidget *check_button;
  GtkWidget *label;
  GtkWidget *event_box;
  GtkWidget *aspect_frame;
  gint32     l_check_size;
  gdouble    thumb_scale;


  /* the vox2  */
  vbox2 = gtk_vbox_new (FALSE, 1);

  /* aspect_frame is the CONTAINER for the video preview */
  aspect_frame = gtk_aspect_frame_new (NULL   /* without label */
                                      , 0.5   /* xalign center */
                                      , 0.5   /* yalign center */
                                      , attw->stb_refptr->master_width / attw->stb_refptr->master_height     /* ratio */
                                      , TRUE  /* obey_child */
                                      );
  gtk_widget_show (aspect_frame);

  /* Create an EventBox (container for the preview drawing_area)
   */
  event_box = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (event_box), vbox2);
  gtk_widget_set_events (event_box, GDK_BUTTON_PRESS_MASK);


  /*  The preview  */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox2), alignment, FALSE, FALSE, 1);
  gtk_widget_show (alignment);

  /* calculate scale factor (the preview has same proportions as the master
   * and fits into a sqare of size PVIEW_SIZE x PVIEW_SIZE)
   */
  {
    gdouble master_size;

    master_size = (gdouble)MAX(attw->stb_refptr->master_width, attw->stb_refptr->master_height);
    thumb_scale = (gdouble)PVIEW_SIZE / master_size;
  }
  l_check_size = PVIEW_SIZE / 32;
  attw->gfx_tab[img_idx].pv_ptr = gap_pview_new( (thumb_scale * (gdouble)attw->stb_refptr->master_width) + 4.0
                            , (thumb_scale * (gdouble)attw->stb_refptr->master_height) + 4.0
                            , l_check_size
                            , aspect_frame
                            );

  gtk_widget_set_events (attw->gfx_tab[img_idx].pv_ptr->da_widget, GDK_EXPOSURE_MASK);
  gtk_container_add (GTK_CONTAINER (aspect_frame), attw->gfx_tab[img_idx].pv_ptr->da_widget);
  gtk_container_add (GTK_CONTAINER (alignment), aspect_frame);
  gtk_widget_show (attw->gfx_tab[img_idx].pv_ptr->da_widget);

  /* create a gimp image that is internally used for rendering the preview
   * (create no view for this image, to keep it invisible for the user)
   */
  p_create_gfx_image(attw, img_idx);

  g_object_set_data(G_OBJECT(attw->gfx_tab[img_idx].pv_ptr->da_widget)
                   , OBJ_DATA_KEY_IMG_IDX, (gpointer)img_idx);
  g_object_set_data(G_OBJECT(event_box)
                   , OBJ_DATA_KEY_IMG_IDX, (gpointer)img_idx);

  g_signal_connect (G_OBJECT (attw->gfx_tab[img_idx].pv_ptr->da_widget), "event",
                    G_CALLBACK (p_attw_preview_events_cb),
                    attw);
  g_signal_connect (G_OBJECT (event_box), "button_press_event",
                    G_CALLBACK (p_attw_preview_events_cb),
                    attw);




  hbox = gtk_hbox_new (FALSE, 1);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 1);

  /* auto update toggle  check button */
  check_button = gtk_check_button_new_with_label (_("Update"));
  attw->gfx_tab[img_idx].auto_update_toggle = check_button;
  gtk_box_pack_start (GTK_BOX (hbox), check_button, FALSE, FALSE, 1);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button)
      , attw->gfx_tab[img_idx].auto_update);
  gimp_help_set_help_data(check_button, _("automatic update using the referred frame"), NULL);
  gtk_widget_show(check_button);

  g_object_set_data(G_OBJECT(check_button), OBJ_DATA_KEY_ATTW, attw);

  g_signal_connect (G_OBJECT (check_button), "toggled",
                    G_CALLBACK (p_attw_auto_update_toggle_update_callback),
                    &attw->gfx_tab[img_idx].auto_update);

  /* the framenr label  */
  label = gtk_label_new ("000000");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 1);
  gtk_widget_show (label);
  attw->gfx_tab[img_idx].framenr_label = label;

  /* the framenr label  */
  label = gtk_label_new ("mm:ss:msec");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 1);
  gtk_widget_show (label);
  attw->gfx_tab[img_idx].frametime_label = label;


  gtk_widget_show(vbox2);
  gtk_widget_show (event_box);
  gtk_table_attach ( GTK_TABLE (table), event_box, col_start, col_end, row, row+1, GTK_FILL, 0, 0, 0);

}  /* end p_create_and_attach_pv_widgets */


/* -----------------------------------
 * p_create_and_attach_att_arr_widgets
 * -----------------------------------
 * creates widgets for attribute array values (enable, from, to duration)
 * and attaches them to the specified table in the specified row.
 *
 * Notes: attribute values for the from and to spinbuttons are multiplied by 100
 *        because the internal representation uses the value 1.0
 *        for 100 percent.
 */
static void
p_create_and_attach_att_arr_widgets(const char *row_title
   , GapStbAttrWidget *attw
   , GtkWidget *table
   , gint row
   , gint column
   , gint att_type_idx
   , gdouble lower_constraint
   , gdouble upper_constraint
   , gdouble step_increment
   , gdouble page_increment
   , gdouble page_size
   , gint    digits
   , const char *enable_tooltip
   , gboolean   *att_arr_enable_ptr
   , const char *from_tooltip
   , gdouble    *att_arr_value_from_ptr
   , const char *to_tooltip
   , gdouble    *att_arr_value_to_ptr
   , const char *dur_tooltip
   , gint32     *att_arr_value_dur_ptr
  )
{
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *check_button;
  GtkObject *adj;
  GtkWidget *spinbutton;
  gint      col;

  col = column;

  /* enable label */
  label = gtk_label_new(row_title);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE (table), label, col, col+1, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  col++;

  /* enable check button */
  check_button = gtk_check_button_new_with_label (" ");
  attw->att_rows[att_type_idx].enable_toggle = check_button;
  gtk_table_attach ( GTK_TABLE (table), check_button, col, col+1, row, row+1, GTK_FILL, 0, 0, 0);
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button), *att_arr_enable_ptr);

  }
  gimp_help_set_help_data(check_button, enable_tooltip, NULL);
  gtk_widget_show(check_button);

  g_object_set_data(G_OBJECT(check_button), OBJ_DATA_KEY_ATTW, attw);

  g_signal_connect (G_OBJECT (check_button), "toggled",
                    G_CALLBACK (p_attw_enable_toggle_update_callback),
                    att_arr_enable_ptr);


  col++;

  /* from start label */
  button = gtk_button_new_with_label(_("Start:"));
  attw->att_rows[att_type_idx].button_from = button;
  gtk_table_attach(GTK_TABLE (table), button, col, col+1, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
  gimp_help_set_help_data(button, _("Reset to: defaults, "
                                    "use modifyer keys CTRL, ALT for alternative defaults. "
                                    "SHIFT resets to initial value"), NULL);
  gtk_widget_show(button);

  g_object_set_data(G_OBJECT(button), OBJ_DATA_KEY_ATTW, attw);
  g_signal_connect (G_OBJECT (button), "button_press_event",
                    G_CALLBACK (p_attw_start_button_clicked_callback),
                    (gpointer)att_type_idx);

  col++;

  /* the From value spinbutton */
  adj = gtk_adjustment_new ( *att_arr_value_from_ptr * CONVERT_TO_100PERCENT
                           , lower_constraint
                           , upper_constraint
                           , step_increment
                           , page_increment
                           , page_size
                           );
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 1, digits);
  attw->att_rows[att_type_idx].spinbutton_from_adj = adj;
  attw->att_rows[att_type_idx].spinbutton_from = spinbutton;

  gtk_widget_show (spinbutton);
  gtk_table_attach (GTK_TABLE (table), spinbutton, col, col+1, row, row+1,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_set_size_request (spinbutton, 80, -1);
  gimp_help_set_help_data (spinbutton, from_tooltip, NULL);

  g_object_set_data(G_OBJECT(adj), OBJ_DATA_KEY_ATTW, attw);
  g_signal_connect (G_OBJECT (adj), "value_changed",
                      G_CALLBACK (p_attw_gdouble_adjustment_callback),
                      att_arr_value_from_ptr);

  col++;

  /* to label */
  button = gtk_button_new_with_label(_("End:"));
  attw->att_rows[att_type_idx].button_to = button;
  gtk_table_attach(GTK_TABLE (table), button, col, col+1, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
  gimp_help_set_help_data(button, _("Reset to: defaults, "
                                    "use modifyer keys CTRL, ALT for alternative defaults. "
                                    "SHIFT resets to initial value"), NULL);
  gtk_widget_show(button);

  g_object_set_data(G_OBJECT(button), OBJ_DATA_KEY_ATTW, attw);
  g_signal_connect (G_OBJECT (button), "button_press_event",
                    G_CALLBACK (p_attw_end_button_clicked_callback),
                    (gpointer)att_type_idx);

  col++;

  /* the To value spinbutton */
  adj = gtk_adjustment_new ( *att_arr_value_to_ptr * CONVERT_TO_100PERCENT
                           , lower_constraint
                           , upper_constraint
                           , step_increment
                           , page_increment
                           , page_size
                           );
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 1, digits);
  attw->att_rows[att_type_idx].spinbutton_to_adj = adj;
  attw->att_rows[att_type_idx].spinbutton_to = spinbutton;

  gtk_widget_show (spinbutton);
  gtk_table_attach (GTK_TABLE (table), spinbutton, col, col+1, row, row+1,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_set_size_request (spinbutton, 80, -1);
  gimp_help_set_help_data (spinbutton, to_tooltip, NULL);

  g_object_set_data(G_OBJECT(adj), OBJ_DATA_KEY_ATTW, attw);
  g_signal_connect (G_OBJECT (adj), "value_changed",
                      G_CALLBACK (p_attw_gdouble_adjustment_callback),
                      att_arr_value_to_ptr);


  col++;

  /* Frames Duration label */
  button = gtk_button_new_with_label(_("Frames:"));
  attw->att_rows[att_type_idx].button_dur = button;
  gtk_table_attach(GTK_TABLE (table), button, col, col+1, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
  gimp_help_set_help_data(button, _("Copy this number of frames to all enabled rows"), NULL);
  gtk_widget_show(button);

  g_object_set_data(G_OBJECT(button), OBJ_DATA_KEY_ATTW, attw);
  g_signal_connect (G_OBJECT (button), "button_press_event",
                    G_CALLBACK (p_attw_dur_button_clicked_callback),
                    (gpointer)att_type_idx);

  col++;

  /* the Duration value spinbutton */
  adj = gtk_adjustment_new ( *att_arr_value_dur_ptr
                           , 0
                           , 999999
                           , 1
                           , 10
                           , 10
                           );
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 1, 0);
  attw->att_rows[att_type_idx].spinbutton_dur_adj = adj;
  attw->att_rows[att_type_idx].spinbutton_dur = spinbutton;

  gtk_widget_show (spinbutton);
  gtk_table_attach (GTK_TABLE (table), spinbutton, col, col+1, row, row+1,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_set_size_request (spinbutton, 80, -1);
  gimp_help_set_help_data (spinbutton, _("Number of frames (duration of transition from start to end value)"), NULL);

  g_object_set_data(G_OBJECT(adj), OBJ_DATA_KEY_ATTW, attw);
  g_signal_connect (G_OBJECT (adj), "value_changed",
                      G_CALLBACK (p_attw_duration_adjustment_callback),
                      att_arr_value_dur_ptr);

  col++;

  /* the time duration label  */
  label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE(table), label, col, col+1, row, row+1);
  gtk_widget_show (label);
  attw->att_rows[att_type_idx].dur_time_label = label;

}  /* end p_create_and_attach_att_arr_widgets */


/* --------------------------------
 * gap_story_attw_properties_dialog
 * --------------------------------
 */
GtkWidget *
gap_story_attw_properties_dialog (GapStbAttrWidget *attw)
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *check_button;
  gint       row;
  GapStbTabWidgets *tabw;

  if(attw == NULL) { return (NULL); }
  if(attw->attw_prop_dialog != NULL) { return(NULL); }   /* is already open */

  tabw = (GapStbTabWidgets *)attw->tabw;
  if(tabw == NULL) { return (NULL); }

  if(attw->stb_elem_bck)
  {
    dlg = gimp_dialog_new (_("Transition Attributes"), "gap_story_attr_properties"
                         ,NULL, 0
                         ,gimp_standard_help_func, GAP_STORY_ATTR_PROP_HELP_ID

                         ,GIMP_STOCK_RESET, GAP_STORY_ATT_RESPONSE_RESET
                         ,GTK_STOCK_CLOSE,  GTK_RESPONSE_CLOSE
                         ,NULL);
  }
  else
  {
    dlg = gimp_dialog_new (_("Transition Attributes"), "gap_story_attr_properties"
                         ,NULL, 0
                         ,gimp_standard_help_func, GAP_STORY_ATTR_PROP_HELP_ID

                         ,GTK_STOCK_CLOSE,  GTK_RESPONSE_CLOSE
                         ,NULL);
  }

  attw->attw_prop_dialog = dlg;

  g_signal_connect (G_OBJECT (dlg), "response",
                    G_CALLBACK (p_attw_prop_response),
                    attw);


  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlg)->vbox), main_vbox);

  /*  the frame  */
  frame = gimp_frame_new (_("Properties"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (15, 10, FALSE);
  attw->master_table = table;
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  row = 0;
  /* the fit size label  */
  label = gtk_label_new (_("FitSize:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE(table), label, 0, 1, row, row+1);
  gtk_widget_show (label);

  /* the fit width check button */
  check_button = gtk_check_button_new_with_label (_("Width"));
  attw->fit_width_toggle = check_button;
  gtk_table_attach ( GTK_TABLE (table), check_button, 1, 2, row, row+1, GTK_FILL, 0, 0, 0);
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button)
      , attw->stb_elem_refptr->att_fit_width);

  }
  gimp_help_set_help_data(check_button, _("scale width of frame to fit master width"), NULL);
  gtk_widget_show(check_button);

  g_object_set_data(G_OBJECT(check_button), OBJ_DATA_KEY_ATTW, attw);

  g_signal_connect (G_OBJECT (check_button), "toggled",
                    G_CALLBACK (p_attw_enable_toggle_update_callback),
                    &attw->stb_elem_refptr->att_fit_width);


  /* the fit height check button */
  check_button = gtk_check_button_new_with_label (_("Height"));
  attw->fit_height_toggle = check_button;
  gtk_table_attach ( GTK_TABLE (table), check_button, 2, 3, row, row+1, GTK_FILL, 0, 0, 0);
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button)
      , attw->stb_elem_refptr->att_fit_height);

  }
  gimp_help_set_help_data(check_button, _("scale height of frame to fit master height"), NULL);
  gtk_widget_show(check_button);

  g_object_set_data(G_OBJECT(check_button), OBJ_DATA_KEY_ATTW, attw);

  g_signal_connect (G_OBJECT (check_button), "toggled",
                    G_CALLBACK (p_attw_enable_toggle_update_callback),
                    &attw->stb_elem_refptr->att_fit_height);


  /* the keep proportions check button */
  check_button = gtk_check_button_new_with_label (_("Keep Proportion"));
  attw->keep_proportions_toggle = check_button;
  gtk_table_attach ( GTK_TABLE (table), check_button, 3, 5, row, row+1, GTK_FILL, 0, 0, 0);
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button)
      , attw->stb_elem_refptr->att_keep_proportions);

  }
  gimp_help_set_help_data(check_button, _("ON: keep proportions at scaling. "
                                          " (this may result in black borders)"
                                          "OFF: allow changes of image proportions at scaling"), NULL);
  gtk_widget_show(check_button);

  g_object_set_data(G_OBJECT(check_button), OBJ_DATA_KEY_ATTW, attw);

  g_signal_connect (G_OBJECT (check_button), "toggled",
                    G_CALLBACK (p_attw_enable_toggle_update_callback),
                    &attw->stb_elem_refptr->att_keep_proportions);



  row++;

  {
    gint att_type_idx;
    gint col = 0;

    att_type_idx = GAP_STB_ATT_TYPE_OPACITY;
    p_create_and_attach_att_arr_widgets(_("Opacity:")
      , attw
      , table
      , row
      , col
      , att_type_idx
      , 0.0      /* lower constraint for the from/to values */
      , 100.0    /* upper constraint for the from/to values */
      , 1.0      /* step increment   for the from/to values  */
      , 10.0     /* page increment   for the from/to values */
      , 10.0     /* page size        for the from/to values */
      , 0        /* digits for the from/to values */
      , _("ON: Enable opacity settings")
      , &attw->stb_elem_refptr->att_arr_enable[att_type_idx]
      , _("opacity value for the first handled frame "
          "where 100 is fully opaque, 0 is fully transparent")
      , &attw->stb_elem_refptr->att_arr_value_from[att_type_idx]
      , _("opacity value for the last handled frame"
          "where 100 is fully opaque, 0 is fully transparent")
      , &attw->stb_elem_refptr->att_arr_value_to[att_type_idx]
      , _("number of frames")
      , &attw->stb_elem_refptr->att_arr_value_dur[att_type_idx]
      );

    row++;
    att_type_idx = GAP_STB_ATT_TYPE_MOVE_X;
    p_create_and_attach_att_arr_widgets(_("Move X:")
      , attw
      , table
      , row
      , col
      , att_type_idx
      , -1000.0    /* lower constraint for the from/to values */
      ,  1000.0    /* upper constraint for the from/to values */
      , 1.0        /* step increment   for the from/to values  */
      , 10.0       /* page increment   for the from/to values */
      , 10.0       /* page size        for the from/to values */
      , 2          /* digits for the from/to values */
      , _("ON: Enable move horizontal settings")
      , &attw->stb_elem_refptr->att_arr_enable[att_type_idx]
      , _("move horizontal value for the first handled frame "
          " where 0.0 is centered, 100.0 is outside right, -100.0 is outside left)")
      , &attw->stb_elem_refptr->att_arr_value_from[att_type_idx]
      , _("move horizontal value for the last handled frame "
          " where 0.0 is centered, 100.0 is outside right, -100.0 is outside left)")
      , &attw->stb_elem_refptr->att_arr_value_to[att_type_idx]
      , _("number of frames")
      , &attw->stb_elem_refptr->att_arr_value_dur[att_type_idx]
      );


    row++;
    att_type_idx = GAP_STB_ATT_TYPE_MOVE_Y;
    p_create_and_attach_att_arr_widgets(_("Move Y:")
      , attw
      , table
      , row
      , col
      , att_type_idx
      , -1000.0    /* lower constraint for the from/to values */
      ,  1000.0    /* upper constraint for the from/to values */
      , 1.0        /* step increment   for the from/to values  */
      , 10.0       /* page increment   for the from/to values */
      , 10.0       /* page size        for the from/to values */
      , 2          /* digits for the from/to values */
      , _("ON: Enable move vertical settings")
      , &attw->stb_elem_refptr->att_arr_enable[att_type_idx]
      , _("move vertical value for the first handled frame "
          " where 0.0 is centered, 100.0 is outside at bottom, -100.0 is outside at top)")
      , &attw->stb_elem_refptr->att_arr_value_from[att_type_idx]
      , _("move vertical value for the last handled frame "
          " where 0.0 is centered, 100.0 is outside at bottom, -100.0 is outside at top)")
      , &attw->stb_elem_refptr->att_arr_value_to[att_type_idx]
      , _("number of frames")
      , &attw->stb_elem_refptr->att_arr_value_dur[att_type_idx]
      );

    row++;
    att_type_idx = GAP_STB_ATT_TYPE_ZOOM_X;
    p_create_and_attach_att_arr_widgets(_("Scale Width:")
      , attw
      , table
      , row
      , col
      , att_type_idx
      , 0.0    /* lower constraint for the from/to values */
      , 1000.0 /* upper constraint for the from/to values */
      , 1.0    /* step increment   for the from/to values  */
      , 10.0   /* page increment   for the from/to values */
      , 10.0   /* page size        for the from/to values */
      , 2      /* digits for the from/to values */
      , _("ON: Enable scale width settings")
      , &attw->stb_elem_refptr->att_arr_enable[att_type_idx]
      , _("scale width value for the first handled frame"
          " where 100 is 1:1, 50 is half, 200 is double width")
      , &attw->stb_elem_refptr->att_arr_value_from[att_type_idx]
      , _("scale width value for the last handled frame"
          " where 100 is 1:1, 50 is half, 200 is double width")
      , &attw->stb_elem_refptr->att_arr_value_to[att_type_idx]
      , _("number of frames")
      , &attw->stb_elem_refptr->att_arr_value_dur[att_type_idx]
      );

    row++;
    att_type_idx = GAP_STB_ATT_TYPE_ZOOM_Y;
    p_create_and_attach_att_arr_widgets(_("Scale Height:")
      , attw
      , table
      , row
      , col
      , att_type_idx
      , -1000.0    /* lower constraint for the from/to values */
      ,  1000.0    /* upper constraint for the from/to values */
      , 1.0        /* step increment   for the from/to values  */
      , 10.0       /* page increment   for the from/to values */
      , 10.0       /* page size        for the from/to values */
      , 2          /* digits for the from/to values */
      , _("ON: Enable scale height settings")
      , &attw->stb_elem_refptr->att_arr_enable[att_type_idx]
      , _("scale height value for the first handled frame"
          " where 100 is 1:1, 50 is half, 200 is double height")
      , &attw->stb_elem_refptr->att_arr_value_from[att_type_idx]
      , _("scale height value for the last handled frame"
          " where 100 is 1:1, 50 is half, 200 is double height")
      , &attw->stb_elem_refptr->att_arr_value_to[att_type_idx]
      , _("number of frames")
      , &attw->stb_elem_refptr->att_arr_value_dur[att_type_idx]
      );

  }


  row++;

  {
    GtkWidget *gfx_table;

    gfx_table = gtk_table_new (1, 3, FALSE);
    gtk_widget_show (gfx_table);

    gtk_table_attach ( GTK_TABLE (table), gfx_table, 1, 8, row, row+1, GTK_FILL, 0, 0, 0);

    /* the graphical preview(s) of the attributes at start and end */
    p_create_and_attach_pv_widgets(attw
       , gfx_table
       , 1   /* row */
       , 0   /* col_start */
       , 1   /* col_end */
       , 0   /* img_idx */
      );
    label = gtk_label_new (" ");
    gtk_widget_show (label);
    gtk_table_attach ( GTK_TABLE (gfx_table), label, 1, 2, 1, 1+1, GTK_FILL, 0, 0, 0);

    p_create_and_attach_pv_widgets(attw
       , gfx_table
       , 1   /* row */
       , 2   /* col_start */
       , 3   /* col_end */
       , 1   /* img_idx */
      );
  }


  row++;


  /* the comment label */
  label = gtk_label_new (_("Comment:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE(table), label, 0, 1, row, row+1);
  gtk_widget_show (label);


  /* the comment entry */
  entry = gtk_entry_new ();
  gtk_widget_set_size_request(entry, ATTW_COMMENT_WIDTH, -1);
  if(attw->stb_elem_refptr)
  {
    if(attw->stb_elem_refptr->comment)
    {
      if(attw->stb_elem_refptr->comment->orig_src_line)
      {
        gtk_entry_set_text(GTK_ENTRY(entry), attw->stb_elem_refptr->comment->orig_src_line);
      }
    }
  }
  gtk_table_attach_defaults (GTK_TABLE(table), entry, 1, 8, row, row+1);
  g_signal_connect(G_OBJECT(entry), "changed",
                     G_CALLBACK(p_attw_comment_entry_update_cb),
                     attw);
  gtk_widget_show (entry);
  attw->comment_entry = entry;


  p_attw_update_sensitivity(attw);

  /*  Show the main containers  */
  gtk_widget_show (main_vbox);

  return(dlg);
}  /* end gap_story_attw_properties_dialog */


/* ----------------------------------------
 * gap_story_att_stb_elem_properties_dialog
 * ----------------------------------------
 */
void
gap_story_att_stb_elem_properties_dialog ( GapStbTabWidgets *tabw
                                     , GapStoryElem *stb_elem
                                     , GapStoryBoard *stb_dst)
{
  GapStbAttrWidget *attw;
  gint ii;

  /* check if already open */
  for(attw=tabw->attw; attw!=NULL; attw=(GapStbAttrWidget *)attw->next)
  {
    if(attw->stb_elem_refptr->story_id == stb_elem->story_id)
    {
      if(attw->attw_prop_dialog)
      {
        /* Properties for the selected element already open
         * bring the window to front
         */
        gtk_window_present(GTK_WINDOW(attw->attw_prop_dialog));
        return ;
      }
      /* we found a dead element (that is already closed)
       * reuse that element to open a new clip properties dialog window
       */
      break;
    }
  }

  if(attw==NULL)
  {
    attw = g_new(GapStbAttrWidget ,1);
    attw->next = tabw->attw;
    tabw->attw = attw;
    attw->stb_elem_bck = NULL;
  }
  if(attw->stb_elem_bck)
  {
    gap_story_elem_free(&attw->stb_elem_bck);
  }
  attw->stb_elem_bck = NULL;
  attw->attw_prop_dialog = NULL;
  attw->stb_elem_refptr = stb_elem;
  attw->stb_refptr = stb_dst;
  attw->sgpp = tabw->sgpp;
  attw->tabw = tabw;
  attw->go_timertag = -1;
  for (ii=0; ii < GAP_STB_ATT_GFX_ARRAY_MAX; ii++)
  {
    attw->gfx_tab[ii].auto_update = FALSE;
    attw->gfx_tab[ii].orig_layer_record_type = GAP_STBREC_VID_UNKNOWN;
    attw->gfx_tab[ii].orig_layer_local_framenr = -1;
    attw->gfx_tab[ii].orig_layer_seltrack = -1;
    attw->gfx_tab[ii].orig_layer_filename = NULL;
  }

  attw->gfx_tab[1].auto_update = FALSE;
  attw->close_flag = FALSE;

  if(stb_elem)
  {
    attw->stb_elem_bck = gap_story_elem_duplicate(stb_elem);
    if(stb_elem->comment)
    {
      if(stb_elem->comment->orig_src_line)
      {
        attw->stb_elem_bck->comment = gap_story_elem_duplicate(stb_elem->comment);
      }
    }
  }


  attw->attw_prop_dialog = gap_story_attw_properties_dialog(attw);
  if(attw->attw_prop_dialog)
  {
    gtk_widget_show(attw->attw_prop_dialog);

    /* initial rendering (must be done after pview widgets are realised) */
    p_render_gfx(attw, 0);
    p_render_gfx(attw, 1);

  }
}  /* end gap_story_att_stb_elem_properties_dialog */




/* ----------------------------------
 * gap_story_att_fw_properties_dialog
 * ----------------------------------
 */
void
gap_story_att_fw_properties_dialog (GapStbFrameWidget *fw)
{
  GapStbTabWidgets *tabw;

  if(fw == NULL) { return; }
  if(fw->stb_elem_refptr == NULL)  { return; }

  /* type check, this dialog handles only transition and fit size attributes */
  if (fw->stb_elem_refptr->record_type == GAP_STBREC_ATT_TRANSITION)
  {
    tabw = (GapStbTabWidgets *)fw->tabw;
    if(tabw == NULL)  { return; }

    gap_story_att_stb_elem_properties_dialog(tabw, fw->stb_elem_refptr, fw->stb_refptr);
  }
}  /* end gap_story_att_fw_properties_dialog */


