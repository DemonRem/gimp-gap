/* wr_color_hue_saturation.c
 * 2002.Jan.01 hof (Wolfgang Hofer)
 *
 *  Wrapper Plugin for GIMP Hue Saturation tool
 *
 * Warning: This is just a QUICK HACK to enable
 *          Animated Filterapply in conjunction with the
 *          GIMP Hue Saturation Tool.
 *
 *  It provides a primitive Dialog Interface where you
 *  can call the Hue Saturation Tool
 *
 *  Further it has an Interface to 'Run_with_last_values'
 *  and an Iterator Procedure.
 *  (This enables the 'Animated Filter Call' from
 *   the GAP's Menu Filters->Filter all Layers)
 *
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

/* Revision history
 *  (2004/01/15)  v1.3.24a  hof: adapted for gimp-1.3.x and gtk+2.2 API
 *  (2002/10/27)  v1.03  hof: - appear in menu (for filtermacro)
 *  (2002/01/01)  v1.02  hof: - created
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gap-intl.h"

/* Defines */
#define PLUG_IN_NAME        "plug_in_wr_huesat"
#define PLUG_IN_IMAGE_TYPES "RGB*, GRAY*"
#define PLUG_IN_AUTHOR      "Wolfgang Hofer (hof@gimp.org)"
#define PLUG_IN_COPYRIGHT   "Wolfgang Hofer"
#define PLUG_IN_DESCRIPTION "Wrapper call for GIMP Hue Saturation Color Tool"

#define PLUG_IN_ITER_NAME       "plug_in_wr_huesat_Iterator"
#define PLUG_IN_DATA_ITER_FROM  "plug_in_wr_huesat_ITER_FROM"
#define PLUG_IN_DATA_ITER_TO    "plug_in_wr_huesat_ITER_TO"
#define PLUG_IN_HELP_ID         "plug-in-wr-huesat"



typedef struct
{
  gint32    hue_range;
  gdouble   hue_offset;
  gdouble   lightness;
  gdouble   saturation;
} wr_huesat_val_t;


typedef struct _WrDialog WrDialog;

struct _WrDialog
{
  gint          run;
  gint          show_progress;
  GtkWidget       *shell;
  GtkWidget       *radio_master;
  GtkWidget       *radio_red;
  GtkWidget       *radio_yellow;
  GtkWidget       *radio_green;
  GtkWidget       *radio_cyan;
  GtkWidget       *radio_blue;
  GtkWidget       *radio_magenta;
  wr_huesat_val_t *vals;
};


WrDialog *do_dialog (wr_huesat_val_t *);
static void  query (void);
static void run(const gchar *name
           , gint nparams
           , const GimpParam *param
           , gint *nreturn_vals
           , GimpParam **return_vals);

/* Global Variables */
GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,   /* init_proc  */
  NULL,   /* quit_proc  */
  query,  /* query_proc */
  run     /* run_proc   */
};


gint  gap_debug = 0;  /* 0.. no debug, 1 .. print debug messages */

/* --------------
 * procedures
 * --------------
 */



/*
 * Delta Calculations for the iterator
 */
static void
p_delta_gdouble (gdouble  *val,
		 gdouble   val_from,
		 gdouble   val_to,
		 gint32   total_steps,
		 gdouble  current_step)
{
    gdouble     delta;

    if(total_steps < 1) return;

    delta = ((gdouble)(val_to - val_from) / (gdouble)total_steps) * ((gdouble)total_steps - current_step);
    *val  = val_from + delta;
}

static void
p_delta_gint32 (gint32  *val,
		 gint32   val_from,
		 gint32   val_to,
		 gint32   total_steps,
		 gdouble  current_step)
{
    gdouble     delta;

    if(total_steps < 1) return;

    delta = ((gdouble)(val_to - val_from) / (gdouble)total_steps) * ((gdouble)total_steps - current_step);
    *val  = val_from + delta;
}

/* ============================================================================
 * p_gimp_hue_saturation
 *
 * ============================================================================
 */

gint32
p_gimp_hue_saturation (gint32 drawable_id,
		   gint32 hue_range,
		   gdouble hue_offset,
		   gdouble lightness,
		   gdouble saturation
                   )
{
   static char     *l_procname = "gimp_hue_saturation";
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_procname,
                                 &nreturn_vals,
                                 GIMP_PDB_DRAWABLE,  drawable_id,
                                 GIMP_PDB_INT32,     hue_range,
                                 GIMP_PDB_FLOAT,     hue_offset,
                                 GIMP_PDB_FLOAT,     lightness,
                                 GIMP_PDB_FLOAT,     saturation,
                                  GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      gimp_destroy_params(return_vals, nreturn_vals);
      return (0);
   }
   gimp_destroy_params(return_vals, nreturn_vals);
   printf("Error: PDB call of %s failed status:%d\n", l_procname, (int)return_vals[0].data.d_status);
   return(-1);
}	/* end p_gimp_hue_saturation */

static void
p_run_huesat_tool(gint32 drawable_id, wr_huesat_val_t *cuvals)
{
  if(gap_debug)
  {
     printf("p_run_huesat_tool: drawable_id :%d\n", (int)drawable_id);
     printf("p_run_huesat_tool:  hue_range:%d\n", (int)cuvals->hue_range);
     printf("p_run_huesat_tool:  hue_offset:%f\n", (float)cuvals->hue_offset);
     printf("p_run_huesat_tool:  lightness:%f\n", (float)cuvals->lightness);
     printf("p_run_huesat_tool:  saturation:%f\n", (float)cuvals->saturation);
  }
  p_gimp_hue_saturation(drawable_id
                       , cuvals->hue_range
                       , cuvals->hue_offset
                       , cuvals->lightness
                       , cuvals->saturation
                       );

}

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] = {
                  { GIMP_PDB_INT32,      "run_mode", "Interactive, non-interactive"},
                  { GIMP_PDB_IMAGE,      "image", "Input image" },
                  { GIMP_PDB_DRAWABLE,   "drawable", "Input drawable (must be a layer without layermask)"},
                  { GIMP_PDB_INT32,      "hue_range", "Range of affected hues: ALL_HUES (0), RED_HUES(1), YELLOW_HUES(2), GREEN_HUES(3), CYAN_HUES(4), BLUE_HUES(5), MAGENTA_HUES(6)"},
                  { GIMP_PDB_FLOAT,      "hue_offset", "Hue Offset uin degrees (-180 <= hue_offset <= 180"},
                  { GIMP_PDB_FLOAT,      "lightness", "lightness Modification (-100 <= lightness <= 100"},
                  { GIMP_PDB_FLOAT,      "saturation", "saturation Modification (-100 <= saturation <= 100"},
  };
  static int nargs = sizeof(args) / sizeof(args[0]);

  static GimpParamDef return_vals[] =
  {
    { GIMP_PDB_DRAWABLE, "the_drawable", "the handled drawable" }
  };
  static int nreturn_vals = sizeof(return_vals) / sizeof(return_vals[0]);


  static GimpParamDef args_iter[] =
  {
    {GIMP_PDB_INT32, "run_mode", "non-interactive"},
    {GIMP_PDB_INT32, "total_steps", "total number of steps (# of layers-1 to apply the related plug-in)"},
    {GIMP_PDB_FLOAT, "current_step", "current (for linear iterations this is the layerstack position, otherwise some value inbetween)"},
    {GIMP_PDB_INT32, "len_struct", "length of stored data structure with id is equal to the plug_in  proc_name"},
  };
  static int nargs_iter = sizeof(args_iter) / sizeof(args_iter[0]);

  static GimpParamDef *return_iter = NULL;
  static int nreturn_iter = 0;

  /* the actual installation of the bend plugin */
  gimp_install_procedure (PLUG_IN_NAME,
                          PLUG_IN_DESCRIPTION,
  			 "This Plugin is a wrapper to call the GIMP Hue Saturation Color Tool (gimp_hue_saturation)"
			 " it has a simplified Dialog (without preview) where you can enter the parameters"
			 " this wrapper is useful for animated filtercalls and provides "
			 " a PDB interface that runs in GIMP_RUN_WITH_LAST_VALUES mode"
			 " and also provides an Iterator Procedure for animated calls"
                          ,
                          PLUG_IN_AUTHOR,
                          PLUG_IN_COPYRIGHT,
                          GAP_VERSION_WITH_DATE,
                          N_("<Image>/Video/Layer/Colors/Hue-Saturation..."),
                          PLUG_IN_IMAGE_TYPES,
                          GIMP_PLUGIN,
                          nargs,
                          nreturn_vals,
                          args,
                          return_vals);


  /* the installation of the Iterator extension for the bend plugin */
  gimp_install_procedure (PLUG_IN_ITER_NAME,
                          "This extension calculates the modified values for one iterationstep for the call of plug_in_curve_bend",
                          "",
                          PLUG_IN_AUTHOR,
                          PLUG_IN_COPYRIGHT,
                          GAP_VERSION_WITH_DATE,
                          NULL,    /* do not appear in menus */
                          NULL,
                          GIMP_PLUGIN,
                          nargs_iter, nreturn_iter,
                          args_iter, return_iter);
}



static void 
run(const gchar *name
           , gint nparams
           , const GimpParam *param
           , gint *nreturn_vals
           , GimpParam **return_vals)
{
  wr_huesat_val_t l_cuvals;
  WrDialog   *wcd = NULL;

  gint32    l_image_id = -1;
  gint32    l_drawable_id = -1;
  gint32    l_handled_drawable_id = -1;

  /* Get the runmode from the in-parameters */
  GimpRunMode run_mode = param[0].data.d_int32;

  /* status variable, use it to check for errors in invocation usualy only
     during non-interactive calling */
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  /*always return at least the status to the caller. */
  static GimpParam values[2];


  INIT_I18N();

  /* initialize the return of the status */
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  values[1].type = GIMP_PDB_DRAWABLE;
  values[1].data.d_int32 = -1;
  *nreturn_vals = 2;
  *return_vals = values;

  /* the Iterator Stuff */
  if (strcmp (name, PLUG_IN_ITER_NAME) == 0)
  {
     gint32  len_struct;
     gint32  total_steps;
     gdouble current_step;
     wr_huesat_val_t   cval;                  /* current values while iterating */
     wr_huesat_val_t   cval_from, cval_to;    /* start and end values */

     /* Iterator procedure for animated calls is usually called from
      * "plug_in_gap_layers_run_animfilter"
      * (always run noninteractive)
      */
     if ((run_mode == GIMP_RUN_NONINTERACTIVE) && (nparams == 4))
     {
       total_steps  =  param[1].data.d_int32;
       current_step =  param[2].data.d_float;
       len_struct   =  param[3].data.d_int32;

       if(len_struct == sizeof(cval))
       {
         /* get _FROM and _TO data,
          * This data was stored by plug_in_gap_layers_run_animfilter
          */
          gimp_get_data(PLUG_IN_DATA_ITER_FROM, &cval_from);
          gimp_get_data(PLUG_IN_DATA_ITER_TO,   &cval_to);
          memcpy(&cval, &cval_from, sizeof(cval));

          p_delta_gint32 (&cval.hue_range,  cval_from.hue_range,  cval_to.hue_range,  total_steps, current_step);
          p_delta_gdouble(&cval.hue_offset, cval_from.hue_offset, cval_to.hue_offset, total_steps, current_step);
          p_delta_gdouble(&cval.lightness,  cval_from.lightness,  cval_to.lightness,  total_steps, current_step);
          p_delta_gdouble(&cval.saturation, cval_from.saturation, cval_to.saturation, total_steps, current_step);

          gimp_set_data(PLUG_IN_NAME, &cval, sizeof(cval));
       }
       else status = GIMP_PDB_CALLING_ERROR;
     }
     else status = GIMP_PDB_CALLING_ERROR;

     values[0].data.d_status = status;
     return;
  }



  /* get image and drawable */
  l_image_id = param[1].data.d_int32;
  l_drawable_id = param[2].data.d_drawable;

  if(status == GIMP_PDB_SUCCESS)
  {
    /* how are we running today? */
    switch (run_mode)
     {
      case GIMP_RUN_INTERACTIVE:
        /* Initial values */
        l_cuvals.hue_range = 0;
        l_cuvals.hue_offset = 0;
        l_cuvals.lightness = 0;
        l_cuvals.saturation = 0;

        /* Get information from the dialog */
        wcd = do_dialog(&l_cuvals);
        wcd->show_progress = TRUE;
        break;

      case GIMP_RUN_NONINTERACTIVE:
        /* check to see if invoked with the correct number of parameters */
        if (nparams >= 7)
        {
           wcd = g_malloc (sizeof (WrDialog));
           wcd->run = TRUE;
           wcd->show_progress = FALSE;

           l_cuvals.hue_range  = param[3].data.d_int32;
           l_cuvals.hue_offset = param[4].data.d_float;
           l_cuvals.lightness  = param[5].data.d_float;
           l_cuvals.saturation = param[6].data.d_float;
        }
        else
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
        break;

      case GIMP_RUN_WITH_LAST_VALS:
        wcd = g_malloc (sizeof (WrDialog));
        wcd->run = TRUE;
        wcd->show_progress = TRUE;
        /* Possibly retrieve data from a previous run */
        gimp_get_data (PLUG_IN_NAME, &l_cuvals);
        break;

      default:
        break;
    }
  }

  if (wcd == NULL)
  {
    status = GIMP_PDB_EXECUTION_ERROR;
  }

  if (status == GIMP_PDB_SUCCESS)
  {
     /* Run the main function */
     if(wcd->run)
     {
	gimp_image_undo_group_start (l_image_id);
	p_run_huesat_tool(l_drawable_id, &l_cuvals);
        l_handled_drawable_id = l_drawable_id;
	gimp_image_undo_group_end (l_image_id);

	/* Store variable states for next run */
	if (run_mode == GIMP_RUN_INTERACTIVE)
	{
	  gimp_set_data(PLUG_IN_NAME, &l_cuvals, sizeof(l_cuvals));
	}
     }
     else
     {
       status = GIMP_PDB_EXECUTION_ERROR;       /* dialog ended with cancel button */
     }

     /* If run mode is interactive, flush displays, else (script) don't
        do it, as the screen updates would make the scripts slow */
     if (run_mode != GIMP_RUN_NONINTERACTIVE)
     {
       gimp_displays_flush ();
     }
  }
  values[0].data.d_status = status;
  values[1].data.d_int32 = l_handled_drawable_id;   /* return the id of handled layer */

}	/* end run */


/*
 * DIALOG and callback stuff
 */


static void
radio_callback(GtkWidget *wgt, gpointer user_data)
{
  WrDialog *wcd;

  if(gap_debug) printf("radio_callback: START\n");
  wcd = (WrDialog*)user_data;
  if(wcd != NULL)
  {
    if(wcd->vals != NULL)
    {
       if(wgt == wcd->radio_magenta)  { wcd->vals->hue_range = 6; }
       if(wgt == wcd->radio_blue)     { wcd->vals->hue_range = 5; }
       if(wgt == wcd->radio_cyan)     { wcd->vals->hue_range = 4; }
       if(wgt == wcd->radio_green)    { wcd->vals->hue_range = 3; }
       if(wgt == wcd->radio_yellow)   { wcd->vals->hue_range = 2; }
       if(wgt == wcd->radio_red)      { wcd->vals->hue_range = 1; }
       if(wgt == wcd->radio_master)   { wcd->vals->hue_range = 0; }

       if(gap_debug) printf("radio_callback: value: %d\n", (int)wcd->vals->hue_range);
    }
  }
}



/* ---------------------------------
 * wr_huesat_response
 * ---------------------------------
 */
static void
wr_huesat_response (GtkWidget *widget,
                 gint       response_id,
                 WrDialog *wcd)
{
  GtkWidget *dialog;

  switch (response_id)
  {
    case GTK_RESPONSE_OK:
      if(wcd)
      {
	if (GTK_WIDGET_VISIBLE (wcd->shell))
	  gtk_widget_hide (wcd->shell);

	wcd->run = TRUE;
      }

    default:
      dialog = NULL;
      if(wcd)
      {
        dialog = wcd->shell;
	if(dialog)
	{
          wcd->shell = NULL;
          gtk_widget_destroy (dialog);
	}
      }
      gtk_main_quit ();
      break;
  }
}  /* end wr_huesat_response */


WrDialog *
do_dialog (wr_huesat_val_t *cuvals)
{
  WrDialog *wcd;
  GtkWidget  *vbox;

  GtkWidget *dialog1;
  GtkWidget *dialog_vbox1;
  GtkWidget *frame1;
  GtkWidget *hbox1;
  GtkWidget *vbox1;
  GtkWidget *label1;
  GSList *vbox1_group = NULL;
  GtkWidget *radiobutton1;
  GtkWidget *radiobutton2;
  GtkWidget *radiobutton3;
  GtkWidget *radiobutton4;
  GtkWidget *radiobutton5;
  GtkWidget *radiobutton6;
  GtkWidget *radiobutton7;
  GtkWidget *table1;
  GtkWidget *label2;
  GtkWidget *label3;
  GtkWidget *label4;
  GtkObject *spinbutton_sat_adj;
  GtkWidget *spinbutton_sat;
  GtkObject *spinbutton_light_adj;
  GtkWidget *spinbutton_light;
  GtkObject *spinbutton_hue_adj;
  GtkWidget *spinbutton_hue;
  GtkWidget *dialog_action_area1;

  /* Init UI  */
  gimp_ui_init ("wr_curves", FALSE);


  /*  The dialog1  */
  wcd = g_malloc (sizeof (WrDialog));
  wcd->run = FALSE;
  wcd->vals = cuvals;

  /*  The dialog1 and main vbox  */
  dialog1 = gimp_dialog_new (_("Hue-Saturation"), "hue_saturation_wrapper",
                               NULL, 0,
			       gimp_standard_help_func, PLUG_IN_HELP_ID,

                               GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                               GTK_STOCK_OK,     GTK_RESPONSE_OK,
                               NULL);

  wcd->shell = dialog1;


  /*
   * g_object_set_data (G_OBJECT (dialog1), "dialog1", dialog1);
   * gtk_window_set_title (GTK_WINDOW (dialog1), _("dialog1"));
   */


  g_signal_connect (G_OBJECT (dialog1), "response",
		      G_CALLBACK (wr_huesat_response),
		      wcd);

  /* the vbox */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog1)->vbox), vbox,
		      TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  dialog_vbox1 = GTK_DIALOG (dialog1)->vbox;
  g_object_set_data (G_OBJECT (dialog1), "dialog_vbox1", dialog_vbox1);
  gtk_widget_show (dialog_vbox1);



  /* the frame */
  frame1 = gimp_frame_new (_("Hue / Lightness / Saturation Adjustments "));

  gtk_widget_show (frame1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), frame1, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame1), 2);

  hbox1 = gtk_hbox_new (FALSE, 0);

  gtk_widget_show (hbox1);
  gtk_container_add (GTK_CONTAINER (frame1), hbox1);
  gtk_container_set_border_width (GTK_CONTAINER (hbox1), 4);

  vbox1 = gtk_vbox_new (FALSE, 0);

  gtk_widget_show (vbox1);
  gtk_box_pack_start (GTK_BOX (hbox1), vbox1, TRUE, TRUE, 0);


  /* Hue Mode the label */
  label1 = gtk_label_new (_("Hue Mode:"));

  gtk_widget_show (label1);
  gtk_box_pack_start (GTK_BOX (vbox1), label1, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);


  /* Hue Mode the radio buttons */
  radiobutton1 = gtk_radio_button_new_with_label (vbox1_group, _("Master"));
  vbox1_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radiobutton1));
  gtk_widget_show (radiobutton1);
  gtk_box_pack_start (GTK_BOX (vbox1), radiobutton1, FALSE, FALSE, 0);

  radiobutton2 = gtk_radio_button_new_with_label (vbox1_group, _("Red"));
  vbox1_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radiobutton2));
  gtk_widget_show (radiobutton2);
  gtk_box_pack_start (GTK_BOX (vbox1), radiobutton2, FALSE, FALSE, 0);

  radiobutton3 = gtk_radio_button_new_with_label (vbox1_group, _("Yellow"));
  vbox1_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radiobutton3));
  gtk_widget_show (radiobutton3);
  gtk_box_pack_start (GTK_BOX (vbox1), radiobutton3, FALSE, FALSE, 0);

  radiobutton4 = gtk_radio_button_new_with_label (vbox1_group, _("Green"));
  vbox1_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radiobutton4));
  gtk_widget_show (radiobutton4);
  gtk_box_pack_start (GTK_BOX (vbox1), radiobutton4, FALSE, FALSE, 0);

  radiobutton5 = gtk_radio_button_new_with_label (vbox1_group, _("Cyan"));
  vbox1_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radiobutton5));
  gtk_widget_show (radiobutton5);
  gtk_box_pack_start (GTK_BOX (vbox1), radiobutton5, FALSE, FALSE, 0);

  radiobutton6 = gtk_radio_button_new_with_label (vbox1_group, _("Blue"));
  vbox1_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radiobutton6));
  gtk_widget_show (radiobutton6);
  gtk_box_pack_start (GTK_BOX (vbox1), radiobutton6, FALSE, FALSE, 0);

  radiobutton7 = gtk_radio_button_new_with_label (vbox1_group, _("Magenta"));
  vbox1_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radiobutton7));
  gtk_widget_show (radiobutton7);
  gtk_box_pack_start (GTK_BOX (vbox1), radiobutton7, FALSE, FALSE, 0);


  /* table1 for spinbuttons  */
  table1 = gtk_table_new (4, 2, FALSE);
  gtk_widget_show (table1);
  gtk_box_pack_start (GTK_BOX (hbox1), table1, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (table1), 4);

  label2 = gtk_label_new (_("Hue:"));
  gtk_widget_show (label2);
  gtk_table_attach (GTK_TABLE (table1), label2, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);

  label3 = gtk_label_new (_("Lightness:"));
  gtk_widget_show (label3);
  gtk_table_attach (GTK_TABLE (table1), label3, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label3), 0, 0.5);

  label4 = gtk_label_new (_("Saturation:"));
  gtk_widget_show (label4);
  gtk_table_attach (GTK_TABLE (table1), label4, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label4), 0, 0.5);

  spinbutton_sat_adj = gtk_adjustment_new (0, -100, 100, 1, 10, 10);
  spinbutton_sat = gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton_sat_adj), 1, 0);
  gtk_widget_show (spinbutton_sat);
  gtk_table_attach (GTK_TABLE (table1), spinbutton_sat, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  spinbutton_light_adj = gtk_adjustment_new (0, -100, 100, 1, 10, 10);
  spinbutton_light = gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton_light_adj), 1, 0);
  gtk_widget_show (spinbutton_light);
  gtk_table_attach (GTK_TABLE (table1), spinbutton_light, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  spinbutton_hue_adj = gtk_adjustment_new (0, -180, 180, 1, 10, 10);
  spinbutton_hue = gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton_hue_adj), 1, 0);
  gtk_widget_show (spinbutton_hue);
  gtk_table_attach (GTK_TABLE (table1), spinbutton_hue, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  dialog_action_area1 = GTK_DIALOG (dialog1)->action_area;
  gtk_widget_show (dialog_action_area1);
  gtk_container_set_border_width (GTK_CONTAINER (dialog_action_area1), 10);





  wcd->radio_master  = radiobutton1;
  wcd->radio_red     = radiobutton2;
  wcd->radio_yellow  = radiobutton3;
  wcd->radio_green   = radiobutton4;
  wcd->radio_cyan    = radiobutton5;
  wcd->radio_blue    = radiobutton6;
  wcd->radio_magenta = radiobutton7;


  /* signals */
  g_signal_connect (G_OBJECT (wcd->radio_master),  "clicked",  G_CALLBACK (radio_callback), wcd);
  g_signal_connect (G_OBJECT (wcd->radio_red),     "clicked",  G_CALLBACK (radio_callback), wcd);
  g_signal_connect (G_OBJECT (wcd->radio_yellow),  "clicked",  G_CALLBACK (radio_callback), wcd);
  g_signal_connect (G_OBJECT (wcd->radio_green),   "clicked",  G_CALLBACK (radio_callback), wcd);
  g_signal_connect (G_OBJECT (wcd->radio_cyan),    "clicked",  G_CALLBACK (radio_callback), wcd);
  g_signal_connect (G_OBJECT (wcd->radio_blue),    "clicked",  G_CALLBACK (radio_callback), wcd);
  g_signal_connect (G_OBJECT (wcd->radio_magenta), "clicked",  G_CALLBACK (radio_callback), wcd);

  g_signal_connect (G_OBJECT (spinbutton_hue_adj),   "value_changed",  G_CALLBACK (gimp_double_adjustment_update), &cuvals->hue_offset);
  g_signal_connect (G_OBJECT (spinbutton_light_adj), "value_changed",  G_CALLBACK (gimp_double_adjustment_update), &cuvals->lightness);
  g_signal_connect (G_OBJECT (spinbutton_sat_adj),   "value_changed",  G_CALLBACK (gimp_double_adjustment_update), &cuvals->saturation);


  gtk_widget_show (dialog1);

  gtk_main ();
  gdk_flush ();

  return wcd;
}
