/*! ----------------------------------------------------------------------------
 *  @file    main.c
 *  @brief   This example demonstrates how to enable DW IC GPIOs as inputs and outputs.
 *           And drive the output to turn on/off LED on EVB1000 HW.
 *
 *           GPIO2 will be used to flash the RXOK LED (LED4 on EVB1000 HW)
 *           GPIO5 and GPIO6 are configured as inputs, toggling S3-3 and S3-4 will change them:
 *           S3-3 is connected to GPIO5 and S3-4 to GPIO6
 *
 *           NOTE!!! The switch S3-3 and S3-4 on EVB1000 HW should be OFF before the example is run
 *           to make sure the DW1000 SPI mode is set to 0 on IC start up.
 * @attention
 *
 * Copyright 2017 (c) Decawave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author Decawave
 */
#include "deca_device_api.h"
#include "deca_regs.h"
#include "deca_spi.h"
#include "port.h"
#include "sleep.h"
#include "lcd.h"

/* Example application name and version to display on LCD screen. */
#define APP_NAME "GPIO        v1.0"

/**
 * Application entry point.
 */
int main(void)
{
    /* Display application name on LCD. */
    lcd_display_str(APP_NAME);

    /* During initialisation and continuous frame mode activation, DW1000 clocks must be set to crystal speed so SPI rate have to be lowered and will
     * not be increased again in this example. */
    spi_set_rate_low();


    /* NOTE!!! The switch S3-3 and S3-4 on EVB1000 HW should be OFF at this point
     * to make sure the DW1000 SPI mode is set to 0 on IC start up
     */

    /* Reset and initialise DW1000 */
    reset_DW1000(); /* Target specific drive of RSTn line into DW1000 low for a period. */
    if (dwt_initialise(DWT_LOADNONE) == DWT_ERROR)
    {
        lcd_display_str("INIT FAILED");
        while (1)
        { };
    }
    spi_set_rate_high();

    /* see NOTE 1: 1st enable GPIO clocks */
    dwt_enablegpioclocks();

    /*
     * GPIO2 will be used to flash the RXOK LED (LED4 on EVB1000 HW)
     *
     * GPIO5 and GPIO6 are configured as inputs, toggling S3-3 and S3-4 will change their values:
     * S3-3 is connected to GPIO5 and S3-4 to GPIO6
     *
     */

    dwt_setgpiodirection(DWT_GxM2 | DWT_GxM6 | DWT_GxM5, DWT_GxP6 | DWT_GxP5);

    while(1)
    {

        dwt_setgpiovalue(DWT_GxM2, DWT_GxP2); /* set GPIO2 high (LED4 will light up)*/

        /* set GPIO6 is high use short Sleep ON period*/
        if(dwt_getgpiovalue(DWT_GxP6))
            sleep_ms(100);
        else
            sleep_ms(400);

        dwt_setgpiovalue(DWT_GxM2, 0); /* set GPIO2 low (LED4 will be off)*/

        /* set GPIO5 is high use short Sleep OFF period*/
        if(dwt_getgpiovalue(DWT_GxP5))
            sleep_ms(100);
        else
            sleep_ms(400);

    }

}

/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. When enabling the GPIO mode/value, the GPIO clock needs to be enabled and GPIO reset set:
 *
 ****************************************************************************************************************************************************/
