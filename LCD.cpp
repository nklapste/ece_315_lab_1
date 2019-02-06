/**
 * LCD.cpp
 *
 *  Created on: Sep 22, 2016
 *      Author: Nancy Minderman
 *      nancy.minderman@ualberta.ca
 *      Much of the Initialization Code and raw
 *      font tables were taken from the Arduino code
 *      provided by Sparkfun
 *      https://www.sparkfun.com/products/10168
 *      Arduino sketch code
 *      http://playground.arduino.cc/Code/PCD8544
 *
 * Modified on: January 29, 2019
 *      Author: Nathan Klapstein
 */

#include "LCD.h"
#include "point.h"
#include <dspi.h>
#include <pins.h>
#include <basictypes.h>
#include <stdio.h>
#include <constants.h>
#include "error_wrapper.h"

#define __DEBUG_DSPI

/* custom definitions to improve readability */
#define MIN_DELAY 1
#define CMD_MODE 0  // set LCD_C_D_LINE to this for command mode
#define CMD_SIZE 1  // commands are exactly 1 byte in size
#define DATA_MODE 1 // set LCD_C_D_LINE to this for data transfer mode

LCD::LCD() {
    // TODO Auto-generated constructor stub

}

LCD::~LCD() {
    // TODO Auto-generated destructor stub
}

/**
 * Initializes both the DQSPI module on the MOD54415 and the LCD controller.
 */
void LCD::Init(void) {

    init_spi();
    init_lcd();
}

/**
 * Clears the LCD by sending a screen full of space characters.
 */
void LCD::Clear(void) {
    // clear_array is in bitmaps.h it is a 48 X 84 pixels (504 Bytes) array of space characters.
    send_data((BYTE *) clear_array, SCREEN_SIZE);
}

/**
 * Inverts every pixel on screen from white to black and vice versa.
 *
 * @note The data in memory on the LCD does not change.
 */
void LCD::Invert(void) {
    static BOOL inverted = false;
    if (inverted) {
        send_cmd(CMD_FUNCTION_SET);
//        send_cmd(0x0C);
        send_cmd(CMD_DISPLAY_CONTROL | OPT_NORMAL);

        inverted = false;
    } else {
        send_cmd(CMD_FUNCTION_SET);
//        send_cmd(0x0D);
        send_cmd(CMD_DISPLAY_CONTROL | OPT_INVERSE);

        inverted = true;
    }
}

/**
 * Sends a bitmap of raw data to the screen starting at the upper left
 * hand pixel of the screen.
 *
 * @param bitmap {@code const BYTE *} bitmap that is 504 Bytes in size.
 */
void LCD::DrawBitmap(const BYTE *bitmap) {
    Home();
    send_data((BYTE *) bitmap, SCREEN_SIZE);
}

/**
 * Sends a c-style string to the display at the current index.
 *
 * @note String must be null terminated
 * @note All elements in the string must be ASCII characters > space (0x20).
 * @warning Do not send non-printable characters. Bad things will happen.
 *
 * @param str {@code char *} address of first character in string.
 *      String will be displayed at fixed locations on screen determined
 *      by points in ASCII_7 array in bitmaps.h
 *
 * @see bitmaps.h for the Font table
 */
void LCD::DrawString(char *str) {
    BYTE index = 0;
    while ((*(str)) != '\0') {
        index = *str - ASCII_BASE; // convert ASCII character to index in font table
        DrawChar((const BYTE *) &(ASCII_7[index]));
        str++;
    }
}

/**
 * Draws a 7 pixel (width) by 8 pixel (height) sprite at the current location.
 *
 * @note Use the font tables in bitmaps.h to select a sprite.
 *
 * @param ch {@code const BYTE *} array of raw data that represents the character.
 *
 * @see bitmaps.h for the Font table
 */
void LCD::DrawChar(const BYTE *ch) {
    send_data((BYTE *) ch, CHAR_SIZE);
}

/**
 * Draws a 7 pixel(width) by 8 pixel( height) sprite at the provided location.
 *
 * @param ch {@code const BYTE *} array of raw data that represents the character.
 * @param loc {@code point} one of the locations in the char_index array in bitmaps.h
 *
 * @see bitmaps.h for the char_index array
 */
void LCD::DrawChar(const BYTE *ch, point loc) {
    move(loc);
    send_data((BYTE *) ch, CHAR_SIZE);
}

/**
 * Sends a fixed 84 X 48 pixels = 504 Bytes bitmap to the screen
 *
 * @note This method should display a funny xkcd comic.
 */
void LCD::TestLCD(void) {
    Home();
    send_data((BYTE *) xkcdSandwich, SCREEN_SIZE);
}

/**
 * Move the internal counter of the LCD to the upper left
 * hand point of the LCD.
 */
void LCD::Home(void) {
    point origin = char_index[LINE1_ORIGIN];
    move(origin);
}

/**
 * Moves te internal counter of the LCD to the location specified.
 *
 * @note To write characters limit the values of point to the members of the
 * char_index array in bitmaps.h. No checks are made.
 *
 * @warning If you overflow the x and y counters you will get unexpected results.
 *
 * @param loc {@code point} location to move to
 *
 * @see bitmaps.h for the char_index array
 */
void LCD::Move(point loc) {
    move(loc);
}

/**
 * Initializes the SPI module on the 54415 to match the requirements of the
 * NOKIA 51150 with PCD8544 LCD controller.
 */
void LCD::init_spi(void) {
    LCD_CLOCK.function(PINJ2_25_DSPI1_SCK);      // set SPI clock function on pin J2[25]
    LCD_DATA_OUT.function(PINJ2_28_DSPI1_SOUT);  // set SPI Data out function on pin J2[28]
    LCD_RESET.function(PINJ2_26_GPIO);           // use this as a programmable reset for the LCD reset
    LCD_C_D_LINE.function(PINJ2_27_GPIO);        // use this as the D/C line 0 = command, 1 = data

    display_error("LCD::init_spi SemInit\n", OSSemInit(&DSPI_SEM, 0));  // Use sem to be notified when transfer is finished.
    /* Initialize DSPI options
       void DSPIInit( BYTE SPIModule = DEFAULT_DSPI_MODLE, DWORD Baudrate = 2000000,
                   BYTE QueueBitSize = 0x8, BYTE CS = 0x0F,
                   BYTE CSPol = 0x0F, BYTE ClkPolarity = 0x0,
                   BYTE ClkPhase = 0x1, BOOL DoutHiz = TRUE,
                   BYTE csToClockDelay = 0, BYTE delayAfterTransfer = 0 ) */
    DSPIInit(DEFAULT_DSPI_MODULE, // DEFAULT is SPI 1 so OK
        2000000,  // Speed
        8,        // Queue bit size = 8 bits
        0x0,      // CS set these to all off. No chip selects please.
        0x1,      // CS Pol 1 = when CS is inactive, pin is pulled high
        0,        // Clock polarity logic level when inactive. Set this to 0
        0,        // Clock phase to 0 means data is captured on the rising (or leading) edge of clock
        FALSE,    // DoutHiz should be true if the DOUT line needs to be in high impedance in between transfers. Both work in our case.
        0,        // Use default for now: -QCD is a value in the QDLYR register and will change the delay between the assertion of
        // the chip select and the start of the DSPI clock.  Default setting of one half DSPI clk
        // will be used if parameter is specified as 0x0 or not included.
        0         // Use default for now: DTL is a value in the QDLYR register and will change the delay following a transfer of
        // a single WORD in the DSPI queue.  Default reset value of 17/(fsys/2) will be used if
        // parameter is specified as 0x0 or not included.
    );
}

/**
 * Sends data to the LCD, with size being the total number of bytes of data.
 * This will display the given data at the current location.
 *
 * @param data {@code const BYTE *} data to be sent to screen in raw form.
 * @param size {@code WORD} number of bytes to display
 */
void LCD::send_data(const BYTE *data, WORD size) {
//    LCD_C_D_LINE = 1; // 1 = data
    LCD_C_D_LINE = DATA_MODE; // 1 = data
    // send data via SPI bus
    DSPIStart(DEFAULT_DSPI_MODULE, (BYTE *) data, NULL, size, &DSPI_SEM);
    display_error("LCD::send_data \n", OSSemPend(&DSPI_SEM, WAIT_FOREVER));
}

/**
 * Sends a command to the LCD.
 *
 * @param command {@code BYTE} the LCD command to send
 *
 * @see LCD.h for all possible command definitions
 */
void LCD::send_cmd(BYTE command) {
//    LCD_C_D_LINE = 0;  // 0 = command
    LCD_C_D_LINE = CMD_MODE;  // 0 = command

    // send command via the SPI bus
    //  commands are exactly 1 byte in size
//    DSPIStart(DEFAULT_DSPI_MODULE, &command, NULL, 1, &DSPI_SEM);
    DSPIStart(DEFAULT_DSPI_MODULE, &command, NULL, CMD_SIZE, &DSPI_SEM);
    display_error("LCD::send_command \n", OSSemPend(&DSPI_SEM, WAIT_FOREVER));
}

/**
 * Send initialization commands to the PCD 8544 LCD controller.
 *
 * Modified to avoid using magic values. Instead this uses the
 * definitions placed within LCD.h.
 *
 * @see LCD.h for all possible command definitions
 */
void LCD::init_lcd(void) {

    LCD_RESET = 0;
    OSTimeDly(MIN_DELAY); // minimal possible delay
    LCD_RESET = 1;
    // Insert your ex 2 code modifications here
    // H = 1
//	send_cmd(0x21)
    send_cmd(CMD_FUNCTION_SET | OPT_EXT_INSTR); // set H = 1
//	send_cmd(0xB0);
//	send_cmd(CMD_SET_VOP | OPT_CONTRAST_LIGHT) // original low contrast mode
    send_cmd(CMD_SET_VOP | OPT_CONTRAST_DARK);
//	send_cmd(0x04)
    send_cmd(CMD_TEMP_CNTRL | OPT_VLCD_COEFF_0);
//	send_cmd(0x14)
    send_cmd(CMD_BIAS_SYSTEM | OPT_N_3);
    // H = 0
//	send_cmd(0x20)
    send_cmd(CMD_FUNCTION_SET | OPT_BASIC_INSTR); // set H = 0
//	send_cmd(0x0C)
    send_cmd(CMD_DISPLAY_CONTROL | OPT_NORMAL);
    // End ex 2 modifications
    move(char_index[LINE1_ORIGIN]);
}

/**
 * Set the internal counters of the LCDs to the location provided.
 *
 * @param loc {@code point} new location for internal location counters of LCD controller.
 *      For ASCII character data:
 *          - loc should be one of the char_index points in char_index in bitmaps.h
 *      For non-ASCII character data:
 *          - loc.x should be between 0 and 83
 *          - loc.y should be between 0 and 5
 *
 * @see bitmaps.h for the char_index array
 */
void LCD::move(point loc) {
    send_cmd(CMD_FUNCTION_SET);
    send_cmd(CMD_SET_Y + loc.row);
    send_cmd(CMD_SET_X + loc.col);
}
