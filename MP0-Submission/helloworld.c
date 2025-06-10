/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "platform.h"
#include "xgpiops.h"
#include "xil_printf.h"
#include <xparameters.h>
#include <unistd.h>
#include "xgpio_l.h"
#include "xgpio.h"
#include "unistd.h"


/************************** VARIABLE DEFINITIONS **************************/
XGpioPs GPIO;           /* The driver instance for GPIO device. */
XGpioPs_Config *CONFIGPTR;

/*****************************************************************************/

int main() {
    init_platform();
    int* sw_base_addr = XPAR_AXI_GPIO_2_BASEADDR;
    int* led_base_addr =  XPAR_AXI_GPIO_0_BASEADDR;
    int* btn_base_addr =  XPAR_AXI_GPIO_1_BASEADDR;
    int* nes_base_addr = XPAR_AXI_GPIO_3_BASEADDR;
    int sw_vals = 0;
    int btn_vals = 0;
    char* str[50];

    int status;
    u32 inputdata;

    XGpio myGpioInstance;

    // Initialize the GPIO instance with a specific Device ID
    if (XGpio_Initialize(&myGpioInstance, XPAR_AXI_GPIO_3_DEVICE_ID) != XST_SUCCESS) {
        xil_printf("GPIO initialization failed!\n");
        return XST_FAILURE;
    }

    // Configure the GPIO instance with the base address
    if (XGpio_CfgInitialize(&myGpioInstance, XGpio_LookupConfig(XPAR_AXI_GPIO_3_DEVICE_ID),
                            myGpioInstance.BaseAddress) != XST_SUCCESS) {
        xil_printf("GPIO configuration failed!\n");
        return XST_FAILURE;
    }


    int ctrl_vals = 0;

    // Set the data direction for Channel 1
    XGpio_SetDataDirection(&myGpioInstance, 1, 0x00000001); // Set Pin 0 as input, Pin 2 as output


    // Clock High
    XGpio_DiscreteWrite(&myGpioInstance, 1, 0x00000004); // Pin 2

    while(1){
    // Latch High 12 us
    XGpio_DiscreteWrite(&myGpioInstance, 1, 0x00000006); // Pin 1
    usleep(12);

    XGpio_DiscreteWrite(&myGpioInstance, 1, 0x00000004); // Pin 1
    usleep(6);

    // Cycle Clock
    for (int i = 0; i < 15; i++) {
        usleep(6);
        XGpio_DiscreteWrite(&myGpioInstance, 1, 0x00000000);
        ctrl_vals |= (XGpio_DiscreteRead(&myGpioInstance, 1) & 0x01) << i;
        usleep(6);
        XGpio_DiscreteWrite(&myGpioInstance, 1, 0x00000004);
        usleep(6);
    }
    xil_printf("%X\n\r", ctrl_vals);
    ctrl_vals = 0;
    }
    /* Initialize the GPIO driver. */
    /*
    CONFIGPTR = XGpioPs_LookupConfig(XPAR_XGPIOPS_0_DEVICE_ID);
    status = XGpioPs_CfgInitialize(&GPIO, CONFIGPTR, CONFIGPTR->BaseAddr);
    if (status != XST_SUCCESS)
    {
        return XST_FAILURE;
    }

    // GPIO_PS
    XGpioPs_SetDirectionPin(&GPIO, 51, 0);
    XGpioPs_SetOutputEnablePin(&GPIO, 51, 0);

    while(1){
    // PS_GPIO
    	inputdata = XGpioPs_ReadPin(&GPIO, 51);
    	sprintf(str, "Value : %d\n\r", (int)inputdata);
    	print(str);
    	sleep(0.1);
    	if(inputdata != 0)
    		break;
    	print("not pressed !\n\r");
    }

    print("pressed !\n\r");
    */

    for (int i = 0; i < 100000; i++) {
        //sw_vals = *sw_base_addr;
        //btn_vals = *btn_base_addr;
        //*led_base_addr = btn_vals;
        //sprintf(str, "%X\n\r", btn_vals);
       // print(str);
       *nes_base_addr = 8;
       sleep(2);
       *nes_base_addr = 0;
       sleep(0.001);
       xil_printf("CTLR : %X\n\r", *nes_base_addr);
    }

    /*
    int on_off = 0;
    while(1){
    	sleep(0.001);
    	*led_base_addr = on_off;

    	if(on_off == 0){
    		on_off = 1;
    	}else{
    		on_off = 0;
    	}
    }
    */

    cleanup_platform();
    return 0;
}
