/*
 * Copyright (c) 2015-2016 Jean Guyomarc'h
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "war2edit.h"

static void
_mouse_coords_convert(const Editor *ed,
                      int           x,
                      int           y,
                      int          *outx,
                      int          *outy)
{
   int cx, cy, ox, oy;

   evas_object_geometry_get(ed->minimap.map, &ox, &oy, NULL, NULL);

   cx = rintf((float)(x - ox) / ed->minimap.ratio);
   cy = rintf((float)(y - oy) / ed->minimap.ratio);

   *outx = cx;
   *outy = cy;
}

static void
_mouse_down_cb(void        *data,
               Evas        *e    EINA_UNUSED,
               Evas_Object *obj  EINA_UNUSED,
               void        *info)
{
   Evas_Event_Mouse_Down *const down = info;
   Editor *const ed = data;
   int cx, cy;

   _mouse_coords_convert(ed, down->output.x, down->output.y, &cx, &cy);
   minimap_view_move(ed, cx, cy, EINA_TRUE);
}

static void
_mouse_move_cb(void        *data EINA_UNUSED,
               Evas        *e    EINA_UNUSED,
               Evas_Object *obj  EINA_UNUSED,
               void        *info EINA_UNUSED)
{
   Editor *const ed = data;
   Evas_Event_Mouse_Move *const move = info;
   int cx, cy;

   if (move->buttons & 1)
     {
        _mouse_coords_convert(ed, move->cur.output.x, move->cur.output.y, &cx, &cy);
        minimap_view_move(ed, cx, cy, EINA_TRUE);
     }
}

static void
_show_cb(void        *data,
         Evas_Object *obj      EINA_UNUSED,
         const char  *emission EINA_UNUSED,
         const char  *source   EINA_UNUSED)
{
   Editor *const ed = data;
   evas_object_show(ed->minimap.rect);
   minimap_render(ed, 0, 0, ed->pud->map_w, ed->pud->map_h);
}

static void
_hide_cb(void        *data,
         Evas_Object *obj      EINA_UNUSED,
         const char  *emission EINA_UNUSED,
         const char  *source   EINA_UNUSED)
{
   Editor *const ed = data;
   evas_object_hide(ed->minimap.rect);
}



Eina_Bool
minimap_add(Editor *ed)
{
   Evas_Object *o;
   Evas *evas;

   evas = evas_object_evas_get(ed->lay);
   o = ed->minimap.map = editor_image_new(ed->lay, NULL, 0, 0);

   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
                                  _mouse_down_cb, ed);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE,
                                  _mouse_move_cb, ed);

   /* Current view mask */
   o = ed->minimap.rect = evas_object_rectangle_add(evas);
   evas_object_smart_member_add(o, ed->lay);
   evas_object_color_set(o, 100, 100, 100, 100);
   evas_object_resize(o, 1, 1);
   evas_object_pass_events_set(o, EINA_TRUE);
   evas_object_move(o, 0, 0);
   evas_object_show(o);

   elm_layout_signal_callback_add(ed->lay, "war2edit,minimap,show", "war2edit",
                                  _show_cb, ed);
   elm_layout_signal_callback_add(ed->lay, "war2edit,minimap,hide", "war2edit",
                                  _hide_cb, ed);

   return EINA_TRUE;
}

Eina_Bool
minimap_resize(Editor *ed)
{
   unsigned int w, i;
   float ratio;
   void *ptr;
   Evas_Object *map;

   INF("Resizing minimap");

   /* Define a ratio to resize the minimap (way too small overwise) */
   switch (ed->pud->dims)
     {
      case PUD_DIMENSIONS_32_32:   ratio = 6.0f; break;
      case PUD_DIMENSIONS_64_64:   ratio = 3.0f; break;
      case PUD_DIMENSIONS_96_96:   ratio = 2.0f; break;
      case PUD_DIMENSIONS_128_128: ratio = 1.5f; break;
      default:
         CRI("ed->pud->dims is %i. This MUST NEVER happen", ed->pud->dims);
         goto fail;
     }
   ed->minimap.ratio = ratio;

   /* Colorspace width */
   w = ed->pud->map_w * 4;

   /* Allocate Iliffe vector to hold the minimap */
   ptr = (ed->minimap.data) ? ed->minimap.data[0] : NULL;
   ptr = realloc(ptr, ed->pud->map_w * ed->pud->map_h * 4);
   if (EINA_UNLIKELY(!ptr))
     {
        CRI("Failed to alloc memory");
        goto fail;
     }
   ed->minimap.data = realloc(ed->minimap.data,
                              ed->pud->map_h * sizeof(unsigned char *));
   if (EINA_UNLIKELY(!ed->minimap.data))
     {
        CRI("Failed to alloc memory");
        goto fail_free;
     }
   ed->minimap.data[0] = ptr;
   for (i = 1; i < ed->pud->map_h; ++i)
     ed->minimap.data[i] = ed->minimap.data[i - 1] + w;

   /* Get the real map object */
   map = elm_image_object_get(ed->minimap.map);

   /* Resize map for current minimap */
   evas_object_size_hint_max_set(ed->minimap.map,
                                 (float)ed->pud->map_w * ratio,
                                 (float)ed->pud->map_h * ratio);
   evas_object_size_hint_min_set(ed->minimap.map,
                                 (float)ed->pud->map_w * ratio,
                                 (float)ed->pud->map_h * ratio);

   /* Configure minimap image */
   evas_object_image_size_set(map, ed->pud->map_w, ed->pud->map_h);
   evas_object_image_data_set(map, ed->minimap.data[0]);

   return EINA_TRUE;
fail_free:
   free(ptr);
fail:
   return EINA_FALSE;
}

void
minimap_show(Editor *ed)
{
   elm_layout_content_set(ed->lay, "war2edit.main.minimap", ed->minimap.map);
}

void
minimap_del(Editor *ed)
{
   EINA_SAFETY_ON_NULL_RETURN(ed);

   if (ed->minimap.data)
     {
        free(ed->minimap.data[0]);
        free(ed->minimap.data);
     }
}

Eina_Bool
minimap_reload(Editor *ed)
{
   unsigned int i, j;
   Eina_Bool ret = EINA_TRUE;

   if (ed->bitmap.norender) return EINA_FALSE;
   for (j = 0; j < ed->pud->map_h; j++)
     for (i = 0; i < ed->pud->map_w; i++)
       ret &= minimap_update(ed, i, j);

   return ret;
}

Eina_Bool
minimap_update(Editor       *ed,
               unsigned int  x,
               unsigned int  y)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(ed, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL((x >= ed->pud->map_w) ||
                                  (y >= ed->pud->map_h), EINA_FALSE);

   const Cell *c = &(ed->cells[y][x]);
   uint8_t player;
   unsigned int px, py;
   Pud_Unit u = PUD_UNIT_NONE;
   Pud_Color col;
   unsigned char *ptr;
   unsigned int i, j;
   unsigned int ry, rx;
   unsigned int w, h;

   if (ed->bitmap.norender) return EINA_FALSE;

   if (c->unit_above != PUD_UNIT_NONE)
     {
        player = c->player_above;
        u = c->unit_above;
     }
   else if (c->unit_below != PUD_UNIT_NONE)
     {
        player = c->player_below;
        u = c->unit_below;
     }

   if (u == PUD_UNIT_NONE)
     {
        col = pud_minimap_tile_to_color(ed->pud->era, c->tile);
        w = 1;
        h = 1;
     }
   else
     {
        col = pud_minimap_color_for_unit(u, player);
        sprite_tile_size_get(u, &w, &h);
     }

   /* Format ARGB8888: each pixel is 4 bytes long */
   px = x * 4;
   py = y;
   rx = px + (w * 4);
   ry = py + h;

   if (rx > ed->pud->map_w * 4) rx = (ed->pud->map_w - 1) * 4;
   if (ry > ed->pud->map_h) ry = ed->pud->map_h - 1;

   for (j = py; j < ry; ++j)
     {
        for (i = px; i < rx; i += 4)
          {
             ptr = &(ed->minimap.data[j][i]); /* Raw data */
             ptr[0] = col.b;
             ptr[1] = col.g;
             ptr[2] = col.r;
             ptr[3] = col.a;
          }
     }

   /*
    * Don't update the image!
    * Use minimap_render() to do so (for perfs).
    */

   return EINA_TRUE;
}

void
minimap_render(Editor *ed,
               unsigned int  x,
               unsigned int  y,
               unsigned int  w,
               unsigned int  h)
{
   if (ed->bitmap.norender) return;
   evas_object_image_data_update_add(elm_image_object_get(ed->minimap.map),
                                     x, y, w, h);
}

void
minimap_render_unit(Editor *ed,
                    unsigned int  x,
                    unsigned int  y,
                    Pud_Unit      u)
{
   minimap_render(ed, x, y, ed->pud->units_descr[u].size_w,
                  ed->pud->units_descr[u].size_h);
}

void
minimap_view_move(Editor    *ed,
                  int         x,
                  int         y,
                  Eina_Bool   clicked)
{
   int bx, by, rw = 0, rh = 0, srw, srh, cx, cy, ox, oy, ow, oh, tx, ty;

   evas_object_geometry_get(ed->minimap.rect, NULL, NULL, &rw, &rh);
   evas_object_geometry_get(ed->minimap.map, &ox, &oy, &ow, &oh);

   if (clicked)
     {
        x -= rw / (2.0f * ed->minimap.ratio);
        y -= rh / (2.0f * ed->minimap.ratio);
     }

   if (x < 0) x = 0;
   if (y < 0) y = 0;

   tx = rintf(((float)x * ed->minimap.ratio) + (float)ox);
   ty = rintf(((float)y * ed->minimap.ratio) + (float)oy);

   if (tx - ox + rw > ow) tx = ox + ow - rw;
   if (ty - oy + rh > oh) ty = oy + oh - rh;

   evas_object_move(ed->minimap.rect, tx, ty);

   if (clicked)
     {
        elm_scroller_region_get(ed->scroller, NULL, NULL, &srw, &srh);
        bitmap_cell_size_get(ed, &cx, &cy);
        bx = x * cx;
        by = y * cy;
        elm_scroller_region_show(ed->scroller, bx, by, srw, srh);
     }
}

void
minimap_view_resize(Editor       *ed,
                    unsigned int  w,
                    unsigned int  h)
{
   const float wf = (float)w;
   const float hf = (float)h;
   w = rintf(wf * ed->minimap.ratio);
   h = rintf(hf * ed->minimap.ratio);
   if (wf > (float)ed->pud->map_w * ed->minimap.ratio)
     w = rintf((float)ed->pud->map_w * ed->minimap.ratio);
   if (hf > (float)ed->pud->map_h * ed->minimap.ratio)
     h = rintf((float)ed->pud->map_h * ed->minimap.ratio);
   evas_object_resize(ed->minimap.rect, w, h);
}

unsigned char *
minimap_pixels_get(const Editor *ed,
                   int *width,
                   int *height)
{
   if (width) *width = ed->pud->map_w;
   if (height) *height = ed->pud->map_h;
   return ed->minimap.data[0];
}
