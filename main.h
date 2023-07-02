/**
 * File:        main.h
 * Purpose:     Header file for main.c
 *
 * Author:      Lionel Pinkhard
 * Date:        January 27, 2019
 * Version:     1.0
 *
 */

// Only include this header once
#ifndef _MAIN_H_
#define _MAIN_H_

// Include Allegro library and C stdlib
#include <allegro.h>
#include <stdio.h>
#include <stdlib.h>

// Include pthread
#include "pthread.h"

// Include headers for external functions
#include "defines.h"
#include "mappyal.h"
#include "util.h"

// Defines for the game
#define JUMPIT	1600

#define MAP_WIDTH 1500
#define MAP_HEIGHT 25

#define MODE_INTRO 0
#define MODE_GAMEPLAY 1
#define MODE_HELP 2

#define NUM_ACTORS 5

// Sprite structure
typedef struct SPRITE
{
	int active;
	int alive;
	int player;
	int x, y;
	int oldx, oldy;
	int moving;
	int dir;
	int jump;
	int jumpqueued;
	BITMAP *bitmaps[8];
	int w, h;
	int xspd, yspd;
	int xdelay, ydelay;
	int xcount, ycount;
	int frame, maxframe, animdir;
	int framecount, framedelay;
} SPRITE;

#endif
