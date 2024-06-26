#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
//#include "gba.h" // header

// Define background image
#include "background.h"
//define tile maps
#include "map.h"
//define sprite image
#include "player.h"
// Define sprite data
//#include "sprite_data.h"

//Width and height of screen
#define WIDTH 240
#define HEIGHT 160

//Define palette size
#define PALETTE_SIZE 256
//enable palette
volatile unsigned short* bg_palette = (volatile unsigned short*) 0x5000000;

//enable display control
volatile unsigned long* display_control = (volatile unsigned long*) 0x4000000;

//Define the three tile modes
#define MODE0 0x00
#define MODE1 0x01
#define MODE2 0x02

//define control registers for four tile layers
volatile unsigned short* bg0_control = (volatile unsigned short*) 0x4000008;
volatile unsigned short* bg1_control = (volatile unsigned short*) 0x400000a;
volatile unsigned short* bg2_control = (volatile unsigned short*) 0x400000c;
volatile unsigned short* bg3_control = (volatile unsigned short*) 0x400000e;

//enable buts for the tile layers
#define BG0_ENABLE 0x100
#define BG1_ENABLE 0x200
#define BG2_ENABLE 0x400
#define BG3_ENABLE 0x800

//define registers for background
volatile short* bg0_x_scroll = (unsigned short*) 0x4000010;
volatile short* bg0_y_scroll = (unsigned short*) 0x4000012;
volatile short* bg1_x_scroll = (unsigned short*) 0x4000014;
volatile short* bg1_y_scroll = (unsigned short*) 0x4000016;
volatile short* bg2_x_scroll = (unsigned short*) 0x4000018;
volatile short* bg2_y_scroll = (unsigned short*) 0x400001a;
volatile short* bg3_x_scroll = (unsigned short*) 0x400001c;
volatile short* bg3_y_scroll = (unsigned short*) 0x400001e;

//define sprite handling
#define SPRITE_MAP_2D 0x0
#define SPRITE_MAP_1D 0x40
#define SPRITE_ENABLE 0x1000
#define NUM_SPRITES 128

/* the memory location which controls sprite attributes */
volatile unsigned short* sprite_attribute_memory = (volatile unsigned short*) 0x7000000;

/* the memory location which stores sprite image data */
volatile unsigned short* sprite_image_memory = (volatile unsigned short*) 0x6010000;
volatile unsigned short* sprite_palette = (volatile unsigned short*) 0x5000200;

/* flag for turning on DMA */
#define DMA_ENABLE 0x80000000

/* flags for the sizes to transfer, 16 or 32 bits */
#define DMA_16 0x00000000
#define DMA_32 0x04000000

/* pointer to the DMA source location */
volatile unsigned int* dma_source = (volatile unsigned int*) 0x40000D4;

/* pointer to the DMA destination location */
volatile unsigned int* dma_destination = (volatile unsigned int*) 0x40000D8;

/* pointer to the DMA count/control */
volatile unsigned int* dma_count = (volatile unsigned int*) 0x40000DC;

//define button inputs
#define BUTTON_A (1 << 0)
#define BUTTON_B (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START (1 << 3)
#define BUTTON_RIGHT (1 << 4)
#define BUTTON_LEFT (1 << 5)
#define BUTTON_UP (1 << 6)
#define BUTTON_DOWN (1 << 7)
#define BUTTON_R (1 << 8)
#define BUTTON_L (1 << 9)

volatile unsigned short* buttons = (volatile unsigned short*) 0x04000130;


// Define assembly functions
extern void assemblyFunction1();
extern void assemblyFunction2();

struct Sprite {
    unsigned short attribute0;
    unsigned short attribute1;
    unsigned short attribute2;
    unsigned short attribute3;
};

struct Sprite sprites[NUM_SPRITES];
int next_sprite_index = 0;

enum SpriteSize {
    SIZE_8_8,
    SIZE_16_16,
    SIZE_32_32,
    SIZE_64_64,
    SIZE_16_8,
    SIZE_32_8,
    SIZE_32_16,
    SIZE_64_32,
    SIZE_8_16,
    SIZE_8_32,
    SIZE_16_32,
    SIZE_32_64
};

/* copy data using DMA */
void memcpy16_dma(unsigned short* dest, unsigned short* source, int amount) {
    *dma_source = (unsigned int) source;
    *dma_destination = (unsigned int) dest;
    *dma_count = amount | DMA_16 | DMA_ENABLE;
}

struct Sprite* sprite_init(int x, int y, enum SpriteSize size,
        int horizontal_flip, int vertical_flip, int tile_index, int priority) {

    /* grab the next index */
    int index = next_sprite_index++;

    /* setup the bits used for each shape/size possible */
    int size_bits, shape_bits;
    switch (size) {
        case SIZE_8_8:   size_bits = 0; shape_bits = 0; break;
        case SIZE_16_16: size_bits = 1; shape_bits = 0; break;
        case SIZE_32_32: size_bits = 2; shape_bits = 0; break;
        case SIZE_64_64: size_bits = 3; shape_bits = 0; break;
        case SIZE_16_8:  size_bits = 0; shape_bits = 1; break;
        case SIZE_32_8:  size_bits = 1; shape_bits = 1; break;
        case SIZE_32_16: size_bits = 2; shape_bits = 1; break;
        case SIZE_64_32: size_bits = 3; shape_bits = 1; break;
        case SIZE_8_16:  size_bits = 0; shape_bits = 2; break;
        case SIZE_8_32:  size_bits = 1; shape_bits = 2; break;
        case SIZE_16_32: size_bits = 2; shape_bits = 2; break;
        case SIZE_32_64: size_bits = 3; shape_bits = 2; break;
    }

    int h = horizontal_flip ? 1 : 0;
    int v = vertical_flip ? 1 : 0;

    /* set up the first attribute */
    sprites[index].attribute0 = y |             /* y coordinate */
        (0 << 8) |          /* rendering mode */
        (0 << 10) |         /* gfx mode */
        (0 << 12) |         /* mosaic */
        (1 << 13) |         /* color mode, 0:16, 1:256 */
        (shape_bits << 14); /* shape */

    /* set up the second attribute */
    sprites[index].attribute1 = x |             /* x coordinate */
        (0 << 9) |          /* affine flag */
        (h << 12) |         /* horizontal flip flag */
        (v << 13) |         /* vertical flip flag */
        (size_bits << 14);  /* size */

    /* setup the second attribute */
    sprites[index].attribute2 = tile_index |   // tile index */
        (priority << 10) | // priority */
        (0 << 12);         // palette bank (only 16 color)*/

    /* return pointer to this sprite */
    return &sprites[index];
}

/* update all of the spries on the screen */
void sprite_update_all() {
    /* copy them all over */
    memcpy16_dma((unsigned short*) sprite_attribute_memory, (unsigned short*) sprites, NUM_SPRITES * 4);
}

/* setup all sprites */
void sprite_clear() {
    /* clear the index counter */
    next_sprite_index = 0;

    /* move all sprites offscreen to hide them */
    for(int i = 0; i < NUM_SPRITES; i++) {
        sprites[i].attribute0 = HEIGHT;
        sprites[i].attribute1 = WIDTH;
    }
}

/* set a sprite postion */
void sprite_position(struct Sprite* sprite, int x, int y) {
    /* clear out the y coordinate */
    sprite->attribute0 &= 0xff00;

    /* set the new y coordinate */
    sprite->attribute0 |= (y & 0xff);

    /* clear out the x coordinate */
    sprite->attribute1 &= 0xfe00;

    /* set the new x coordinate */
    sprite->attribute1 |= (x & 0x1ff);
}

/* move a sprite in a direction */
void sprite_move(struct Sprite* sprite, int dx, int dy) {
    /* get the current y coordinate */
    int y = sprite->attribute0 & 0xff;

    /* get the current x coordinate */
    int x = sprite->attribute1 & 0x1ff;

    /* move to the new location */
    sprite_position(sprite, x + dx, y + dy);
}

/* change the vertical flip flag */
void sprite_set_vertical_flip(struct Sprite* sprite, int vertical_flip) {
    if (vertical_flip) {
        /* set the bit */
        sprite->attribute1 |= 0x2000;
    } else {
        /* clear the bit */
        sprite->attribute1 &= 0xdfff;
    }
}

/* change the vertical flip flag */
void sprite_set_horizontal_flip(struct Sprite* sprite, int horizontal_flip) {
    if (horizontal_flip) {
        /* set the bit */
        sprite->attribute1 |= 0x1000;
    } else {
        /* clear the bit */
        sprite->attribute1 &= 0xefff;
    }
}

/* change the tile offset of a sprite */
void sprite_set_offset(struct Sprite* sprite, int offset) {
    /* clear the old offset */
    sprite->attribute2 &= 0xfc00;

    /* apply the new one */
    sprite->attribute2 |= (offset & 0x03ff);
}

/* setup the sprite image and palette */
void setup_sprite_image() {
    /* load the palette from the image into palette memory*/
    memcpy16_dma((unsigned short*) sprite_palette, (unsigned short*) player_palette, PALETTE_SIZE);

    /* load the image into sprite image memory */
    memcpy16_dma((unsigned short*) sprite_image_memory, (unsigned short*) player_data, (player_width * player_height) / 2);
}

/* a struct for the player's logic and behavior */
struct Player {
    /* the actual sprite attribute info */
    struct Sprite* sprite;

    /* the x and y postion in pixels */
    int x, y;

    /* the player's y velocity in 1/256 pixels/second */
    int yvel;

    /* the player's y acceleration in 1/256 pixels/second^2 */
    int gravity; 

    /* which frame of the animation he is on */
    int frame;

    /* the number of frames to wait before flipping */
    int animation_delay;

    /* the animation counter counts how many frames until we flip */
    int counter;

    /* whether the player is moving right now or not */
    int move;

    /* the number of pixels away from the edge of the screen the player stays */
    int border;

    /* if the player is currently falling */
    int falling;
};

/* initialize the player */
void player_init(struct Player* player) {
    player->x = 100;
    player->y = 113;
    player->yvel = 0;
    player->gravity = 50;
    player->border = 90;
    player->frame = 0;
    player->move = 0;
    player->counter = 0;
    player->falling = 0;
    player->animation_delay = 8;
    player->sprite = sprite_init(player->x, player->y, SIZE_16_32, 0, 0, player->frame, 0);
}

/* move the player left or right returns if it is at edge of the screen */
int player_left(struct Player* player) {
    /* face left */
    sprite_set_horizontal_flip(player->sprite, 1);
    player->move = 1;

    /* if we are at the left end, just scroll the screen */
    if (player->x < player->border) {
        return 1;
    } else {
        /* else move left */
        player->x--;
        return 0;
    }
}

int player_right(struct Player* player) {
    /* face right */
    sprite_set_horizontal_flip(player->sprite, 0);
    player->move = 1;

    /* if we are at the right end, just scroll the screen */
    if (player->x > (WIDTH - 16 - player->border)) {
        return 1;
    } else {
        /* else move right */
        player->x++;
        return 0;
    }
}

/* stop the player from walking left/right */
void player_stop(struct Player* player) {
    player->move = 0;
    player->frame = 0;
    player->counter = 7;
    sprite_set_offset(player->sprite, player->frame);
}

/* start the player jumping, unless already fgalling */
void player_jump(struct Player* player) {
    if (!player->falling) {
        player->yvel = -1350;
        player->falling = 1;
    }
}

/* finds which tile a screen coordinate maps to, taking scroll into acco  unt */
unsigned short tile_lookup(int x, int y, int xscroll, int yscroll,
        const unsigned short* tilemap, int tilemap_w, int tilemap_h) {

    /* adjust for the scroll */
    x += xscroll;
    y += yscroll;

    /* convert from screen coordinates to tile coordinates */
    x >>= 3;
    y >>= 3;

    /* account for wraparound */
    while (x >= tilemap_w) {
        x -= tilemap_w;
    }
    while (y >= tilemap_h) {
        y -= tilemap_h;
    }
    while (x < 0) {
        x += tilemap_w;
    }
    while (y < 0) {
        y += tilemap_h;
    }

    /* the larger screen maps (bigger than 32x32) are made of multiple stitched
       together - the offset is used for finding which screen block we are in
       for these cases */
    int offset = 0;

    /* if the width is 64, add 0x400 offset to get to tile maps on right   */
    if (tilemap_w == 64 && x >= 32) {
        x -= 32;
        offset += 0x400;
    }

    /* if height is 64 and were down there */
    if (tilemap_h == 64 && y >= 32) {
        y -= 32;

        /* if width is also 64 add 0x800, else just 0x400 */
        if (tilemap_w == 64) {
            offset += 0x800;
        } else {
            offset += 0x400;
        }
    }

    /* find the index in this tile map */
    int index = y * 32 + x;

    /* return the tile */
    return tilemap[index + offset];
}

/* update the player */
void player_update(struct Player* player, int xscroll) {
    /* update y position and speed if falling */
    if (player->falling) {
        player->y += (player->yvel >> 8);
        player->yvel += player->gravity;
    }

    /* check which tile the player's feet are over */
    unsigned short tile = tile_lookup(player->x + 8, player->y + 32, xscroll, 0, map,
            map_width, map_height);

    /* if it's block tile
     * these numbers refer to the tile indices of the blocks the player can walk on */
    if ((tile >= 1 && tile <= 6) || 
            (tile >= 12 && tile <= 17)) {
        /* stop the fall! */
        player->falling = 0;
        player->yvel = 0;

        /* make him line up with the top of a block works by clearing out the lower bits to 0 */
        player->y &= ~0x3;

        /* move him down one because there is a one pixel gap in the image */
        player->y++;

    } else {
        /* he is falling now */
        player->falling = 1;
    }


    /* update animation if moving */
    if (player->move) {
        player->counter++;
        if (player->counter >= player->animation_delay) {
            player->frame = player->frame + 16;
            if (player->frame > 16) {
                player->frame = 0;
            }
            sprite_set_offset(player->sprite, player->frame);
            player->counter = 0;
        }
    }

    /* set on screen position */
    sprite_position(player->sprite, player->x, player->y);
}

//define scanline counter
volatile unsigned short* scanline_counter = (volatile unsigned short*) 0x4000006;

void waitForVBlank(){
    while(*scanline_counter < 160){
    }
}
//return pointer to a char block
volatile unsigned short* char_block(unsigned long block) {
     return (volatile unsigned short*) (0x6000000 + (block * 0x4000));
}


//return pointer screen block
volatile unsigned short* screen_block(unsigned long block){
    return (volatile unsigned short*) (0x6000000 + (block * 0x800));
}

unsigned char button_pressed(unsigned short button){
    unsigned short pressed = *buttons & button; 
    
    if(pressed == 0){
        return 1; 
    }else{
        return 0;
    }
}

void delay (unsigned int amount){
    for(int i = 0; i < amount * 10; i++){}
} 

void setup_background(){

       /* load the palette from the image into palette memory*/
    memcpy16_dma((unsigned short*) bg_palette, (unsigned short*) background_palette, PALETTE_SIZE);

    /* load the image into char block 0 */
    memcpy16_dma((unsigned short*) char_block(0), (unsigned short*) background_data,
            (background_width * background_height) / 2);

    *bg0_control = 0 |
       (0 << 2) | 
       (0 << 6) |
       (1 << 7) |
       (16 << 8) |  
       (1 << 13) | 
       (1 << 14);
          
      /* load the tile data into screen block 16 */
   
    memcpy16_dma((unsigned short*) screen_block(16), (unsigned short*) map, map_width * map_height);
}


/*Used to detect collisions*/
int get_pos(struct Player* player, bool collision){
    if(player->x == 135 && player->y==113 && collision == true){ //Detects if player is still on ground when in contact with a block
      return 1;
    }else{
        return 0;
    }
}

int main() {
    *display_control = MODE0 | BG0_ENABLE | SPRITE_ENABLE | SPRITE_MAP_1D; 
    // Load background
    setup_background();
    setup_sprite_image();
    sprite_clear();
    struct Player player;
    player_init(&player);
    int xscroll = 0;
    bool collision = false; //Boolean to detect when a collision is in sync with the player
    bool start = true; //Boolean to determine if it is the start of the game or not
    int index = 0; //Index variable to count distance between blocks
    int counter = 130; //Counter variable to determine if the block and the player are in the same pixel
    // Main game loop
    while (1) {
        if(index == 100 && start){ //Used to detect collisions when the game first starts because scrolling is a little different
            index = 0;
            collision = true;
            start = false;
        }
        if(index == counter && !start){ //Used to detect if the player comes into contact with one of the blocks on the ground
            index = 0;
            collision = true;
            counter--;
        }
        if(counter == 0){ //If counter hits 0 it resets back to 130
            counter = 130;
        }
        player_update(&player, xscroll);
        // Handle input
        player_right(&player);
        xscroll++;
        if(get_pos(&player,collision)){
        while(1){ 
                player_stop(&player);
            }
        }   
        if (button_pressed(BUTTON_A)){
            player_jump(&player);
        }
        waitForVBlank();
        *bg0_x_scroll = xscroll;
        *bg1_x_scroll = xscroll;
        sprite_update_all();
        delay(300);
        index++;
        collision = false;
    }

    return 0;
}
/*
// Assembly function 1
__asm__(
".global assemblyFunction1\n"
"assemblyFunction1:\n"
"    //  assembly code here\n"
"    // Make sure it's at least 8 lines long\n"
"    bx lr\n"
);

// Assembly function 2
__asm__(
".global assemblyFunction2\n"
"assemblyFunction2:\n"
"    // assembly code here\n"
"    // Make sure it's at least 8 lines long\n"
"    bx lr\n"
);
*/
