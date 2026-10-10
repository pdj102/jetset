#include "zxn_tilemap.h"

#include <arch/zxn.h>
#include <string.h>

#include "zxn_util.h"

#define ZXN_TILEMAP_BASE 0x4000 /* The base address of the tilemap in memory */

/* Tilemap memory; the PC tests point this at an array instead */
#ifndef ZXN_TILEMAP_MEM
#define ZXN_TILEMAP_MEM ((zxn_tile_t *) ZXN_TILEMAP_BASE)
#endif

#define ZXN_REG_TILEMAP_CONTROL 0x6B    /* Tilemap Control Register */
#define ZXN_RTC_ENABLE     0b10000000 /* bit 7 = 1 to enable the tilemap */
#define ZXN_RTC_80X32      0b01000000 /* bit 6 = 0 for 40x32, 1 for 80x32 */
#define ZXN_RTC_NO_ATTR    0b00100000 /* bit 5 = 1 to eliminate the attribute entry in the tilemap */
#define ZXN_RTC_PALETTE_1  0b00010000 /* bit 4 = palette select, 0 = first tilemap palette 1 = second*/
#define ZXN_RTC_TEXT       0b00000100 /* bit 3 = 0 for graphics, 1 for text mode */
#define ZXN_RTC_512_TILES  0b00000010 /* bit 1 = 1 to activate 512 tile mode */
#define ZXN_RTC_ON_TOP     0b00000001 /* bit 0 = 1 to force tilemap on top of ULA */

#define ZXN_REG_TILEMAP_TRANSPARENCY_INDEX 0x4C /* Tilemap Transparency Index Register */

#define ZXN_REG_TILEMAP_BASE_ADDR 0x6E /*  Tilemap Base Address Register */

#define ZXN_REG_TILEDEFS_BASE_ADDR 0x6F /* Tile Definitions Base Address Register */

#define ZXN_REG_CLIP_WINDOW_TILEMAP  0x1B /* Tilemap Clip Window Register */
#define ZXN_REG_CLIP_WINDOW_CONTROL  0x1C /* Clip Window Control Register */
#define ZXN_CLIP_WINDOW_RESET_TILEMAP_INDEX 0x08

#define ZXN_REG_TILEMAP_X_OFFSET_MSB 0x2F
#define ZXN_REG_TILEMAP_X_OFFSET_LSB 0x30
#define ZXN_REG_TILEMAP_Y_OFFSET    0x31


static  zxn_tile_t *tilemap_base_p = ZXN_TILEMAP_MEM;

void zxn_tilemap_set_scroll(uint16_t x, uint8_t y)
{
    zxn_util_assert(x <= ZXN_TILEMAP_X_OFFSET_MAX);

    ZXN_WRITE_REG(ZXN_REG_TILEMAP_X_OFFSET_MSB, (uint8_t)((x >> 8) & 0x03));
    ZXN_WRITE_REG(ZXN_REG_TILEMAP_X_OFFSET_LSB, (uint8_t)(x & 0xFF));
    ZXN_WRITE_REG(ZXN_REG_TILEMAP_Y_OFFSET, y);
}

void zxn_tilemap_set_clip(const zxn_rect_t *r)
{
    zxn_util_assert(r != NULL);

    ZXN_WRITE_REG(ZXN_REG_CLIP_WINDOW_CONTROL, ZXN_CLIP_WINDOW_RESET_TILEMAP_INDEX);
    ZXN_WRITE_REG(ZXN_REG_CLIP_WINDOW_TILEMAP, r->x1);
    ZXN_WRITE_REG(ZXN_REG_CLIP_WINDOW_TILEMAP, r->x2);
    ZXN_WRITE_REG(ZXN_REG_CLIP_WINDOW_TILEMAP, r->y1);
    ZXN_WRITE_REG(ZXN_REG_CLIP_WINDOW_TILEMAP, r->y2);
}

void zxn_tilemap_init(void)
{
    /** 
    * Set the Tilemap Control Register
    *
    * bit 7    = 1 to enable tilemap
    * bit 6    = 0 for 40x32, 1 for 80x32
    * bit 5    = 1 to eliminate the attribute entry in the tilemap
    * bit 4    = palette select (0 = first Tilemap palette, 1 = second)
    * bits3    = 0 to enable graphics mode (1 for text mode)
    * bit 2    = Reserved set to 0
    * bit 1    = 1 to activate 512 tile mode (bit 0 of tile attribute is ninth bit of tile-id) 
    * bit 0    = 1 to force tilemap on top of ULA    
    */
    ZXN_WRITE_REG(ZXN_REG_TILEMAP_CONTROL, ZXN_RTC_ENABLE | ZXN_RTC_512_TILES | ZXN_RTC_ON_TOP);


    /**
     * Set the ZXnext tilemap base address register 
     *
     * Sets the tilemap base address offset within bank 5
     *
     * Bank 5 is located at 0x4000 - 0x7fff
     * The ULA screen is located 0x4000 - 0x5FFF (7 KiB)
     * It is assumed the ULA screen is not required and has been disabled so we can place the tilemap at 0x4000
     * The tilemap is 40 * 32 tiles * 2 bytes per tile = 0xA00 (2,560) bytes
     *
     * 0x6E is set with the MSB which represents an n x 256 byte offset within bank 5
     * For example, to load the tilemap at 0x6000 the offset would be 32 decimal (0x6000 - 0x4000) / 0x100 = 0x20 (32)
     *
     * Write the tilemap to 0x4000 - 0x49FF (offset 0 in bank 5)
     */
    ZXN_NEXTREG(ZXN_REG_TILEMAP_BASE_ADDR, 0);

    /**
     * Set the ZXnext register for the base memory address of the tile definitions
     * 
     * 0x6F - Tile definitions base address
     *
     * sets the tile definitions base address offset within bank 5 (or bank 7 if bit 7 is set)
     * Bank 5 is located at 0x4000 - 0x7fff
     * The tilemap is 2,560 (0xa00) bytes and occupies 0x4000 - 0x49ff
     * Loading the tile definitions at or above 0x4A00 avoids it
     * 0x6F is set with the MSB which represents an n x 256 byte offset. (0x4A00 - 0x4000) / 0x100 = 10 decimal 
     * 
     */
    ZXN_NEXTREG(ZXN_REG_TILEDEFS_BASE_ADDR , 10);


    /* Pixels with this 4-bit value are transparent in every tile */
    ZXN_WRITE_REG(ZXN_REG_TILEMAP_TRANSPARENCY_INDEX, ZXN_TILEMAP_TRANSPARENT);
}

 void zxn_tilemap_clear(const zxn_tile_t *tile_p)
{
    zxn_util_assert(tile_p != NULL);

    zxn_tile_t *t_p = ZXN_TILEMAP_MEM;

    uint16_t count = ZXN_TILEMAP_WIDTH * ZXN_TILEMAP_HEIGHT; 
    uint16_t s;

    for (s = 0; s < count; s++) {
        *t_p++ = *tile_p; /* copy the tile and increment to next tile*/
    }
}

void zxn_tilemap_set(uint8_t x, uint8_t y, const zxn_tile_t *tile_p)
{
    zxn_util_assert(x < ZXN_TILEMAP_WIDTH);
    zxn_util_assert(y < ZXN_TILEMAP_HEIGHT);
    zxn_util_assert(tile_p != NULL);

    zxn_tile_t *t_p = tilemap_base_p + ( (y * ZXN_TILEMAP_WIDTH) + x);

    t_p->tile_id = tile_p->tile_id;
    t_p->tile_attr = tile_p->tile_attr;
}

zxn_tile_t *zxn_tilemap_cell(uint8_t x, uint8_t y)
{
    zxn_util_assert(x < ZXN_TILEMAP_WIDTH);
    zxn_util_assert(y < ZXN_TILEMAP_HEIGHT);

    return tilemap_base_p + ((y * ZXN_TILEMAP_WIDTH) + x);
}

void zxn_tilemap_set_attr(uint8_t x, uint8_t y, uint8_t tile_attr)
{
    zxn_util_assert(x < ZXN_TILEMAP_WIDTH);
    zxn_util_assert(y < ZXN_TILEMAP_HEIGHT);

    volatile zxn_tile_t *t_p = tilemap_base_p + ( (y * ZXN_TILEMAP_WIDTH) + x);

    t_p->tile_attr = tile_attr;
}

void  zxn_tilemap_copy(uint8_t fx, uint8_t fy, uint8_t tx, uint8_t ty)
{
    zxn_util_assert(fx < ZXN_TILEMAP_WIDTH);
    zxn_util_assert(fy < ZXN_TILEMAP_HEIGHT);
    zxn_util_assert(tx < ZXN_TILEMAP_WIDTH);
    zxn_util_assert(ty < ZXN_TILEMAP_HEIGHT);
    
    volatile zxn_tile_t *f_p = tilemap_base_p + ( (fy * ZXN_TILEMAP_WIDTH) + fx);
    volatile zxn_tile_t *t_p = tilemap_base_p + ( (ty * ZXN_TILEMAP_WIDTH) + tx);

    *t_p = *f_p; /* copy the tile */
}


void zxn_tilemap_fill(const zxn_rect_t *r, const zxn_tile_t *fill_p)
{
    zxn_util_assert(r != NULL);
    zxn_util_assert(fill_p != NULL);
    zxn_util_assert(r->x1 <= r->x2 && r->x2 < ZXN_TILEMAP_WIDTH);
    zxn_util_assert(r->y1 <= r->y2 && r->y2 < ZXN_TILEMAP_HEIGHT);

    zxn_tile_t *row_p = tilemap_base_p + ((r->y1 * ZXN_TILEMAP_WIDTH) + r->x1);
    uint8_t width = r->x2 - r->x1 + 1;
    uint8_t rows = r->y2 - r->y1 + 1;
    uint8_t x;

    while (rows--) {
        zxn_tile_t *t_p = row_p;

        for (x = width; x; x--) {
            *t_p++ = *fill_p;
        }
        row_p += ZXN_TILEMAP_WIDTH;
    }
}

void zxn_tilemap_scroll_up(const zxn_rect_t *r, const zxn_tile_t *fill_p)
{
    zxn_util_assert(r != NULL);
    zxn_util_assert(fill_p != NULL);
    zxn_util_assert(r->x1 <= r->x2 && r->x2 < ZXN_TILEMAP_WIDTH);
    zxn_util_assert(r->y1 <= r->y2 && r->y2 < ZXN_TILEMAP_HEIGHT);

    zxn_tile_t *dst_p = tilemap_base_p + ((r->y1 * ZXN_TILEMAP_WIDTH) + r->x1);
    uint16_t bytes = (uint16_t)(r->x2 - r->x1 + 1) * sizeof(zxn_tile_t);
    uint8_t rows = r->y2 - r->y1;
    zxn_rect_t bottom;

    /* Copy each row up one line; memcpy compiles to a single LDIR per row */
    while (rows--) {
        memcpy(dst_p, dst_p + ZXN_TILEMAP_WIDTH, bytes);
        dst_p += ZXN_TILEMAP_WIDTH;
    }

    bottom.x1 = r->x1;
    bottom.x2 = r->x2;
    bottom.y1 = r->y2;
    bottom.y2 = r->y2;
    zxn_tilemap_fill(&bottom, fill_p);
}
