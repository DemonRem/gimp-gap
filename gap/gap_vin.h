/*  gap_vin.h
 *
 *  This module handles GAP video info files (_vin.gap)
 *  _vin.gap files store global informations about an animation
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
 * version 1.3.16c; 2003/07/09  hof: support onionskin settings in video_info files
 * version 1.3.14a; 2003/05/24  hof: created (splitted off from gap_pdb_calls module)
 */

#ifndef _GAP_VIN_H
#define _GAP_VIN_H

#include "libgimp/gimp.h"

typedef struct t_video_info {
  gdouble     framerate;    /* playback rate in frames per second */
  gint32      timezoom;

  /* stuff for onionskin layers */
  gboolean onionskin_auto_enable;     /* master switch for onionskin load/save triggers */
  gboolean auto_replace_after_load;
  gboolean auto_delete_before_save;
   
   
  gint32  num_olayers;      /* Number of Onion Layers  1 .. 10 Default: 1 */
  gint32  ref_delta;        /* Reference Frame Delta:  +- 1 ... n  Default: -1 */
  gint32  ref_cycle;        /* Reference is Cycle   : TRUE/FALSE   Default: TRUE
                             *    TRUE .. last frame has frame 0 as next frame
                             */
  gint32  stack_pos;        /* Place OnionLayer(s) on Stackposition 0..n Default: 1 */
  gint32  stack_top;        /* TRUE Stack Position is relative from TOP
                             * FALSE Stack Position is relative from Bottom (Default: FALSE) */
  gdouble opacity;          /* OnionOpacity: 0.0..100.0%  Default: 50 % */
  gdouble opacity_delta;    /* OnionOpacityDelta: 0..100%  Default: 80 %
                             * (2nd Layer has 80% of 50%)
                             */
  gint32  ignore_botlayers; /* Ignore N Bottom Sourcelayers Default: 1
                             *  (0 .. Onion Layer is built from all Src Layers)
                             *  (2 .. Layers are ignored,  Background and next layer)
                             */
  gint32  select_mode;       /* Mode how to identify a layer: -1 Pattern off,  0-3 by layername 0=equal, 1=prefix, 2=suffix, 3=contains */
  gint32  select_case;
  gint32  select_invert;
  gchar   select_string[512];

  gboolean asc_opacity;    /* TRUE: the far neighbour frames have higher opacity
                            * FALSE: near neighbour frames have higher opacity (DEFAULT)
                            */
} t_video_info;


typedef struct t_textfile_lines {
   char    *line;
   gint32   line_nr;
   void    *next;
} t_textfile_lines;


char *p_alloc_video_info_name(char *basename);
int   p_set_video_info(t_video_info *vin_ptr, char *basename);
int   p_set_video_info_onion(t_video_info *vin_ptr, char *basename);
t_video_info *p_get_video_info(char *basename);

#endif