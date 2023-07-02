/**
 * File:        main.c
 * Purpose:     The main C source file for the game
 *
 * Author:      Lionel Pinkhard
 * Date:        January 27, 2019
 * Version:     1.0
 *
 */
#include "main.h"

// Display buffer
BITMAP * g_bBuffer;

// Actor sprites
SPRITE * g_sActors[NUM_ACTORS];
int g_nActors = NUM_ACTORS;
SPRITE * g_sPlayer;

// Sound samples
SAMPLE * g_sJump;
SAMPLE * g_sDie;
SAMPLE * g_sWin;

// Current player information
int g_nPlayerScore = 0;
int g_nHighScore = 0;
int g_bVictory = 0;
int g_nTimeLeft = 90;

// Map information
int g_nMapX = 0;
int g_nMapY = 0;

// Data file reference
DATAFILE * g_dData;

// Whether busy exiting
int g_bExiting = 0;

// Mutex
pthread_mutex_t threadsafe = PTHREAD_MUTEX_INITIALIZER;

// Display state
int g_nMode = MODE_INTRO;

/**
 * Checks for a collision with interactable objects on the map at given screen coordinates
 *
 * Parameters:
 * x			X coordinate
 * y			Y coordinate
 */
int objectCheck(int x, int y) {
    // Fine the tile
    BLKSTR * bBlkData;
    bBlkData = MapGetBlock(x / 32, y / 32);

    if (bBlkData -> unused2) // End of game
    {
        // Add score
        g_nPlayerScore += 150;

        // No longer alive, but victory
        g_sPlayer -> alive = 0;
        g_bVictory = 1;

        play_sample(g_sWin, 250, 128, 1000, 0);

        return 2;
    }

    if (bBlkData -> unused3) // Gem pickup
    {
        // Add score
        g_nPlayerScore += bBlkData -> user1;

        // Set taken
        MapSetBlock(x / 32, y / 32, 0);
        return 1;
    } else {
        return 0;
    }
}

void plantReset(int index) {
    g_sActors[index] -> player = 0;
    g_sActors[index] -> frame = 0;
    g_sActors[index] -> framecount = 0;
    g_sActors[index] -> framedelay = 13;
    g_sActors[index] -> maxframe = 2;
    g_sActors[index] -> alive = 1;
    g_sActors[index] -> w = g_sActors[index] -> bitmaps[0] -> w;
    g_sActors[index] -> h = g_sActors[index] -> bitmaps[0] -> h;

    g_sActors[index] -> jump = JUMPIT;
    g_sActors[index] -> y = 100;
    g_sActors[index] -> dir = 0;

    g_sPlayer -> active = 0;
}

/**
 * Resets the player state to start a new game
 */
void playerReset() {
    FILE * fp; // Pointer to score file

    g_sPlayer -> player = 1;
    g_sPlayer -> frame = 0;
    g_sPlayer -> framecount = 0;
    g_sPlayer -> framedelay = 5;
    g_sPlayer -> maxframe = 7;
    g_sPlayer -> alive = 1;
    g_sPlayer -> w = g_sPlayer -> bitmaps[0] -> w;
    g_sPlayer -> h = g_sPlayer -> bitmaps[0] -> h;

    // Position the player
    g_sPlayer -> x = g_sPlayer -> w;
    g_sPlayer -> y = 100;
    g_sPlayer -> jump = JUMPIT;

    g_sPlayer -> active = 1;

    g_bVictory = 0;

    // Update high score and reset score
    if (g_nPlayerScore > g_nHighScore) {
        g_nHighScore = g_nPlayerScore > g_nHighScore ? g_nPlayerScore : g_nHighScore;
        // Save high score to file
        fp = fopen("score.dat", "w+");
        if (fp != NULL) {
            fprintf(fp, "%d", g_nHighScore);
            fclose(fp);
        }
    }

    // Reset score
    g_nPlayerScore = 0;
    g_nTimeLeft = 90;

    // Reset the plants
    plantReset(1);
    plantReset(2);
    plantReset(3);
    plantReset(4);

    // Position the plants
    g_sActors[1] -> x = g_sActors[1] -> w * 10;
    g_sActors[2] -> x = g_sActors[2] -> w * 52;
    g_sActors[3] -> x = g_sActors[3] -> w * 41;
    g_sActors[3] -> y = 700;
    g_sActors[4] -> x = g_sActors[4] -> w * 60;
}

/**
 * Moves the actors on the screen, according to rules and physics
 */
void moveActors() {
    int i;

    for (i = 0; i < g_nActors; i++) {
        // Only accept movements if alive
        if (g_sActors[i] -> alive) {
            // Loop through frames in the animation
            if (g_sActors[i] -> moving) {
                if (++g_sActors[i] -> framecount > g_sActors[i] -> framedelay) {
                    g_sActors[i] -> framecount = 0;
                    if (++g_sActors[i] -> frame > g_sActors[i] -> maxframe)
                        g_sActors[i] -> frame = 1;
                }
            }

            // Only player picks up gems		
            if (g_sActors[i] -> player)
                objectCheck(g_sActors[i] -> x + g_sActors[i] -> w / 2, g_sActors[i] -> y + g_sActors[i] -> h); // Take any gems
        }

        // Player is falling, not jumping
        if (g_sActors[i] -> jump == JUMPIT) {
            // Check for solid blocks
            if (!mapCollision(g_sActors[i] -> x + g_sActors[i] -> w / 2, g_sActors[i] -> y + g_sActors[i] -> h)) {
                g_sActors[i] -> jump = 0;
                if (spikeCheck(g_sActors[i] -> x + g_sActors[i] -> w / 2, g_sActors[i] -> y + g_sActors[i] -> h)) // Kill actor if hitting a spike
                {
                    // Only play sound once
                    if (g_sActors[i] -> alive && g_sActors[i] -> player) {
                        play_sample(g_sDie, 250, 128, 1000, 0);
                    }
                    g_sActors[i] -> alive = 0;
                }
            }

            // Actor wants to jump (only for player)
            if (i == 0 && g_sActors[i] -> alive && g_sActors[i] -> jumpqueued) {
                g_sActors[i] -> jump = 32;
                play_sample(g_sJump, 250, 128, 1000, 0);
            }
        } else {
            // Actor is jumping
            g_sActors[i] -> y -= g_sActors[i] -> jump / 3;
            g_sActors[i] -> jump--;
        }

        // End of jump
        if (g_sActors[i] -> jump < 0) {
            if (mapCollision(g_sActors[i] -> x + g_sActors[i] -> w / 2, g_sActors[i] -> y + g_sActors[i] -> h)) {
                g_sActors[i] -> jump = JUMPIT;
                while (mapCollision(g_sActors[i] -> x + g_sActors[i] -> w / 2, g_sActors[i] -> y + g_sActors[i] -> h))
                    g_sActors[i] -> y -= 2;
            }
        }

        if (!g_sActors[i] -> dir) { // Check collision on left edge of sprite
            if (mapCollision(g_sActors[i] -> x, g_sActors[i] -> y + g_sActors[i] -> h)) {
                g_sActors[i] -> x = g_sActors[i] -> oldx;
                if (!g_sActors[i] -> player)
                    g_sActors[i] -> dir = 1;
            }
        } else { // Check collision on right edge of sprite
            if (mapCollision(g_sActors[i] -> x + g_sActors[i] -> w, g_sActors[i] -> y + g_sActors[i] -> h)) {
                g_sActors[i] -> x = g_sActors[i] -> oldx;
                if (!g_sActors[i] -> player)
                    g_sActors[i] -> dir = 0;
            }
        }

        // Check collisions with edges
        if (g_sActors[i] -> x < 0) {
            g_sActors[i] -> x = 0;
            if (!g_sActors[i] -> player)
                g_sActors[i] -> dir = 1;
        } else if (g_sActors[i] -> x > (MAP_WIDTH - 1) * 32) {
            g_sActors[i] -> x = (MAP_WIDTH - 1) * 32;
            if (!g_sActors[i] -> player)
                g_sActors[i] -> dir = 0;
        }

        // Check player collisions
        if (i != 0 && g_sActors[i] -> alive) {
            if (abs(g_sActors[i] -> y - g_sActors[0] -> y) < g_sActors[i] -> h / 2) // Vertical collision
            {
                if (abs(g_sActors[i] -> x - g_sActors[0] -> x) < g_sActors[i] -> w / 2) // Horizontal collision
                {
                    // Kill player
                    if (g_sActors[0] -> alive) {
                        play_sample(g_sDie, 250, 128, 1000, 0);
                        g_sActors[0] -> alive = 0;
                        g_sActors[0] -> jump = JUMPIT;
                    }
                }
            }
        }

        if (g_sActors[i] -> y > 740) {
            // Only play sound once, only for player
            if (g_sActors[i] -> alive && g_sActors[i] -> player)
                play_sample(g_sDie, 250, 128, 1000, 0);

            // Actor is dead
            g_sActors[i] -> alive = 0;
            g_sActors[i] -> jump = JUMPIT;
        } else if (g_sActors[i] -> y < -g_sActors[i] -> h) {
            g_sActors[i] -> y = -g_sActors[i] -> h;
        }
    }
}

inline int actorVisible(int i) {
    return g_sActors[i] -> x - g_nMapX > -g_sActors[i] -> w && g_sActors[i] -> x - g_nMapX < SCREEN_W + g_sActors[i] -> w &&
        g_sActors[i] -> y - g_nMapY > -g_sActors[i] -> h && g_sActors[i] -> y - g_nMapY < SCREEN_H + g_sActors[i] -> h;
}

/**
 * Decides on AI movement for NPCs
 */
void aiMovement() {
    int i;
    int tmpY;
    int unsafe;

    for (i = 1; i < g_nActors; i++) {
        // Check if seen
        if ((!g_sActors[i] -> active) && actorVisible(i))
            g_sActors[i] -> active = 1;

        // Don't move if jumping or never seen
        if (g_sActors[i] -> jump == 0 && g_sActors[i] -> active) {
            // Move in current direction
            g_sActors[i] -> moving = 1;
            if (g_sActors[i] -> dir) {
                g_sActors[i] -> x += 5;
            } else {
                g_sActors[i] -> x -= 5;
            }
        }

        // Check for dangers
        tmpY = g_sActors[i] -> y;
        unsafe = 0;

        // Simulate a fall
        while (!mapCollision(g_sActors[i] -> x + g_sActors[i] -> w / 2, tmpY + g_sActors[i] -> h)) {
            tmpY += 2;
            if (tmpY > 730) // Check for falling off map
            {
                unsafe = 1;
                break;
            }

            if (spikeCheck(g_sActors[i] -> x + g_sActors[i] -> w / 2, tmpY + g_sActors[i] -> h)) // Check for spikes
            {
                unsafe = 1;
            }
        }

        // If unsafe, reverse direction
        if (unsafe) {
            if (g_sActors[i] -> dir) {
                g_sActors[i] -> dir = 0;
                g_sActors[i] -> x -= 10;
            } else {
                g_sActors[i] -> dir = 1;
                g_sActors[i] -> x += 10;
            }
        }
    }
}

/**
 * Performs one step in the game loop while the game is ongoing
 */
void gameStep() {
    // Iterator
    int i;

    // Keep track of current state
    g_sPlayer -> moving = 0;
    g_sPlayer -> jumpqueued = 0;

    // Update old locations
    for (i = 0; i < g_nActors; i++) {
        g_sActors[i] -> oldx = g_sActors[i] -> x;
        g_sActors[i] -> oldy = g_sActors[i] -> y;
    }

    // Only accept movements if player is alive
    if (g_sPlayer -> alive) {
        if (key[KEY_LEFT] || key[KEY_A]) // Go left
        {
            g_sPlayer -> x -= 3;
            g_sPlayer -> dir = 0;
            g_sPlayer -> moving = 1;
        } else if (key[KEY_RIGHT] || key[KEY_D]) // Go right
        {
            g_sPlayer -> x += 3;
            g_sPlayer -> dir = 1;
            g_sPlayer -> moving = 1;
        } else {
            g_sPlayer -> frame = 0; // Stop moving
        }

        // Wants to jump
        if (key[KEY_UP] || key[KEY_W])
            g_sPlayer -> jumpqueued = 1;
    }

    aiMovement();

    moveActors();

    // Determine scrolling offsets for drawing
    g_nMapX = g_sPlayer -> x + g_sPlayer -> w / 2 - SCREEN_W / 2;
    g_nMapY = g_sPlayer -> y + g_sPlayer -> h / 2 - SCREEN_H / 2;

    if (g_nMapX < 0)
        g_nMapX = 0;
    if (g_nMapX > MAP_WIDTH * 32 - SCREEN_W)
        g_nMapX = MAP_WIDTH * 32 - SCREEN_W;

    if (g_nMapY < 0)
        g_nMapY = 0;
    if (g_nMapY > MAP_HEIGHT * 32 - SCREEN_H)
        g_nMapY = MAP_HEIGHT * 32 - SCREEN_H;

    // Draw the map
    MapDrawBG(g_bBuffer, g_nMapX, g_nMapY, 0, 0, SCREEN_W - 1, SCREEN_H - 1);
    MapDrawFG(g_bBuffer, g_nMapX, g_nMapY, 0, 0, SCREEN_W - 1, SCREEN_H - 1, 0);
    MapDrawFG(g_bBuffer, g_nMapX, g_nMapY, 0, 0, SCREEN_W - 1, SCREEN_H - 1, 1);

    // Draw player, if alive
    if (g_sPlayer -> alive) {
        if (g_sPlayer -> dir) {
            draw_sprite(g_bBuffer, g_sPlayer -> bitmaps[g_sPlayer -> frame], g_sPlayer -> x - g_nMapX, g_sPlayer -> y - g_nMapY);
        } else {
            draw_sprite_h_flip(g_bBuffer, g_sPlayer -> bitmaps[g_sPlayer -> frame], g_sPlayer -> x - g_nMapX, g_sPlayer -> y - g_nMapY);
        }
    } else {
        // Game is over, check why
        if (g_bVictory) {
            textout_centre_ex(g_bBuffer, font, "VICTORY!",
                SCREEN_W / 2, SCREEN_H / 2, makecol(0, 255, 0), -1);
        } else {
            textout_centre_ex(g_bBuffer, font, "You have died!",
                SCREEN_W / 2, SCREEN_H / 2, makecol(255, 0, 0), -1);
        }

        if (key[KEY_ENTER] || key[KEY_SPACE]) {
            // Reload the map
            MapFreeMem();
            MapLoad("map.fmp");
            // Reset the player
            playerReset();
        }
    }

    // Draw other actors, if alive and in view
    for (i = 1; i < g_nActors; i++) {
        if (g_sActors[i] -> alive && actorVisible(i)) {
            if (g_sActors[i] -> dir) {
                draw_sprite_h_flip(g_bBuffer, g_sActors[i] -> bitmaps[g_sActors[i] -> frame], g_sActors[i] -> x - g_nMapX, g_sActors[i] -> y - g_nMapY);
            } else {
                draw_sprite(g_bBuffer, g_sActors[i] -> bitmaps[g_sActors[i] -> frame], g_sActors[i] -> x - g_nMapX, g_sActors[i] -> y - g_nMapY);
            }
        }
    }

    // Display score	
    textprintf_ex(g_bBuffer, font, 10, 10, makecol(255, 255, 255), -1, "Score: %i", g_nPlayerScore);
    textprintf_ex(g_bBuffer, font, 10, 20, makecol(255, 255, 255), -1, "High Score: %i", g_nHighScore);
    textprintf_ex(g_bBuffer, font, 10, 30, makecol(255, 255, 255), -1, "Time: %i", g_nTimeLeft);
}

/**
 * Displays a step in the title screen loop
 */
int titleStep() {
    static BITMAP * bLogo = NULL;

    // Load logo when needed
    if (bLogo == NULL)
        bLogo = (BITMAP * ) g_dData[LOGO_BMP].dat;

    // Draw logo as a sprite
    draw_sprite(g_bBuffer, bLogo, SCREEN_W / 2 - bLogo -> w / 2, SCREEN_H / 3);

    // Display instructions
    textout_centre_ex(g_bBuffer, font, "Please SPACE or ENTER to start", SCREEN_W / 2, SCREEN_H / 2, makecol(200, 200, 200), -1);
    textout_centre_ex(g_bBuffer, font, "Please Ctrl-H for help", SCREEN_W / 2, SCREEN_H / 2 + 50, makecol(200, 200, 200), -1);

    // Display copyright
    textout_centre_ex(g_bBuffer, font, "Copyright (C) 2019 Lionel Pinkhard. All Rights Reserved.", SCREEN_W / 2, SCREEN_H * 3 / 4, makecol(255, 255, 255), -1);

    // Continue to gameplay
    if (key[KEY_SPACE] || key[KEY_ENTER]) {
        // Clean up first
        bLogo = NULL;
        return MODE_GAMEPLAY;
    }

    // Stay in intro
    return MODE_INTRO;
}

/**
 * Displays the help screen
 */
int helpStep() {
    int nTitleColor = makecol(0xf9, 0xef, 0x06); // Color of the title text
    int nTextColor = makecol(0xf9, 0xef, 0xf6); // Color of the other text

    // Clear buffer
    clear_bitmap(g_bBuffer);

    // Display text
    textout_centre_ex(g_bBuffer, font, "HELP", SCREEN_W / 2, SCREEN_H * 0.15, nTitleColor, -1);

    // Display help
    textout_centre_ex(g_bBuffer, font, "CONTROLS", SCREEN_W / 2, SCREEN_H * 0.25, nTextColor, -1);
    textout_ex(g_bBuffer, font, "Left arrow / A    Move to the left", SCREEN_W / 3 - 20, SCREEN_H * 0.3, nTextColor, -1);
    textout_ex(g_bBuffer, font, "Right arrow / D   Move to the left", SCREEN_W / 3 - 20, SCREEN_H * 0.35, nTextColor, -1);
    textout_ex(g_bBuffer, font, "Up arrow / W      Jump", SCREEN_W / 3 - 20, SCREEN_H * 0.4, nTextColor, -1);
    textout_ex(g_bBuffer, font, "Ctrl-H            Display help", SCREEN_W / 3 - 20, SCREEN_H * 0.5, nTextColor, -1);
    textout_ex(g_bBuffer, font, "Ctrl-M            Toggle music", SCREEN_W / 3 - 20, SCREEN_H * 0.55, nTextColor, -1);

    textout_centre_ex(g_bBuffer, font, "SCORING", SCREEN_W / 2, SCREEN_H * 0.6, nTextColor, -1);
    textout_centre_ex(g_bBuffer, font, "You gain points by collecting gems and finding the torch", SCREEN_W / 2, SCREEN_H * 0.65, nTextColor, -1);
    textout_centre_ex(g_bBuffer, font, "Avoid touching the deadly plants", SCREEN_W / 2, SCREEN_H * 0.75, nTextColor, -1);

    textout_centre_ex(g_bBuffer, font, "Press SPACE or ENTER to continue playing...", SCREEN_W / 2, SCREEN_H * 0.85, nTextColor, -1);

    // Check for key press
    if (key[KEY_ENTER] || key[KEY_SPACE]) {
        return MODE_GAMEPLAY;
    }

    return MODE_HELP;
}

/**
 * Handles times
 */
void * timeThread(void * data) {
    int my_thread_id = * ((int * ) data);
    int i;

    while (!g_bExiting) {
        if (g_nMode == MODE_GAMEPLAY) {
            // Lock mutex and process time
            pthread_mutex_lock( & threadsafe);

            // Kill player if time runs out		
            if (g_nTimeLeft > 0) {
                g_nTimeLeft--;

                if (g_nTimeLeft <= 0) {
                    if (g_sPlayer -> alive) {
                        g_sPlayer -> alive = 0;
                        play_sample(g_sDie, 250, 128, 1000, 0);
                    }
                }
            }

            pthread_mutex_unlock( & threadsafe);

            // Wait for next second
            rest(1000);
        }
    }

    pthread_exit(NULL);
    return NULL;
}

/**
 * Handles the main loop for the game, regardless of current state
 */
void gameLoop() {
    static int bMusic = 1;

    // Main game loop
    while (!key[KEY_ESC]) {
        // Check for help key or music key
        if (key_shifts & KB_CTRL_FLAG) {
            if (key[KEY_H]) {
                // Switch to help mode
                g_nMode = MODE_HELP;

                // Slight wait
                rest(10);
            } else if (key[KEY_M]) {
                if (bMusic) {
                    // Pause music
                    midi_pause();
                    bMusic = 0;
                } else {
                    // Resumse music
                    midi_resume();
                    bMusic = 1;
                }
                // Slight wait
                rest(10);
            }
        }

        // Draw the right contents
        switch (g_nMode) {
        case MODE_INTRO:
            g_nMode = titleStep();
            break;
        case MODE_GAMEPLAY:
            // Perform a step in the game
            /*if (pthread_mutex_lock(&threadsafe))
            { */
            gameStep();
            //pthread_mutex_unlock(&threadsafe);
            //}
            break;
        case MODE_HELP:
            g_nMode = helpStep();
            break;
        }

        // Draw buffer to screen
        vsync();
        acquire_screen();
        blit(g_bBuffer, screen, 0, 0, 0, 0, SCREEN_W - 1, SCREEN_H - 1);
        release_screen();
    }
}

/**
 * Main entry point for the game
 */
int main(void) {
    BITMAP * tmp; // Temporary bitmap
    MIDI * mMusic; // Background music
    FILE * fp; // File pointer for scores
    int i, j; // Index counters

    // Threading for timer
    pthread_t pthread0;
    int threadid0 = 0;

    // Initialize Allegro
    allegro_init();

    // Set up display
    set_color_depth(16);
    set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0);

    // Set up keyboard and timer
    install_keyboard();
    install_timer();

    // Set up sound
    if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, "") != 0) {
        allegro_message("Error initializing the sound system: %s", allegro_error);
        return 1;
    }

    // Load the data file
    g_dData = load_datafile("game.dat");

    // Load the map
    MapLoad("map.fmp");

    // Set up sprites
    for (i = 0; i < g_nActors; i++)
        g_sActors[i] = malloc(sizeof(SPRITE));

    // Player is also an actor
    g_sPlayer = g_sActors[0];

    // Load player sprite frames
    tmp = (BITMAP * ) g_dData[PLAYER_BMP].dat;
    for (i = 0; i < 8; i++) {
        g_sPlayer -> bitmaps[i] = grabFrame(tmp, 34, 42, 0, 0, 8, i);
    }

    // Load plant sprite frames
    tmp = (BITMAP * ) g_dData[PLANT_BMP].dat;
    for (i = 0; i < 3; i++) {
        g_sActors[1] -> bitmaps[i] = grabFrame(tmp, 50, 42, 0, 0, 3, i);

        // Copy frames
        g_sActors[2] -> bitmaps[i] = g_sActors[1] -> bitmaps[i];
        g_sActors[3] -> bitmaps[i] = g_sActors[1] -> bitmaps[i];
        g_sActors[4] -> bitmaps[i] = g_sActors[1] -> bitmaps[i];
    }

    // Load high score from file
    fp = fopen("score.dat", "r");
    if (fp != NULL) {
        fscanf(fp, "%d", & g_nHighScore);
        fclose(fp);
    }

    playerReset();

    // Create the memory buffer
    g_bBuffer = create_bitmap(SCREEN_W, SCREEN_H);
    clear(g_bBuffer);

    // Load and play music
    mMusic = (MIDI * ) g_dData[MUSIC_MID].dat;
    play_midi(mMusic, 1);

    // Load the sound samples
    g_sJump = (SAMPLE * ) g_dData[JUMP_WAV].dat;
    g_sDie = (SAMPLE * ) g_dData[DIE_WAV].dat;
    g_sWin = (SAMPLE * ) g_dData[WIN_WAV].dat;

    // Start time thread
    pthread_create( & pthread0, NULL, timeThread, (void * ) & threadid0);

    // Enter the game loop
    gameLoop();

    // Busy exiting, thread cleanup
    g_bExiting = 1;
    rest(200);
    pthread_mutex_destroy( & threadsafe);

    // Free the memory buffer
    destroy_bitmap(g_bBuffer);

    // Free the map
    MapFreeMem();

    // Free sprites
    for (i = 0; i < g_nActors; i++) {
        free(g_sActors[i]);
    }

    unload_datafile(g_dData);

    // Sound cleanup
    stop_midi();
    remove_sound();

    // Shut down Allegro
    allegro_exit();

    return 0;
}
END_OF_MAIN()
