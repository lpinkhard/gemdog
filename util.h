/**
 * File:        util.h
 * Purpose:     Header file for util.c
 *
 * Author:      Lionel Pinkhard
 * Date:        January 27, 2019
 * Version:     1.0
 *
 */

// Only include this header once
#ifndef _UTIL_H_
#define _UTIL_H_

// Include Allegro library and C stdlib
#include <allegro.h>
#include <stdlib.h>

// Function declarations
BITMAP *grabFrame(BITMAP *src, int w, int h, int startx, int starty, int col, int frame);
int mapCollision(int x, int y);
int spikeCheck(int x, int y);

#endif
