/* gap_base_ops.h
 * 2003.05.24 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * basic anim functions
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

/* revision history:
 * 1.3.14a  2003/05/24   hof: created (module was splitted off from gap_lib)
 */

#ifndef _GAP_BASE_OPS_H
#define _GAP_BASE_OPS_H

#include "libgimp/gimp.h"

/* Video menu basic fuctions */

gint32 gap_next(GimpRunMode run_mode, gint32 image_id);
gint32 gap_prev(GimpRunMode run_mode, gint32 image_id);
gint32 gap_first(GimpRunMode run_mode, gint32 image_id);
gint32 gap_last(GimpRunMode run_mode, gint32 image_id);
gint32 gap_goto(GimpRunMode run_mode, gint32 image_id, int nr);

gint32 gap_dup(GimpRunMode run_mode, gint32 image_id, int nr, long range_from, long range_to);
gint32 gap_del(GimpRunMode run_mode, gint32 image_id, int nr);
gint32 gap_exchg(GimpRunMode run_mode, gint32 image_id, int nr);
gint32 gap_shift(GimpRunMode run_mode, gint32 image_id, int nr, long range_from, long range_to);
gint32 gap_renumber(GimpRunMode run_mode, gint32 image_id,
            long start_frame_nr, long digits);

#endif


