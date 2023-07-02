/**
 * File:        util.c
 * Purpose:     Helper utility functions
 *
 * Author:      Lionel Pinkhard
 * Date:        January 27, 2019
 * Version:     1.0
 *
 */

#include "util.h"

#include "mappyal.h"

/**
 * Grabs a frame for an animation
 *
 * Parameters:
 * src		    Source bitmap
 * w   			Width of a single frame
 * h			Height of a single frame
 * startx		Starting X coordinate
 * starty		Starting Y coordinate
 * col			Number of frames in bitmap
 * frame		Frame to grab
 */
BITMAP * grabFrame(BITMAP * src, int w, int h, int startx, int starty, int col, int frame) {
    int x;
    int y;

    BITMAP * tmp = create_bitmap(w, h);
    x = startx + (frame % col) * w;
    y = starty + (frame / col) * h;

    blit(src, tmp, x, y, 0, 0, w, h);

    return tmp;
}

/**
 * Checks for a collision on the map at given screen coordinates
 *
 * Parameters:
 * x			X coordinate
 * y			Y coordinate
 */
int mapCollision(int x, int y) {
    // Find the tile
    BLKSTR * bBlkData;
    bBlkData = MapGetBlock(x / 32, y / 32);

    // Zoom in on the collision area
    if (x % 32 < 16) // Left side of tile
    {
        if (y % 32 < 16) // Top side of tile
        {
            return bBlkData -> tl;
        } else // Bottom side of tile
        {
            return bBlkData -> bl;
        }
    } else // Right side of tile
    {
        if (y % 32 < 16) // Top side of tile
        {
            return bBlkData -> tr;
        } else // Bottom side of tile
        {
            return bBlkData -> br;
        }
    }
}

/**
 * Checks for the presence of spikes on the map at given screen coordinates
 *
 * Parameters:
 * x			X coordinate
 * y			Y coordinate
 */
int spikeCheck(int x, int y) {
    // Fine the tile
    BLKSTR * bBlkData;
    bBlkData = MapGetBlock(x / 32, y / 32);

    // Return flag 1
    return bBlkData -> unused1;
}
