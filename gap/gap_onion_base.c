/* gap_onion_base.c   procedures
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
 * version 1.3.16c;  2003.07.08   hof: created (as extract of the gap_onion_worker.c module)
 */

/* SYTEM (UNIX) includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

/* #include <locale.h> */
#include <gap-intl.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <gap_match.h>
#include <gap_lib.h>
#include <gap_layer_copy.h>

#include <gap_onion_base.h>

extern int gap_debug;


/* ============================================================================
 * p_mark_as_onionlayer
 *   set onion layer parasite (store timestamp and tattoo)
 *   tattoos are unique identifiers within an image
 *   and remain unique over sessions -- if saved in xcf format.
 * ============================================================================
 */
void
p_mark_as_onionlayer(gint32 layer_id)
{
  t_onion_parasite_data *l_parasite_data;
  GimpParasite          *l_parasite;

  if(gap_debug) printf("p_mark_as_onionlayer: START\n");

  l_parasite_data = g_malloc(sizeof(t_onion_parasite_data));
  l_parasite_data->timestamp = time(0);
  l_parasite_data->tattoo = gimp_layer_get_tattoo(layer_id);
  if(gap_debug) printf("p_mark_as_onionlayer: tattoo is: %d\n", (int)l_parasite_data->tattoo);

  l_parasite = gimp_parasite_new(GAP_ONION_PARASITE_NAME,
                                 GIMP_PARASITE_PERSISTENT,
                                 sizeof(t_onion_parasite_data),
                                 l_parasite_data);
  if(l_parasite)
  {
    gimp_drawable_parasite_attach(layer_id, l_parasite);
    gimp_parasite_free(l_parasite);
  }
}       /* end p_mark_as_onionlayer */


/* ============================================================================
 * p_check_is_onion_layer
 *   check for onion layer parasite and tattoo.
 *   (if the user made a copy of an onion layer, the copy has an onionparasite
 *    but the originals parasitedata.tattoo will not match with the layers tattoo.
 *    therefore the copy is not identified as onion layer.)
 * returns TRUE if layer is an original onion layer.
 * ============================================================================
 */
gint32
p_check_is_onion_layer(gint32 layer_id)
{
   gint           l_found;
   t_onion_parasite_data *l_parasite_data;
   GimpParasite  *l_parasite;

   if(gap_debug) printf("p_check_is_onion_layer: START layer_id %d\n", (int)layer_id);

   l_found = FALSE;
   l_parasite = gimp_drawable_parasite_find(layer_id, GAP_ONION_PARASITE_NAME);
   if (l_parasite)
   {
      l_parasite_data = (t_onion_parasite_data *)l_parasite->data;
      if(gap_debug) printf("p_check_is_onion_layer: tattoo is: %d\n", (int)l_parasite_data->tattoo);

      if (l_parasite_data->tattoo == gimp_layer_get_tattoo(layer_id))
      {
        l_found = TRUE;
        if(gap_debug) printf("p_check_is_onion_layer: ONION_LAYER_FOUND layer_id %d\n", (int)layer_id);
      }
      gimp_parasite_free(l_parasite);
   }

   return l_found;
}       /* end p_check_is_onion_layer */



/* ============================================================================
 * p_delete_image_immediate
 *    delete image (with workaround to ensure that most of the
 *    allocatd memory is freed)
 * ============================================================================
 */
void
p_delete_image_immediate (gint32 image_id)
{
    if(gap_debug) printf("p_delete_image_immediate: SCALED down to 2x2 id = %d (workaround for gimp_image-delete problem)\n", (int)image_id);

    gimp_image_scale(image_id, 2, 2);

    gimp_image_undo_enable(image_id); /* clear undo stack */
    gimp_image_delete(image_id);
}       /* end  p_delete_image_immediate */


/* ============================================================================
 * p_get_merged_layer
 *    merge visible layer an return layer_id of the resulting merged layer.
 *    (with workaround, for empty images return transparent layer)
 * ============================================================================
 */
gint32
p_get_merged_layer(gint32 image_id, GimpMergeType mergemode)
{
  GimpImageBaseType l_type;
  guint   l_width, l_height;
  gint32  l_layer_id;

  /* get info about the image */
  l_width  = gimp_image_width(image_id);
  l_height = gimp_image_height(image_id);
  l_type   = gimp_image_base_type(image_id);

  l_type   = (l_type * 2); /* convert from GimpImageBaseType to GimpImageType */

  /* add 2 full transparent dummy layers at top
   * (because gimp_image_merge_visible_layers complains
   * if there are less than 2 visible layers)
   */
  l_layer_id = gimp_layer_new(image_id, "dummy",
                                 l_width, l_height,  l_type,
                                 0.0,       /* Opacity full transparent */
                                 0);        /* NORMAL */
  gimp_image_add_layer(image_id, l_layer_id, 0);

  l_layer_id = gimp_layer_new(image_id, "dummy",
                                 10, 10,  l_type,
                                 0.0,       /* Opacity full transparent */
                                 0);        /* NORMAL */
  gimp_image_add_layer(image_id, l_layer_id, 0);

  return gimp_image_merge_visible_layers (image_id, mergemode);
}       /* end p_get_merged_layer */


/* ============================================================================
 * p_onionskin_visibility
 *    set visibility of all onion layer(s) in the current image.
 * ============================================================================
 */
gint
p_onionskin_visibility(gint32 image_id, gint visi_mode)
{
#define VISIBILTY_UNSET  -4444
  gint32     *l_layers_list;
  gint        l_nlayers;
  gint        l_idx;
  gint        l_is_onion;
  gint        l_visible;
  gint32      l_layer_id;

  if(gap_debug)
  {
     printf("p_onionskin_visibility: START visi_mode: %d\n", (int)visi_mode);
     printf("  image_ID: %d\n", (int)image_id);
  }

  l_visible = VISIBILTY_UNSET;
  if(visi_mode == GAP_ONION_VISI_TRUE)
  {
    l_visible = TRUE;
  }
  if(visi_mode == GAP_ONION_VISI_FALSE)
  {
    l_visible = TRUE;
  }

  l_layers_list = gimp_image_get_layers(image_id, &l_nlayers);
  if(l_layers_list)
  {
    for(l_idx=0;l_idx < l_nlayers;l_idx++)
    {
      l_layer_id = l_layers_list[l_idx];
      l_is_onion = p_check_is_onion_layer(l_layer_id);

      if(l_is_onion)
      {
        if (l_visible == VISIBILTY_UNSET)
        {
           if(gimp_layer_get_visible(l_layer_id)) { l_visible = FALSE; }
           else                                   { l_visible = TRUE; }
        }

        /* set visibility  */
        if(gap_debug) printf("layer_id %d  visibility: %d\n", (int)l_layer_id ,(int)l_visible);
        gimp_layer_set_visible(l_layer_id, l_visible);
      }
    }
    g_free(l_layers_list);
  }
  return 0;
}       /* end p_onionskin_visibility */



/* ============================================================================
 * p_onionskin_delete
 *    remove onion layer(s) from the current image.
 * ============================================================================
 */
gint
p_onionskin_delete(gint32 image_id)
{
  gint32     *l_layers_list;
  gint        l_nlayers;
  gint        l_idx;
  gint        l_is_onion;

  if(gap_debug)
  {
     printf("p_onionskin_delete: START\n");
     printf("  image_ID: %d\n", (int)image_id);
  }

  l_layers_list = gimp_image_get_layers(image_id, &l_nlayers);
  if(gap_debug) printf("p_onionskin_delete: l_nlayers = %d\n", (int)l_nlayers);
  if(l_layers_list)
  {
    for(l_idx=0;l_idx < l_nlayers;l_idx++)
    {
      if(gap_debug) printf("p_onionskin_delete: l_idx = %d\n", (int)l_idx);
      l_is_onion = p_check_is_onion_layer(l_layers_list[l_idx]);

      if(l_is_onion)
      {
        /* remove onion layer from source */
        gimp_image_remove_layer(image_id, l_layers_list[l_idx]);
      }
    }
    g_free(l_layers_list);
  }
  if(gap_debug) printf("p_onionskin_delete: END\n");
  return 0;
}       /* end p_onionskin_delete */


/* ============================================================================
 * p_onionskin_apply
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
 * The functionpointers
 *     fptr_add_img_to_cache
 *     fptr_find_frame_in_img_cache
 *
 *   are used for caching the handled images.
 *   this is useful when onionskin layers are created for more than one frame
 *   at once. for processing single frames it is OK to pass NULL pointers.
 *
 * returns   value >= 0 if all is ok
 *           (or -1 on error)
 * ============================================================================
 */
gint
p_onionskin_apply(gpointer gpp
             , gint32 image_id
             , t_video_info *vin_ptr
             , long   ainfo_curr_frame_nr
             , long   ainfo_first_frame_nr
             , long   ainfo_last_frame_nr
             , char  *ainfo_basename
             , char  *ainfo_extension
             , t_fptr_add_img_to_cache        fptr_add_img_to_cache
             , t_fptr_find_frame_in_img_cache fptr_find_frame_in_img_cache
             , gboolean use_cache)
{
  gint32        l_onr;
  gint32        l_ign;
  gint32        l_idx;
  gint32        l_frame_nr;
  gint32        l_tmp_image_id;
  gint32        l_is_onion;
  gint32        l_layerstack;
  char         *l_new_filename;
  char         *l_name;
  gint32        l_layer_id;
  gint32        l_new_layer_id;
  gint32     *l_layers_list;
  gint        l_nlayers;
  gdouble     l_opacity;
  char       *l_layername;


  if(gap_debug)
  {
     printf("p_onionskin_apply: START\n");
     printf("  num_olayers: %d\n",   (int)vin_ptr->num_olayers);
     printf("  ref_delta: %d\n",     (int)vin_ptr->ref_delta);
     printf("  ref_cycle: %d\n",     (int)vin_ptr->ref_cycle);
     printf("  stack_pos: %d\n",     (int)vin_ptr->stack_pos);
     printf("  stack_top: %d\n",     (int)vin_ptr->stack_top);
     printf("  opacity: %f\n",       (float)vin_ptr->opacity);
     printf("  opacity_delta: %f\n", (float)vin_ptr->opacity_delta);
     printf("  ignore_botlayers: %d\n", (int)vin_ptr->ignore_botlayers);
     printf("  image_ID: %d\n",      (int)image_id);
     printf("  use_cache: %d\n",     (int)use_cache);
     printf("  asc_opacity: %d\n",   (int)vin_ptr->asc_opacity);

     printf("  ainfo_curr_frame_nr: %d\n",     (int)ainfo_curr_frame_nr);
     printf("  ainfo_first_frame_nr: %d\n",     (int)ainfo_first_frame_nr);
     printf("  ainfo_last_frame_nr: %d\n",     (int)ainfo_last_frame_nr);
     printf("  ainfo_basename: %s\n",     ainfo_basename);
     printf("  ainfo_extension: %s\n",    ainfo_extension);

  }

  /* delete onion layers (if there are old ones) */
  p_onionskin_delete(image_id);

  l_layers_list = gimp_image_get_layers(image_id, &l_nlayers);
  if(vin_ptr->stack_top)
  {
    l_layerstack = CLAMP(vin_ptr->stack_pos, 0, l_nlayers);
  }
  else
  {
    l_layerstack = CLAMP((l_nlayers - vin_ptr->stack_pos),0 ,l_nlayers);
  }
  if(l_layers_list) { g_free(l_layers_list); l_layers_list = NULL;}

  /* create new onion layer(s) */
  l_opacity = vin_ptr->opacity;
  l_new_filename = NULL;
  l_frame_nr = ainfo_curr_frame_nr;
  for(l_onr=1; l_onr <= vin_ptr->num_olayers; l_onr++)
  {
    /* find out reference frame number */
    if(vin_ptr->asc_opacity)
    {
       /* process far neigbours first to give them the highest configured opacity value */
       l_frame_nr = ainfo_curr_frame_nr
                  + (vin_ptr->ref_delta * ((1+ vin_ptr->num_olayers) - l_onr));
    }
    else
    {
       /* process near neigbours first to give them the highest configured opacity value */
       l_frame_nr = ainfo_curr_frame_nr
                  + (vin_ptr->ref_delta * l_onr);
    }


    if(!vin_ptr->ref_cycle)
    {
      if((l_frame_nr < ainfo_first_frame_nr)
      || (l_frame_nr > ainfo_last_frame_nr))
      {
         break;  /* fold back cycle turned off */
      }
    }
    if (l_frame_nr < ainfo_first_frame_nr)
    {
      l_frame_nr = ainfo_last_frame_nr +1 - (ainfo_first_frame_nr - l_frame_nr);
      if (l_frame_nr < ainfo_first_frame_nr)
      {
        break;  /* stop on multiple fold back cycle */
      }
    }
    if (l_frame_nr > ainfo_last_frame_nr)
    {
      l_frame_nr = ainfo_first_frame_nr -1 + (l_frame_nr - ainfo_last_frame_nr);
      if (l_frame_nr > ainfo_last_frame_nr)
      {
         break;  /* stop on multiple fold back cycle */
      }
    }

    l_tmp_image_id = -1;
    l_layer_id = -1;
    if(use_cache)
    {
      if(fptr_find_frame_in_img_cache != NULL)
      {
         (*fptr_find_frame_in_img_cache)(gpp, l_frame_nr, &l_tmp_image_id, &l_layer_id);
      }

      if (l_tmp_image_id == image_id)
      {
        /* never use the same image as source and destination,
         * if references point to same image, always force
         * loading a 2.nd copy for the merge
         */
        l_tmp_image_id = -1;
      }
    }

    if(l_tmp_image_id < 0)
    {
      /* frame is not available in the cache */
      if(gap_debug) printf("p_onionskin_apply: frame is NOT available in the CACHE\n");
      /* build the frame name */
      if(l_new_filename != NULL) g_free(l_new_filename);
      l_new_filename = p_alloc_fname(ainfo_basename,
                                          l_frame_nr,
                                          ainfo_extension);
      /* load referenced frame */
      l_tmp_image_id = p_load_image(l_new_filename);
      if(l_tmp_image_id < 0)
         return -1;

      /* dont waste time and memory for undo in noninteracive processing
       * of the src frames
       */
      gimp_image_undo_enable(l_tmp_image_id); /* clear undo stack */
      gimp_image_undo_disable(l_tmp_image_id); /*  NO Undo */
    }
    else
    {
      if(gap_debug) printf("p_onionskin_apply: frame is AVAILABLE in the CACHE l_tmp_image_id: %d\n", (int)l_tmp_image_id);
    }


    if(l_layer_id < 0)
    {
      /* there was no merged layer in the cache, so we must
       * merge layers now
       */
      if(gap_debug) printf("p_onionskin_apply: layer is NOT available in the CACHE\n");

      /* set some layers invisible
       * a) ignored bottomlayer(s)
       * b) select_mode dependent: layers where layername does not match select-string
       * c) all onion layers
       */
      l_layers_list = gimp_image_get_layers(l_tmp_image_id, &l_nlayers);
      for(l_ign=0, l_idx=l_nlayers -1; l_idx >= 0;l_idx--)
      {
        l_layer_id = l_layers_list[l_idx];
        l_layername = gimp_layer_get_name(l_layer_id);


        l_is_onion = p_check_is_onion_layer(l_layer_id);

        if((l_ign <  vin_ptr->ignore_botlayers)
        || (FALSE == p_match_layer( l_idx
                                  , l_layername
                                  , &vin_ptr->select_string[0]
                                  , vin_ptr->select_mode
                                  , vin_ptr->select_case
                                  , vin_ptr->select_invert
                                  , l_nlayers
                                  , l_layer_id)
           )
        || (l_is_onion))
        {
          gimp_layer_set_visible(l_layer_id, FALSE);
        }

        g_free (l_layername);

        if(!l_is_onion)
        {
          /* exclude other onion layers from counting ignored layers
           */
          l_ign++;
        }
      }

      /* merge visible layers (clip at image size) */
      l_layer_id = p_get_merged_layer(l_tmp_image_id, GIMP_CLIP_TO_IMAGE);
    }
    else
    {
      if(gap_debug) printf("p_onionskin_apply: layer is AVAILABLE in the CACHE l_layer_id: %d\n", (int)l_layer_id);
    }

    /* COPY or MOVE the resulting merged layer to current image image_id */
    if(use_cache)
    {
      /* when using image cache we must copy the merged layer
       * but first update the cache, so we can reuse the merged layer
       * when processing the next frame
       */
      if(fptr_add_img_to_cache != NULL)
      {
         (*fptr_add_img_to_cache)(gpp, l_frame_nr, l_tmp_image_id, l_layer_id);
      }
    }

    if(l_layer_id >= 0)
    {
      gint         l_src_offset_x;    /* layeroffsets as they were in src_image */
      gint         l_src_offset_y;

      /* copy the layer to dest image image_id */
      l_new_layer_id = p_my_layer_copy (image_id
                                     ,l_layer_id
                                     ,CLAMP(l_opacity, 0.0, 100.0)
                                     ,0           /* NORMAL GimpLayerModeEffects */
                                     ,&l_src_offset_x
                                     ,&l_src_offset_y
                                     );

      if(! gimp_drawable_has_alpha(l_new_layer_id))
      {
         /* have to add alpha channel */
         gimp_layer_add_alpha(l_new_layer_id);
      }



       if(gap_debug) printf("ONL:p_onionskin_apply  l_onr:%d, framenr:%d, layerstack:%d opacity:%f\n"
       , (int)l_onr
       , (int)l_frame_nr
       , (int)l_layerstack
       , (float)l_opacity
       );


      /* add the layer to current frame at desired stackposition  */
      gimp_image_add_layer (image_id, l_new_layer_id, l_layerstack);
      gimp_layer_set_offsets(l_new_layer_id, l_src_offset_x, l_src_offset_y);

      /* set layername */
      l_name = g_strdup_printf(_("onionskin_%06d"), (int) l_frame_nr);
      gimp_layer_set_name(l_new_layer_id, l_name);
      g_free(l_name);


      /* Set parasite or tattoo */
      p_mark_as_onionlayer(l_new_layer_id);
      /* gimp_layer_set_opacity(l_new_layer_id, l_opacity); */
    }


    if(l_layers_list != NULL)  { g_free (l_layers_list); l_layers_list = NULL;}

    if(!use_cache)
    {
      /* destroy the tmp image */
      p_delete_image_immediate(l_tmp_image_id);
    }

    /* opacitiy for next onion layer (reduced by delta percentage) */
    l_opacity = CLAMP((l_opacity * vin_ptr->opacity_delta / 100.0), 0.0 , 100.0);

    /* stackposition for next onion layer */
    l_layerstack++;

    if(l_new_filename) { g_free(l_new_filename); l_new_filename = NULL; };

  }
  if(gap_debug) printf("p_onionskin_apply: END\n\n");

  return 0;
}       /* end p_onionskin_apply */