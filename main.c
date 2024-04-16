#include <stdlib.h>
#include <stdio.h>
//#include "gba.h" // header

// Define background image
#include "background.h"
//define tile maps
#include "map.h"
#include "map2.h"
//define sprite image
#include "sprite.h"
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




// Define assembly functions
extern void assemblyFunction1();
extern void assemblyFunction2();

/* copy data using DMA */
void memcpy16_dma(unsigned short* dest, unsigned short* source, int amount) {
    *dma_source = (unsigned int) source;
    *dma_destination = (unsigned int) dest;
    *dma_count = amount | DMA_16 | DMA_ENABLE;
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

void setup_background(){

       /* load the palette from the image into palette memory*/
    memcpy16_dma((unsigned short*) bg_palette, (unsigned short*) background_palette, PALETTE_SIZE);

    /* load the image into char block 0 */
    memcpy16_dma((unsigned short*) char_block(0), (unsigned short*) background_data,
            (background_width * background_height) / 2);

    *bg0_control = 1 |
       (0 << 2) | 
       (0 << 6) |
       (1 << 7) |
       (16 << 8) |  
       (1 << 13) | 
       (0 << 14);
            /* load the tile data into screen block 16 */
    memcpy16_dma((unsigned short*) screen_block(16), (unsigned short*) map, map_width * map_height);
    *bg1_control = 0 |
        (0 << 2) |
        (0 << 6) |
        (1 << 7) |
        (17 << 8) |
        (1 << 13) |
        (0 << 14); 
        /* load the tile data into screen block 16 */
    memcpy16_dma((unsigned short*) screen_block(17), (unsigned short*) map2, map_width * map_height);

    /* load the tile data into screen block 16 */
}

int main() {
    *display_control = MODE0 | BG0_ENABLE | BG1_ENABLE; 
    // Load background
    setup_background();

    // Load sprites


    // Main game loop
    while (1) {
        // Update game logic

        // Handle input

        // Update sprites

        // Scroll background
        //scrollBackground();

        // Call assembly functions
        //assemblyFunction1();
        //assemblyFunction2();

        // Wait for VBlank
        waitForVBlank();
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
