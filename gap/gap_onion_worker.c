/* gap_onion_worker.c   procedures
 * 2003.05.22 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains GAP Onionskin Worker Procedures
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
 * version 1.3.16c; 2003.07.09   hof: splitted off gap_onion_base.c (for automatic apply)
 * version 1.3.16b; 2003.07.06   hof: bugfixes, added parameter asc_opacity
 * version 1.3.14a; 2003.05.24   hof: integration into gimp-gap-1.3.14
 * version 1.2.2a;  2001.11.24   hof: created
 */

#include <gap_onion_main.h>
extern int gap_debug;


/* ============================================================================
 * p_find_frame_in_img_cache
 *    return delete image (with workaround to ensure that most of the
 *    return -1 if nothing was found.
 * ============================================================================
 */
gint32
p_find_frame_in_img_cache(void *gpp_void
                         , gint32 framenr, gint32 *image_id, gint32 *layer_id)
{
  t_global_params *gpp;
  gint32 l_idx;
  *image_id = -1;
  *layer_id = -1;
  
  gpp = (t_global_params *)gpp_void;

  for(l_idx = 0; l_idx < MIN(gpp->cache.count, GAP_ONION_CACHE_SIZE); l_idx++)
  {
    if(framenr == gpp->cache.framenr[l_idx])
    {
      *image_id = gpp->cache.image_id[l_idx];
      *layer_id = gpp->cache.layer_id[l_idx];
      return (l_idx);
    }
  }
  return (-1);
}       /* end p_find_frame_in_img_cache */

/* ============================================================================
 * p_add_img_to_cache
 *    add image_id and layer_id to the image cache.
 *    the image cache is a list of temporary images (without display)
 *    and contains frames that were processed in the previous steps
 *    of the onionskin layer processing.
 *    - a layer_id of -1 is used, if the image was loaded, but not
 *      merged.
 *    - If the framenr is already in the cache
 *      ??
 *    - If the cache is full, the oldest image_id is deleted
 *      and removed from the cache
 *    - Do not add the current image to the cache !!
 * ============================================================================
 */
void
p_add_img_to_cache (void *gpp_void, gint32 framenr, gint32 image_id, gint32 layer_id)
{
  t_global_params *gpp;
  gint32 l_idx;

  gpp = (t_global_params *)gpp_void;

  if(gap_debug)
  {
     printf("p_add_img_to_cache: cache.count: %d)\n", (int)gpp->cache.count);
     printf("PARAMS framenr: %d image_id: %d layer_id: %d\n"
           , (int)framenr
           , (int)image_id
           , (int)layer_id
           );
  }
  for(l_idx = 0; l_idx < MIN(gpp->cache.count, GAP_ONION_CACHE_SIZE); l_idx++)
  {
    if(framenr == gpp->cache.framenr[l_idx])
    {
      /* an image with this framenr is already in the cache */
      if(gap_debug) printf("p_add_img_to_cache: framenr already in cache at index: %d)\n", (int)l_idx);

      if( gpp->cache.layer_id[l_idx] >= 0)
      {
        /* the cached image has aready merged layers */
        return;
      }
      if(layer_id >= 0)
      {
         /* update the chache with the id of the merged layer */
         gpp->cache.image_id[l_idx] = image_id;
         gpp->cache.layer_id[l_idx] = layer_id;
      }
      return;
    }
  }
  if(l_idx < GAP_ONION_CACHE_SIZE)
  {
    gpp->cache.count++;
  }
  else
  {
    if(gap_debug)
    {
      printf("p_add_img_to_cache: FULL CACHE delete oldest entry\n");
    }
    /* cache is full, so delete 1.st (oldest) entry */
    p_delete_image_immediate(gpp->cache.image_id[0]);

    /* shift all entries down one index step */
    for(l_idx = 0; l_idx <  GAP_ONION_CACHE_SIZE -1; l_idx++)
    {
      gpp->cache.framenr[l_idx] = gpp->cache.framenr[l_idx +1];
      gpp->cache.image_id[l_idx] = gpp->cache.image_id[l_idx +1];
      gpp->cache.layer_id[l_idx] = gpp->cache.layer_id[l_idx +1];
    }
    /* l_idx = GAP_ONION_CACHE_SIZE -1; */
  }

  if(gap_debug)
  {
    printf("p_add_img_to_cache: new entry ADDED at IMAGECACHE index: %d\n", (int)l_idx);
  }
  gpp->cache.framenr[l_idx] = framenr;
  gpp->cache.image_id[l_idx] = image_id;
  gpp->cache.layer_id[l_idx] = layer_id;
}       /* end p_add_img_to_cache */



/* ============================================================================
 * p_delete_img_cache
 *    delete all images in the cache, and set the cache empty.
 * ============================================================================
 */
static void
p_delete_img_cache (t_global_params *gpp)
{
  gint32 l_idx;

  if(gap_debug) printf("p_delete_image_immediate: cache.count: %d)\n", (int)gpp->cache.count);
  for(l_idx = 0; l_idx < MIN(gpp->cache.count, GAP_ONION_CACHE_SIZE); l_idx++)
  {
    p_delete_image_immediate(gpp->cache.image_id[l_idx]);
  }
  gpp->cache.count = 0;
}       /* end p_delete_img_cache */


/* ============================================================================
 * p_plug_in_gap_get_animinfo
 *      get informations about the animation
 * ============================================================================
 */
void
p_plug_in_gap_get_animinfo(gint32 image_ID, t_ainfo *ainfo)
{
   static char     *l_called_proc = "plug_in_gap_get_animinfo";
   GimpParam       *return_vals;
   int              nreturn_vals;
   gint32           l_gap_feature_level;

   l_gap_feature_level = 0;
   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_INT32, GIMP_RUN_NONINTERACTIVE,
                                 GIMP_PDB_IMAGE, image_ID,
                                 GIMP_PDB_DRAWABLE, -1,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      ainfo->first_frame_nr = return_vals[1].data.d_int32;
      ainfo->last_frame_nr =  return_vals[2].data.d_int32;
      ainfo->curr_frame_nr =  return_vals[3].data.d_int32;
      ainfo->frame_cnt =      return_vals[4].data.d_int32;
      g_snprintf(ainfo->basename, sizeof(ainfo->basename), "%s", return_vals[5].data.d_string);
      g_snprintf(ainfo->extension, sizeof(ainfo->extension), "%s", return_vals[6].data.d_string);
      ainfo->framerate = return_vals[7].data.d_float;

      return;
   }

   printf("Error: PDB call of %s failed, image_ID: %d\n", l_called_proc, (int)image_ID);

}       /* end p_plug_in_gap_get_animinfo */


/* wrappers for gap_onoin_base procedures */

gint
p_set_data_onion_cfg(t_global_params *gpp, char *key)
{
  if(gap_debug) printf("p_set_data_onion_cfg: START\n");
  
  return (p_set_video_info_onion(&gpp->vin, &gpp->ainfo.basename[0]));
}

gint
p_get_data_onion_cfg(t_global_params *gpp)
{
  t_video_info *l_vin;
  
  
  if(gap_debug) printf("p_get_data_onion_cfg: START\n");

  /* try to read configuration params */
  l_vin = p_get_video_info(&gpp->ainfo.basename[0]);
  if(l_vin)
  {
    memcpy(&gpp->vin, l_vin, sizeof(t_video_info));
    g_free(l_vin);
    return 0;
  }

  return -1;
}

gint
p_onion_visibility(t_global_params *gpp, gint visi_mode)
{
  return (p_onionskin_visibility(gpp->image_ID, visi_mode));
}


/* ============================================================================
 * p_onion_delete
 *    remove onion layer(s) from the current image.
 * ============================================================================
 */
gint
p_onion_delete(t_global_params *gpp)
{
  if(gap_debug)
  {
     printf("p_onion_delete: START\n");
     printf("  image_ID: %d\n", (int)gpp->image_ID);
  }

  p_onionskin_delete(gpp->image_ID);


  if(gap_debug) printf("p_onion_delete: END\n");
  return 0;
}       /* end p_onion_delete */


/* ============================================================================
 * p_onion_apply
 *    create or replace onion layer(s) in the current image.
 *    Onion layers do show one (or more) merged copies of previos (or next)
 *    videoframe(s).
 *    This procedure first removes onion layers (if there are any)
 *    then reads the other videoframes, merges them (according to config params)
 *    and imports the merged layer(s) as onion layer(s).
 *    Onion Layers are marked by tattoo and parasite
 *    use_chache TRUE:
 *       check if desired frame is already in the image cache we can skip loading
 *       if the cached image is already merged we can simply copy the layer.
 *       Further we update the cache with merged layer.
 *       use_cache is for processing multiple frames (speeds up remarkable)
 *    use_cache FALSE:
 *       always load frames and perform merge,
 *       but we can steal layer from the tmp_image.
 *       (faster than copy when processing a single frame only)
 *
 *
 * returns   value >= 0 if all is ok
 *           (or -1 on error)
 * ============================================================================
 */
gint
p_onion_apply(t_global_params *gpp, gboolean use_cache)
{
  gint    l_rc;


  if(gap_debug)
  {
     printf("p_onion_apply: START\n");
  }
  l_rc = p_onionskin_apply(gpp
             , gpp->image_ID
             , &gpp->vin
             , gpp->ainfo.curr_frame_nr
             , gpp->ainfo.first_frame_nr
             , gpp->ainfo.last_frame_nr
             , &gpp->ainfo.basename[0]
             , &gpp->ainfo.extension[0]
             , p_add_img_to_cache
             , p_find_frame_in_img_cache
             , use_cache
             );
  if(gap_debug) printf("p_onion_apply: END\n\n");

  return 0;
}       /* end p_onion_apply */



/* ============================================================================
 * p_onion_range
 *    Apply or delete onionskin layers in selected framerange
 * ============================================================================
 */
gint
p_onion_range(t_global_params *gpp)
{
  int    l_rc;
  gint32 l_frame_nr;
  gint32 l_step, l_begin, l_end;
  gint32 l_current_image_id;
  gint32 l_curr_frame_nr;
  gdouble    l_percentage, l_percentage_step;
  gchar  *l_new_filename;


  l_percentage = 0.0;
  l_current_image_id = gpp->image_ID;
  if(gpp->vin.ref_delta < 0)
  {
    /* for references to previous frames we step up (from low to high frame numbers) */
    l_begin = MIN(gpp->range_from, gpp->range_to);
    l_end = MAX(gpp->range_from, gpp->range_to);
    l_step = 1;
    l_percentage_step = 1.0 / ((1.0 + l_end) - l_begin);
  }
  else
  {
    /* for references to next frames we step down (from high to low frame numbers) */
    l_begin = MAX(gpp->range_from, gpp->range_to);
    l_end = MIN(gpp->range_from, gpp->range_to);
    l_step = -1;
    l_percentage_step = 1.0 / ((1.0 + l_begin) - l_end);
  }


  if(gpp->run_mode == GIMP_RUN_INTERACTIVE)
  {
    if(gpp->run == GAP_ONION_RUN_APPLY)
    {
      gimp_progress_init( _("Creating Onionskin Layers..."));
    }
    else
    {
      gimp_progress_init( _("Removing Onionskin Layers..."));
    }
  }

  /* save current image */
  l_new_filename = gimp_image_get_filename(l_current_image_id);
  l_rc = p_save_named_frame(gpp->image_ID, l_new_filename);
  if(l_rc < 0)
     return -1;

  l_curr_frame_nr = gpp->ainfo.curr_frame_nr;
  l_frame_nr = l_begin;
  while(1)
  {
    if(gap_debug) printf("p_onion_range processing frame %d\n", (int)l_frame_nr);
    gpp->ainfo.curr_frame_nr = l_frame_nr;
    if(l_new_filename != NULL) { g_free(l_new_filename); l_new_filename = NULL; }
    if (l_curr_frame_nr == l_frame_nr)
    {
       l_new_filename = gimp_image_get_filename(l_current_image_id);
       gpp->image_ID = l_current_image_id;

       /* never add the current image to the cache
        * (to prevent it from merging !!)
        */
    }
    else
    {
      /* build the frame name */
      l_new_filename = p_alloc_fname(gpp->ainfo.basename,
                                          l_frame_nr,
                                          gpp->ainfo.extension);
      /* load frame */
      gpp->image_ID =  p_load_image(l_new_filename);
      if(gpp->image_ID < 0)
         return -1;

      /* add image to cache */
      p_add_img_to_cache(gpp, l_frame_nr, gpp->image_ID, -1);
    }

    if(gpp->run == GAP_ONION_RUN_APPLY)
    {
      p_onion_apply(gpp, TRUE /* use_cache */);
    }
    else if(gpp->run == GAP_ONION_RUN_DELETE)
    {
      p_onion_delete(gpp);
    }
    else
    {
      printf("operation not implemented\n");
    }

    l_rc = p_save_named_frame(gpp->image_ID, l_new_filename);
    if(l_rc < 0)
       return -1;

    if(gpp->run_mode == GIMP_RUN_INTERACTIVE)
    {
      l_percentage += l_percentage_step;
      gimp_progress_update (l_percentage);
    }

    if(l_frame_nr == l_end)
    {
      break;
    }
    l_frame_nr += l_step;
  }

  /* clean up the cache (delete all cached temp images) */
  p_delete_img_cache(gpp);

  return 0; /* OK */
}       /* end p_onion_range */
