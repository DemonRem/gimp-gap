/* gap_gve_misc_util.h
 *
 *  GAP common encoder tool procedures
 *  with no dependencies to external libraries.
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
 * version 1.2.1a;  2004.05.14   hof: created
 */

#ifndef GAP_GVE_MISC_UTIL_H
#define GAP_GVE_MISC_UTIL_H

#include "libgimp/gimp.h"

#include <sys/types.h>
#include <unistd.h>



typedef struct GapGveEncAInfo {
   long         first_frame_nr;
   long         last_frame_nr;
   long         curr_frame_nr;
   long         frame_cnt;
   char         basename[1024];    /* may include path */
   char         extension[50];
   gdouble      framerate;
} GapGveEncAInfo;


typedef struct GapGveMasterEncoderStatus {                     /* nick:  */
 /* Status for monitoring video encoding progress */
  gint32 master_encoder_id;  /* typically the PID */
  gint32 total_frames;
  gint32 frames_processed;
  gint32 frames_encoded;
  gint32 frames_copied_lossless;
} GapGveMasterEncoderStatus;


/* --------------------------*/
/* PROCEDURE DECLARATIONS    */
/* --------------------------*/


void        gap_gve_misc_get_ainfo(gint32 image_ID, GapGveEncAInfo *ainfo);

void        gap_gve_misc_initGapGveMasterEncoderStatus(GapGveMasterEncoderStatus *encStatus
                                 , gint32 master_encoder_id, gint32 total_frames);
void        gap_gve_misc_do_master_encoder_progress(GapGveMasterEncoderStatus *encStatus);
gboolean    gap_gve_misc_is_master_encoder_cancel_request(GapGveMasterEncoderStatus *encStatus);

void        gap_gve_misc_get_master_encoder_progress(GapGveMasterEncoderStatus *encStatus);
void        gap_gve_misc_set_master_encoder_cancel_request(GapGveMasterEncoderStatus *encStatus, gboolean cancelRequest);


extern int gap_debug;


#endif        /* GAP_GVE_MISC_UTIL_H */
