/* gap_frontends_main.c
 * 1999.11.20 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Package
 *
 * This Module contains:
 * - MAIN of GAP_Plugins that are frontends to external programs
 *     those external programs are not available for all
 *     operating systems.
 *
 * - query   registration of GAP Procedures (Video Menu) in the PDB
 * - run     invoke the selected GAP procedure by its PDB name
 * 
 *
 * GAP provides Animation Functions for the gimp,
 * working on a series of Images stored on disk in gimps .xcf Format.
 *
 * Frames are Images with naming convention like this:
 * Imagename_<number>.<ext>
 * Example:   snoopy_0001.xcf, snoopy_0002.xcf, snoopy_0003.xcf
 *
 * if gzip is installed on your system you may optional
 * use gziped xcf frames with extensions ".xcfgz"
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

static char *gap_main_version =  "1.1.29b; 2000/11/25";

/* revision history:
 * gimp    1.1.29b; 2000/11/25  hof: use gap lock procedures, update e-mail adress + main version
 * gimp    1.1.11b; 1999/11/20  hof: added gap_decode_xanim, fixed typo in mpeg encoder menu path
 *                                   based on parts that were found in gap_main.c before.
 */
 
/* SYTEM (UNIX) includes */ 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* GIMP includes */
#include "gtk/gtk.h"
#include "config.h"
#include "libgimp/stdplugins-intl.h"
#include "libgimp/gimp.h"

/* GAP includes */
#include "gap_lib.h"
#include "gap_mpege.h"
#include "gap_decode_xanim.h"
#include "gap_arr_dialog.h"

/* ------------------------
 * global gap DEBUG switch
 * ------------------------
 */

/* int gap_debug = 1; */    /* print debug infos */
/* int gap_debug = 0; */    /* 0: dont print debug infos */

int gap_debug = 0;

static void query(void);
static void run(char *name, int nparam, GimpParam *param,
                int *nretvals, GimpParam **retvals);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  query, /* query_proc */
  run,   /* run_proc */
};

MAIN ()

static void
query ()
{
  static GimpParamDef args_xanim[] =
  {
    {GIMP_PDB_INT32, "run_mode", "Interactive"},
    {GIMP_PDB_IMAGE, "image", "(unused)"},
    {GIMP_PDB_DRAWABLE, "drawable", "(unused)"},
  };

  static GimpParamDef args_xanim_ext[] =
  {
    {GIMP_PDB_INT32, "run_mode", "Interactive"},
  };

  static GimpParamDef args_mpege[] =
  {
    {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (one of the Anim Frames)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
  };

  static GimpParamDef *return_vals = NULL;
  static int nreturn_vals = 0;


  gimp_install_procedure("plug_in_gap_xanim_decode",
			 "This plugin calls xanim to split any video to anim frames. (xanim exporting edition must be installed on your system)",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Split Video to Frames/Any XANIM readable..."),
			 NULL,
			 GIMP_PLUGIN,
			 G_N_ELEMENTS (args_xanim), nreturn_vals,
			 args_xanim, return_vals);

  gimp_install_procedure("extension_gap_xanim_decode",
			 "This plugin calls xanim to split any video to anim frames. (xanim exporting edition must be installed on your system)",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Toolbox>/Xtns/Split Video to Frames/Any XANIM readable..."),
			 NULL,
			 GIMP_EXTENSION,
			 G_N_ELEMENTS (args_xanim_ext), nreturn_vals,
			 args_xanim_ext, return_vals);

  gimp_install_procedure("plug_in_gap_mpeg_encode",
			 "This plugin calls mpeg_encode to convert anim frames to MPEG1, or just generates a param file for mpeg_encode. (mpeg_encode must be installed on your system)",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Encode/MPEG1..."),
			 "*",
			 GIMP_PLUGIN,
			 G_N_ELEMENTS (args_mpege), nreturn_vals,
			 args_mpege, return_vals);


  gimp_install_procedure("plug_in_gap_mpeg2encode",
			 "This plugin calls mpeg2encode to convert anim frames to MPEG1 or MPEG2, or just generates a param file for mpeg2encode. (mpeg2encode must be installed on your system)",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Encode/MPEG2..."),
			 "*",
			 GIMP_PLUGIN,
			 G_N_ELEMENTS (args_mpege), nreturn_vals,
			 args_mpege, return_vals);


}	/* end query */



static void
run (char    *name,
     int      n_params,
     GimpParam  *param,
     int     *nreturn_vals,
     GimpParam **return_vals)
{
  const char *l_env;
  
  char        l_extension[32];
  static GimpParam values[2];
  GimpRunModeType run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint32     image_id;
  gint32     lock_image_id;
  gint32     nr;
   
  gint32     l_rc;

  *nreturn_vals = 1;
  *return_vals = values;
  nr = 0;
  l_rc = 0;


  l_env = g_getenv("GAP_DEBUG");
  if(l_env != NULL)
  {
    if((*l_env != 'n') && (*l_env != 'N')) gap_debug = 1;
  }

  run_mode = param[0].data.d_int32;
  image_id = -1;
  lock_image_id = image_id;

  if(gap_debug) fprintf(stderr, "\n\ngap_main: debug name = %s\n", name);

  if (strcmp (name, "extension_gap_xanim_decode") != 0)
  {
    image_id = param[1].data.d_image;
    lock_image_id = image_id;

    /* check for locks */
    if(p_gap_lock_is_locked(lock_image_id, run_mode))
    {
         status = GIMP_PDB_EXECUTION_ERROR;
         values[0].type = GIMP_PDB_STATUS;
         values[0].data.d_status = status;
         return ;
    }


    /* set LOCK on current image (for all gap_plugins) */
    p_gap_lock_set(lock_image_id);
  }
  
  if (run_mode == GIMP_RUN_NONINTERACTIVE) {
    INIT_I18N();
  } else {
    INIT_I18N_UI();
  }

  if ((strcmp (name, "plug_in_gap_xanim_decode") == 0)
  ||  (strcmp (name, "extension_gap_xanim_decode") == 0))
  {
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
          status = GIMP_PDB_CALLING_ERROR;
          /* planed: define non interactive PARAMS */
      }

      if (status == GIMP_PDB_SUCCESS)
      {
        /* planed: define non interactive PARAMS */

        l_rc = gap_xanim_decode(run_mode /* more PARAMS */);

      }
  }
  else if (strcmp (name, "plug_in_gap_mpeg_encode") == 0)
  {
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        if (n_params != 3)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
        else
        {
          /* planed: define non interactive PARAMS */
          l_extension[sizeof(l_extension) -1] = '\0';
        }
      }

      if (status == GIMP_PDB_SUCCESS)
      {

        image_id    = param[1].data.d_image;
        /* planed: define non interactive PARAMS */

        l_rc = gap_mpeg_encode(run_mode, image_id, MPEG_ENCODE /* more PARAMS */);

      }
  }
  else if (strcmp (name, "plug_in_gap_mpeg2encode") == 0)
  {
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        if (n_params != 3)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
        else
        {
          /* planed: define non interactive PARAMS */
          l_extension[sizeof(l_extension) -1] = '\0';
        }
      }

      if (status == GIMP_PDB_SUCCESS)
      {

        image_id    = param[1].data.d_image;
        /* planed: define non interactive PARAMS */

        l_rc = gap_mpeg_encode(run_mode, image_id, MPEG2ENCODE /* more PARAMS */);

      }
  }

  /* ---------- return handling --------- */

 if(l_rc < 0)
 {
    status = GIMP_PDB_EXECUTION_ERROR;
 }
 
  
 if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush();

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  if (strcmp (name, "extension_gap_xanim_decode") != 0)
  {
    /* remove LOCK on this image for all gap_plugins */
     p_gap_lock_remove(lock_image_id);
  }
}
