#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"


//
// The SNES controller looks like this:
//
//   -----------------        1: VCC       4: Data
//  | 1 2 3 4 | 5 6 7 )       2: Clock     7: Ground
//   -----------------        3: Latch
//
// Hook up the VCC to the 3V3 line, Ground to any ground
// pin, and the Clock/Latch/Data to three GPIO pins (don't
// need resistors or anything). The GPIOs you've used need
// to be specified in the constants below
//
const uint LATCH = 15;
const uint CLOCK = 14;
const uint SDATA = 16;


//
// Controller buttons will be stored in this array, 1 = on
// and 0 = off.
//
// The order of buttons is:
// B Y Select Start Up Down Left Right A X LTrigger RTrigger
// 
uint controller[16];


void snes_init();
void snes_fetch();
void game_iter();


/**
 * Simple main loop
 *
 * Polls controller once every 16.66ms and also executes
 * the game_iter() method at the same frequency; this
 * simulates the polling rate of a real SNES device.
 */
int main()
{
    stdio_init_all();
    snes_init();

    uint start;
    while (true) {
        start = time_us_32();
        snes_fetch();
        game_iter();
        sleep_us(16666 - (time_us_32() - start));
    }
}


/**
 * Init the GPIO as needed to fetch SNES controller data
 */
void snes_init()
{
    gpio_init(LATCH);
    gpio_init(CLOCK);
    gpio_init(SDATA);

    gpio_set_dir(LATCH, GPIO_OUT);
    gpio_set_dir(CLOCK, GPIO_OUT);
    gpio_set_dir(SDATA, GPIO_IN);

    gpio_put(CLOCK, 1);
}


/**
 * Request an update from the SNES controller
 *
 * Will take about 210 microseconds to execute and results
 * are stored in the 'controller' array.
 *
 * Traditionally this is called every 16ms but the controller
 * doesn't seem to mind if it's less often; I haven't tested
 * more often, it's probably okay until you cook the chips.
 */
void snes_fetch()
{
    gpio_put(LATCH, 1);
    sleep_us(12);
    gpio_put(LATCH, 0);

    sleep_us(6);

    for (uint btn = 0; btn < 16; ++btn) {
        busy_wait_us(3);
        controller[btn] = !gpio_get(SDATA);
        busy_wait_us(3);
        gpio_put(CLOCK, 1);
        busy_wait_us(6);
        gpio_put(CLOCK, 0);
    }
}


/****  SIMPLE "game" WHERE A BOX MOVES AROUND THE SCREEN  ****/

float playerx = 20.0;
float playery = 10.0;

/**
 * Draw a "cell" in the 20x40 gameplay world
 */
char draw_cell(uint x, uint y)
{
    if (y == 0 || y == 19 || x == 0 || x == 39) {
        return '*';
    } else if ((int)playerx == x && (int)playery == y) {
        return '#';
    } else {
        return ' ';
    }
}

/**
 * A single iteration of the game loop. Called ~ 60 times per second
 */
void game_iter()
{
    // Clear screen and show debug output of all inputs
    printf("\x1B[2J");
    printf(
        " B=%i Y=%i SL=%i ST=%i U=%i D=%i L=%i R=%i A=%i X=%i LP=%i RP=%i ",
        controller[0], controller[1], controller[2], controller[3], controller[4], controller[5],
        controller[6], controller[7], controller[8], controller[9], controller[10], controller[11]
    );
    printf("\n\n");

    // Handle input
    if (controller[4]) playery -= 0.1;
    if (controller[5]) playery += 0.1;
    if (controller[6]) playerx -= 0.1;
    if (controller[7]) playerx += 0.1;

    // Draw the grid of cells
    char cells[20 * 42];
    char* ptr = cells;
    for (uint y = 0; y < 20; ++y) {
        for (uint x = 0; x < 40; ++x) {
            *ptr++ = draw_cell(x, y);
        }
        *ptr++ = '\n';
    }
    *ptr++ = '\0';
    printf(cells);
}

