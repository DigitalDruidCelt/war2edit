/*
 * sprite.c
 *
 * Copyright (c) 2015 Jean Guyomarc'h
 */

#include "war2edit.h"

#define SELECTION_1x1 "sel/1x1"
#define SELECTION_2x2 "sel/2x2"
#define SELECTION_3x3 "sel/3x3"
#define SELECTION_4x4 "sel/4x4"

static Eet_File *_units_ef = NULL;
static Eet_File *_buildings[4] = { NULL, NULL, NULL, NULL };
static Eina_Hash *_sprites = NULL;

static Eina_Bool
_sprite_load_add(Eet_File   *ef,
                 const char *key)
{
   unsigned char *data;

   data = sprite_load(ef, key, NULL, NULL, NULL, NULL);
   if (EINA_UNLIKELY(!data))
     {
        CRI("Failed to load data for key \"%s\"", key);
        return EINA_FALSE;
     }
   if (EINA_UNLIKELY(!eina_hash_add(_sprites, key, data)))
     {
        CRI("Failed to write data in hash for key \"%s\"", key);
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

void *
sprite_load(Eet_File     *src,
            const char   *key,
            int          *x_ret,
            int          *y_ret,
            unsigned int *w_ret,
            unsigned int *h_ret)
{
   unsigned char *mem;
   int size, expected_size;
   uint16_t x, y, w, h;

   mem = eet_read(src, key, &size);
   EINA_SAFETY_ON_NULL_RETURN_VAL(mem, NULL);

   memcpy(&x, mem + 0, sizeof(uint16_t));
   memcpy(&y, mem + 2, sizeof(uint16_t));
   memcpy(&w, mem + 4, sizeof(uint16_t));
   memcpy(&h, mem + 6, sizeof(uint16_t));

   expected_size = (w * h * 4) + 8;
   if (expected_size != size)
     {
        CRI("Sprite data was loaded with size [%i], expected [%i]",
            size, expected_size);
        free(mem);
        return NULL;
     }

   //DBG("Loaded sprite [%s] of size %ix%i (offsets: %i,%i)\n", key, w, h, x, y);

   if (x_ret) *x_ret = x;
   if (y_ret) *y_ret = y;
   if (w_ret) *w_ret = w;
   if (h_ret) *h_ret = h;
   return mem + 8;
}

Eet_File *
sprite_units_open(void)
{
   Eet_File *ef;
   char path[PATH_MAX];

   /* Don't open the units file twice */
   if  (_units_ef)
     return _units_ef;

   snprintf(path, sizeof(path),
            "%s/sprites/units/units.eet", elm_app_data_dir_get());
   ef = eet_open(path, EET_FILE_MODE_READ);
   if (EINA_UNLIKELY(ef == NULL))
     {
        CRI("Failed to open [%s]", path);
        return NULL;
     }
   DBG("Open units file [%s]", path);
   _units_ef = ef;

   return ef;
}

Eet_File *
sprite_buildings_open(Pud_Era era)
{
   EINA_SAFETY_ON_FALSE_RETURN_VAL((era >= 0) && (era <= 3), NULL);

   Eet_File *ef;
   const char *file = NULL;
   char path[PATH_MAX];

   /* Don't load buildings file twice */
   if (_buildings[era])
     return _buildings[era];

   switch (era)
     {
      case PUD_ERA_FOREST:    file = "sprites/buildings/forest.eet";    break;
      case PUD_ERA_WINTER:    file = "sprites/buildings/winter.eet";    break;
      case PUD_ERA_WASTELAND: file = "sprites/buildings/wasteland.eet"; break;
      case PUD_ERA_SWAMP:     file = "sprites/buildings/swamp.eet";     break;
     }

   snprintf(path, sizeof(path), "%s/%s", elm_app_data_dir_get(), file);
   ef = eet_open(path, EET_FILE_MODE_READ);
   if (EINA_UNLIKELY(ef == NULL))
     {
        CRI("Failed to open [%s]", path);
        return NULL;
     }
   DBG("Open buildings file [%s]", path);

   _buildings[era] = ef;

   return ef;
}

unsigned char *
sprite_get(Pud_Unit       unit,
           Pud_Era        era,
           Sprite_Info    info,
           int           *x,
           int           *y,
           unsigned int  *w,
           unsigned int  *h,
           Eina_Bool     *flip_me)
{
   unsigned char *data;
   char key[64];
   Eet_File *ef;
   Eina_Bool chk;
   int orient;
   Eina_Bool flip;
   uint16_t sx, sy, sw, sh;

   if (pud_unit_building_is(unit))
     {
        ef = _buildings[era];
        snprintf(key, sizeof(key), "%s", pud_unit2str(unit));
        flip = EINA_FALSE;
     }
   else
     {
        ef = _units_ef;

        if (info != SPRITE_INFO_ICON)
          {
             switch (info)
               {
                case SPRITE_INFO_SOUTH_WEST:
                   orient = SPRITE_INFO_SOUTH_EAST;
                   flip = EINA_TRUE;
                   break;

                case SPRITE_INFO_WEST:
                   orient = SPRITE_INFO_EAST;
                   flip = EINA_TRUE;
                   break;

                case SPRITE_INFO_NORTH_WEST:
                   orient = SPRITE_INFO_NORTH_EAST;
                   flip = EINA_TRUE;
                   break;

                default:
                   orient = info;
                   flip = EINA_FALSE;
                   break;
               }

             switch (unit)
               {
                case PUD_UNIT_GNOMISH_SUBMARINE:
                case PUD_UNIT_GIANT_TURTLE:
                case PUD_UNIT_CRITTER:
                   snprintf(key, sizeof(key), "%s/%s/%i",
                            pud_unit2str(unit), pud_era2str(era), orient);
                   break;

                case PUD_UNIT_HUMAN_START:
                case PUD_UNIT_ORC_START:
                   snprintf(key, sizeof(key), "%s/0", pud_unit2str(unit));
                   break;

                default:
                   snprintf(key, sizeof(key), "%s/%i",
                            pud_unit2str(unit), orient);
                   break;
               }
          }
        else
          {
             CRI("ICONS not implemented!");
             return NULL;
          }
     }
   if (flip_me) *flip_me = flip;

   data = eina_hash_find(_sprites, key);
   if (data == NULL)
     {
        data = sprite_load(ef, key, x, y, w, h);
        if (EINA_UNLIKELY(data == NULL))
          {
             ERR("Failed to load sprite for key [%s]", key);
             return NULL;
          }
        chk = eina_hash_add(_sprites, key, data);
        if (EINA_UNLIKELY(chk == EINA_FALSE))
          {
             ERR("Failed to add sprite <%p> to hash", data);
             free(data);
             return NULL;
          }
        //DBG("Access key [%s] (not yet registered). SRT = <%p>", key, data);
        return data;
     }
   else
     {
        if (x) { memcpy(&sx, data - 8, sizeof(uint16_t)); *x = sx; }
        if (y) { memcpy(&sy, data - 6, sizeof(uint16_t)); *y = sy; }
        if (w) { memcpy(&sw, data - 4, sizeof(uint16_t)); *w = sw; }
        if (h) { memcpy(&sh, data - 2, sizeof(uint16_t)); *h = sh; }
        //DBG("Access key [%s] (already registered). SRT = <%p>", key, data);
        return data;
     }
}

Sprite_Info
sprite_info_random_get(void)
{
   /* Does not return 4 */
   return rand() % (SPRITE_INFO_NORTH_WEST - SPRITE_INFO_NORTH) + SPRITE_INFO_NORTH;
}


static void
_free_cb(void *data)
{
   /* There is an 8 bytes offset */
   data -= 8;
   free(data);
}

Eina_Bool
sprite_init(void)
{
   Eet_File *ef;
   char path[PATH_MAX];

   snprintf(path, sizeof(path),
            "%s/sprites/misc/sel.eet", elm_app_data_dir_get());
   if (EINA_UNLIKELY(!sprite_units_open()))
     {
        CRI("Failed to open units file");
        goto fail;
     }

   _sprites = eina_hash_string_superfast_new(_free_cb);
   if (EINA_UNLIKELY(!_sprites))
     {
        CRI("Failed to create Hash for sprites");
        goto sprites_fail;
     }

   if (EINA_UNLIKELY(!sprite_units_open()))
     {
        CRI("Failed to load units");
        goto units_fail;
     }

   ef = eet_open(path, EET_FILE_MODE_READ);
   if (EINA_UNLIKELY(!ef))
     {
        CRI("Failed to open file \"%s\"", path);
        goto sel_fail;
     }

   _sprite_load_add(ef, SELECTION_1x1);
   _sprite_load_add(ef, SELECTION_2x2);
   _sprite_load_add(ef, SELECTION_3x3);
   _sprite_load_add(ef, SELECTION_4x4);

   return EINA_TRUE;

sel_fail:
units_fail:
   eina_hash_free(_sprites);
sprites_fail:
   eet_close(_units_ef);
fail:
   return EINA_FALSE;
}

void
sprite_shutdown(void)
{
   unsigned int i;

   for (i = 0; i < EINA_C_ARRAY_LENGTH(_buildings); ++i)
     {
        if (_buildings[i])
          {
             eet_close(_buildings[i]);
             _buildings[i] = NULL;
          }
     }

   eet_close(_units_ef);
   _units_ef = NULL;
   eina_hash_free(_sprites);
}

void
sprite_tile_size_get(Pud_Unit      unit,
                     unsigned int *sprite_w,
                     unsigned int *sprite_h)
{
   unsigned int w;
   unsigned int h;

   switch (unit)
     {
      case PUD_UNIT_NONE:
         w = 0;
         h = 0;
         break;

      case PUD_UNIT_PIG_FARM:
      case PUD_UNIT_FARM:
      case PUD_UNIT_ORC_SCOUT_TOWER:
      case PUD_UNIT_HUMAN_SCOUT_TOWER:
      case PUD_UNIT_HUMAN_GUARD_TOWER:
      case PUD_UNIT_HUMAN_CANNON_TOWER:
      case PUD_UNIT_ORC_GUARD_TOWER:
      case PUD_UNIT_ORC_CANNON_TOWER:
      case PUD_UNIT_GOBLIN_ZEPPLIN:
      case PUD_UNIT_GNOMISH_FLYING_MACHINE:
      case PUD_UNIT_ORC_TANKER:
      case PUD_UNIT_HUMAN_TANKER:
      case PUD_UNIT_GRYPHON_RIDER:
      case PUD_UNIT_ELVEN_DESTROYER:
      case PUD_UNIT_TROLL_DESTROYER:
      case PUD_UNIT_GNOMISH_SUBMARINE:
      case PUD_UNIT_GIANT_TURTLE:
      case PUD_UNIT_ORC_TRANSPORT:
      case PUD_UNIT_HUMAN_TRANSPORT:
      case PUD_UNIT_CIRCLE_OF_POWER:
      case PUD_UNIT_RUNESTONE:
         w = 2;
         h = 2;
         break;

      case PUD_UNIT_DEATHWING:
      case PUD_UNIT_HUMAN_BARRACKS:
      case PUD_UNIT_ORC_BARRACKS:
      case PUD_UNIT_CHURCH:
      case PUD_UNIT_ALTAR_OF_STORMS:
      case PUD_UNIT_STABLES:
      case PUD_UNIT_OGRE_MOUND:
      case PUD_UNIT_GNOMISH_INVENTOR:
      case PUD_UNIT_GOBLIN_ALCHEMIST:
      case PUD_UNIT_GRYPHON_AVIARY:
      case PUD_UNIT_DRAGON_ROOST:
      case PUD_UNIT_HUMAN_SHIPYARD:
      case PUD_UNIT_ORC_SHIPYARD:
      case PUD_UNIT_ELVEN_LUMBER_MILL:
      case PUD_UNIT_TROLL_LUMBER_MILL:
      case PUD_UNIT_HUMAN_FOUNDRY:
      case PUD_UNIT_ORC_FOUNDRY:
      case PUD_UNIT_MAGE_TOWER:
      case PUD_UNIT_TEMPLE_OF_THE_DAMNED:
      case PUD_UNIT_HUMAN_BLACKSMITH:
      case PUD_UNIT_ORC_BLACKSMITH:
      case PUD_UNIT_HUMAN_REFINERY:
      case PUD_UNIT_ORC_REFINERY:
      case PUD_UNIT_HUMAN_OIL_WELL:
      case PUD_UNIT_ORC_OIL_WELL:
      case PUD_UNIT_GOLD_MINE:
      case PUD_UNIT_OIL_PATCH:
      case PUD_UNIT_DRAGON:
      case PUD_UNIT_JUGGERNAUGHT:
      case PUD_UNIT_BATTLESHIP:
         w = 3;
         h = 3;
         break;

      case PUD_UNIT_GREAT_HALL:
      case PUD_UNIT_TOWN_HALL:
      case PUD_UNIT_STRONGHOLD:
      case PUD_UNIT_KEEP:
      case PUD_UNIT_CASTLE:
      case PUD_UNIT_FORTRESS:
      case PUD_UNIT_DARK_PORTAL:
         w = 4;
         h = 4;
         break;

      default:
         w = 1;
         h = 1;
         break;
     }

   if (sprite_w) *sprite_w = w;
   if (sprite_h) *sprite_h = h;
}

unsigned char *
sprite_selection_get(unsigned int edge)
{
   const char *key = NULL;
   switch (edge)
     {
      case 1:
         key = SELECTION_1x1;
         break;

      case 2:
         key = SELECTION_2x2;
         break;

      case 3:
         key = SELECTION_3x3;
         break;

      case 4:
         key = SELECTION_4x4;
         break;

      default:
         CRI("Invalid edge parameter '%u'", edge);
         return NULL;
     }

   return eina_hash_find(_sprites, key);
}

