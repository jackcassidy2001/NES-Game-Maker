/*****************************************************************************
 * Joseph Zambreno
 * Phillip Jones
 *
 * Department of Electrical and Computer Engineering
 * Iowa State University
 *****************************************************************************/

/*****************************************************************************
 * nes_bootloader.c - main nes_bootloader application code. The bootloader
 * reads a .nes file from the SD card, and uses this information to
 * load and emulate the NES rom.
 *
 *
 * NOTES:
 * 10/25/13 by JAZ::Design created.
 *****************************************************************************/

#include "nes_bootloader.h"
#include "NESCore/NESCore.h"
#include "xgpiops.h"
#include <unistd.h>  // for usleep
#include "xgpio.h"


#define HORZ_SIZE_PIXELS 640
#define VERT_SIZE_LINES 480
#define BYTES_PER_PIXEL 2

XGpioPs GPIO;           /* The driver instance for GPIO device. */
XGpioPs_Config *CONFIGPTR;

XGpio CTLR;

// Main function. Performs Xilinx-specific initialization, and then goes into the main polling loop
int main() {
	int status;

	 xil_printf("WAV_BASEADDR = %X \r\n", WAV_BASEADDR);

    /* Initialize the GPIO driver. */
    CONFIGPTR = XGpioPs_LookupConfig(XPAR_XGPIOPS_0_DEVICE_ID);
    status = XGpioPs_CfgInitialize(&GPIO, CONFIGPTR, CONFIGPTR->BaseAddr);
    if (status != XST_SUCCESS)
    {
        return XST_FAILURE;
    }

    // Initialize the GPIO instance with a specific Device ID
    if (XGpio_Initialize(&CTLR, XPAR_AXI_GPIO_3_DEVICE_ID) != XST_SUCCESS) {
        xil_printf("GPIO initialization failed!\n");
        return XST_FAILURE;
    }

    // Configure the GPIO instance with the base address
    if (XGpio_CfgInitialize(&CTLR, XGpio_LookupConfig(XPAR_AXI_GPIO_3_DEVICE_ID),
                            CTLR.BaseAddress) != XST_SUCCESS) {
        xil_printf("GPIO configuration failed!\n");
        return XST_FAILURE;
    }

    // Set the data direction for Channel 1
    XGpio_SetDataDirection(&CTLR, 1, 0x00000001); // Set Pin 0 as input, Pin 2 as output

    // Clock High
    XGpio_DiscreteWrite(&CTLR, 1, 0x00000004); // Pin 2

    // GPIO_PS
    XGpioPs_SetDirectionPin(&GPIO, 50, 0);
    XGpioPs_SetOutputEnablePin(&GPIO, 50, 0);

    XGpioPs_SetDirectionPin(&GPIO, 51, 0);
    XGpioPs_SetOutputEnablePin(&GPIO, 51, 0);



	// Initialize all memory space
	xil_init();

	// Initialize the NESCore
	NESCore_Init();


	// Enable the cache
    Xil_DCacheEnable();


    // Main polling loop. For now, you can hard-code the .nes ROM you would like to load.
    // Later, improve the code to have user-specified entry and exit options
	while (1) {
		nes_load();
	}

}



// Runs the main NES emulation
void nes_load() {

	int32_t result = 0, i;
	uint8_t nes_fname[17];

	nes_strncpy(nes_fname, "bombman2.nes", 13);

	usleep(100000);

	if (bootstate.debug_level >= 1)
		xil_printf("nes_load(): loading %s\r\n", nes_fname);


	// Disable the cache so it will play nice with xilsd (needed here)
	Xil_DCacheDisable();
	result = NESCore_LoadROM(nes_fname);
	if (result != 0) {
		xil_printf("nes_load(): invalid ROM load. Returning\r\n");
		xil_printf("Error Number : %d\n\r", result);
	}
	// Enable the cache for performance reasons
    Xil_DCacheEnable();



	result = NESCore_Reset();
	if (result != 0) {
		xil_printf("nes_load(): invalid reset. Returning\r\n");
	}

	if (bootstate.debug_level >= 1)
		xil_printf("nes_load(): beginning emulation of %s\r\n", nes_fname);


	bootstate.nes_playing = 1;
	usleep(100000);
	ptv = 0;

	// Runs the emulator 20 cycles at a time. Currently there is no exit condition.
	do {

		for (i = 0; i < RESET_TIME; i++) {
			NESCore_Cycle();
		}

	} while (1);


	bootstate.nes_playing = 0;

	return;

}





// Initializes bootloader state, the Xilinx peripherals, and the front buffer
void xil_init() {

	XStatus Status = XST_SUCCESS;
	uint32_t i;
	uint16_t *ptr;

	// Setup the bootloader state variables.
	bootstate.nes_playing = 0;
	bootstate.activeBuffer = (uint32_t *)FBUFFER_BASEADDR;


	bootstate.debug_level = 1;

	// For now, we disable the DCache as it causes problems with xilsd and vdma
	Xil_DCacheDisable();


	// Initialize the VTC module
	if (bootstate.debug_level >= 1)
		print("xil_init(): Initializing v_tc module\r\n");

	VtcCfgPtr = XVtc_LookupConfig(XPAR_V_TC_0_DEVICE_ID);
	XVtc_CfgInitialize(&Vtc, VtcCfgPtr, VtcCfgPtr->BaseAddress);
	XVtc_EnableGenerator(&Vtc);


	// Initialize the front buffer
	if (bootstate.debug_level >= 1)
		print("xil_init(): Initializing front buffer\r\n");

	// Initialize the framebuffer. We can overwrite the edges with 0s.
	ptr = (uint16_t *)FBUFFER_BASEADDR;
	for (i = 0; i < WIDTH*HEIGHT; i++) {
		ptr[i] = INIT_COLOR;
		if (i % WIDTH == 0)
			ptr[i] = 0;
	}

	// Initialize the back buffer
	if (bootstate.debug_level >= 1)
		print("xil_init(): Initializing back buffer\r\n");

	ptr = (uint16_t *)BBUFFER_BASEADDR;
	for (i = 0; i < WIDTH*HEIGHT; i++) {
		ptr[i] = INIT_COLOR;
		if (i % WIDTH == 0)
			ptr[i] = 0;
	}

	// Initialize the VDMA module
	if (bootstate.debug_level >= 1)
		print("xil_init(): Initializing vdma module\r\n");


    // Set up VDMA config registers
	//#define CHANGE_ME 0

    // Simple function abstraction by Vendor for writing VDMA registers
    XAxiVdma_WriteReg(XPAR_AXI_VDMA_0_BASEADDR, XAXIVDMA_CR_OFFSET,  (u32)0x00000003);  // Read Channel: VDMA MM2S Circular Mode and Start bits set, VDMA MM2S Control
    XAxiVdma_WriteReg(XPAR_AXI_VDMA_0_BASEADDR, XAXIVDMA_HI_FRMBUF_OFFSET, (u32)0x00000000);  // Read Channel: VDMA MM2S Reg_Index
    XAxiVdma_WriteReg(XPAR_AXI_VDMA_0_BASEADDR, XAXIVDMA_MM2S_ADDR_OFFSET + XAXIVDMA_START_ADDR_OFFSET, (u32)FBUFFER_BASEADDR);  // Read Channel: VDMA MM2S Frame buffer Start Addr 1
    XAxiVdma_WriteReg(XPAR_AXI_VDMA_0_BASEADDR, XAXIVDMA_MM2S_ADDR_OFFSET + XAXIVDMA_STRD_FRMDLY_OFFSET, (u32)(HORZ_SIZE_PIXELS * BYTES_PER_PIXEL));  // Read Channel: VDMA MM2S FRM_Delay, and Stride
    XAxiVdma_WriteReg(XPAR_AXI_VDMA_0_BASEADDR, XAXIVDMA_MM2S_ADDR_OFFSET + XAXIVDMA_HSIZE_OFFSET,  (u32)(HORZ_SIZE_PIXELS * BYTES_PER_PIXEL));  // Read Channel: VDMA MM2S HSIZE
    XAxiVdma_WriteReg(XPAR_AXI_VDMA_0_BASEADDR, XAXIVDMA_MM2S_ADDR_OFFSET + XAXIVDMA_VSIZE_OFFSET, (u32)VERT_SIZE_LINES);  // Read Channel: VDMA MM2S VSIZE  (Note: Also Starts VDMA transaction)


  	return;
}


