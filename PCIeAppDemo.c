
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "PCIE.h"


#define DEMO_PCIE_USER_BAR			PCIE_BAR0
#define DEMO_PCIE_IO_LED_ADDR		0x00
#define DEMO_PCIE_IO_BUTTON_ADDR	0x20
#define DEMO_PCIE_FIFO_WRITE_ADDR	0x40
#define DEMO_PCIE_FIFO_STATUS_ADDR	0x60
#define DEMO_PCIE_FIFO_READ_ADDR	0x80
#define DEMO_PCIE_FIFO_GPIO_ADDR    0x100
#define DEMO_PCIE_MEM_ADDR			0x20000

#define MEM_SIZE			(128*1024) //128KB
#define FIFO_SIZE			(16*1024) // 2KBx8



typedef enum{
	REST = 128,   // 1000 0000 (Bytes)
	THUMB = 64,   // 0100 0000
	INDEX = 32,   // 0010 0000
	MIDDLE = 16,  // 0001 0000
	RING = 8,     // 0000 1000
	PINKY = 4,    // 0000 0100
	WRIST = 2,    // 0000 0000
	QUIT = 0      // 0000 0000
}MENU_ID;

void UI_ShowMenu(void){
	printf("==============================\r\n");
	printf("[%d]: REST\r\n", REST);
	printf("[%d]: THUMB\r\n", THUMB);
	printf("[%d]: INDEX\r\n", MIDDLE);
	printf("[%d]: RING\r\n", RING);
	printf("[%d]: PINKY\r\n", PINKY);
	printf("[%d]: WRIST\r\n", WRIST);
	printf("[%d]: Quit\r\n", QUIT);
}

int UI_UserSelect(void){
	int nSel;
	scanf("%d",&nSel);
	return nSel;
}


void GestureSelect(PCIE_HANDLE hPCIe, int nSel)
{
	BOOL bPass;

	
	bPass = PCIE_Write32(hPCIe, DEMO_PCIE_USER_BAR, DEMO_PCIE_FIFO_GPIO_ADDR, (BYTE)nSel);
	if (bPass)
		printf("GESTURE control success \r\n");
	else
		printf("GESTURE conrol failed\r\n");
}

BOOL TEST_LED(PCIE_HANDLE hPCIe){
	BOOL bPass;
	int	Mask;
	
	printf("Please input led conrol mask:");
	scanf("%d", &Mask);

	bPass = PCIE_Write32(hPCIe, DEMO_PCIE_USER_BAR, DEMO_PCIE_IO_LED_ADDR,(DWORD)Mask);
	if (bPass)
		printf("Led control success, mask=%xh\r\n", Mask);
	else
		printf("Led conrol failed\r\n");

	
	return bPass;
}

BOOL TEST_BUTTON(PCIE_HANDLE hPCIe){
	BOOL bPass = TRUE;
	DWORD Status;

	bPass = PCIE_Read32(hPCIe, DEMO_PCIE_USER_BAR, DEMO_PCIE_IO_BUTTON_ADDR,&Status);
	if (bPass)
		printf("Button status mask:=%xh\r\n", Status);
	else
		printf("Failed to read button status\r\n");

	
	return bPass;
}

char PAT_GEN(int nIndex){
	char Data;
	Data = nIndex & 0xFF;
	return Data;
}

BOOL TEST_DMA_MEMORY(PCIE_HANDLE hPCIe){
	BOOL bPass;
	int i;
	const int nTestSize = MEM_SIZE;
	const PCIE_LOCAL_ADDRESS LocalAddr = DEMO_PCIE_MEM_ADDR;
	char *pWrite;
	char *pRead;
	char szError[256];


	pWrite = (char *)malloc(nTestSize);
	pRead = (char *)malloc(nTestSize);
	if (!pWrite || !pRead){
		bPass = FALSE;
		sprintf(szError, "DMA Memory:malloc failed\r\n");
	}
	

	// init test pattern
	for(i=0;i<nTestSize && bPass;i++)
		*(pWrite+i) = PAT_GEN(i);

	// write test pattern
	if (bPass){
		bPass = PCIE_DmaWrite(hPCIe, LocalAddr, pWrite, nTestSize);
		if (!bPass)
			sprintf(szError, "DMA Memory:PCIE_DmaWrite failed\r\n");
	}		

	// read back test pattern and verify
	if (bPass){
		bPass = PCIE_DmaRead(hPCIe, LocalAddr, pRead, nTestSize);

		if (!bPass){
			sprintf(szError, "DMA Memory:PCIE_DmaRead failed\r\n");
		}else{
			for(i=0;i<nTestSize && bPass;i++){
				if (*(pRead+i) != PAT_GEN(i)){
					bPass = FALSE;
					sprintf(szError, "DMA Memory:Read-back verify unmatch, index = %d, read=%xh, expected=%xh\r\n", i, *(pRead+i), PAT_GEN(i));
				}
			}
		}
	}


	// free resource
	if (pWrite)
		free(pWrite);
	if (pRead)
		free(pRead);
	
	if (!bPass)
		printf("%s", szError);
	else
		printf("DMA-Memory (Size = %d byes) pass\r\n", nTestSize);


	return bPass;
}

BOOL TEST_DMA_FIFO(PCIE_HANDLE hPCIe){
	BOOL bPass;
	int i;
	const int nTestSize = FIFO_SIZE;
	const PCIE_LOCAL_ADDRESS FifoID_Write = DEMO_PCIE_FIFO_WRITE_ADDR;
	const PCIE_LOCAL_ADDRESS FifoID_Read = DEMO_PCIE_FIFO_READ_ADDR;
	char *pBuff;
	char szError[256];


	pBuff = (char *)malloc(nTestSize);
	if (!pBuff){
		bPass = FALSE;
		sprintf(szError, "DMA Fifo: malloc failed\r\n");
	}
	

	// init test pattern
	if (bPass){
		for(i=0;i<nTestSize;i++)
			*(pBuff+i) = PAT_GEN(i);
	}

	// write test pattern into fifo
	if (bPass){
		bPass = PCIE_DmaFifoWrite(hPCIe, FifoID_Write, pBuff, nTestSize);
		if (!bPass)
			sprintf(szError, "DMA Fifo: PCIE_DmaFifoWrite failed\r\n");
	}		

	// read back test pattern and verify
	if (bPass){
		memset(pBuff, 0, nTestSize); // reset buffer content
		bPass = PCIE_DmaFifoRead(hPCIe, FifoID_Read, pBuff, nTestSize);

		if (!bPass){
			sprintf(szError, "DMA Fifo: PCIE_DmaFifoRead failed\r\n");
		}else{
			for(i=0;i<nTestSize && bPass;i++){
				if (*(pBuff+i) != PAT_GEN(i)){
					bPass = FALSE;
					sprintf(szError, "DMA Fifo: Read-back verify unmatch, index = %d, read=%xh, expected=%xh\r\n", i, *(pBuff+i), PAT_GEN(i));
				}
			}
		}
	}


	// free resource
	if (pBuff)
		free(pBuff);
	
	if (!bPass)
		printf("%s", szError);
	else
		printf("DMA-Fifo (Size = %d byes) pass\r\n", nTestSize);


	return bPass;
}



int main(void)
{
	void *lib_handle;
	PCIE_HANDLE hPCIE;
	BOOL bQuit = FALSE;
	int nSel;

	printf("== Terminator Arm PCIe Demo Program ==\r\n");

	lib_handle = PCIE_Load();
	if (!lib_handle){
		printf("PCIE_Load failed!\r\n");
		return 0;
	}

	hPCIE = PCIE_Open(DEFAULT_PCIE_VID,DEFAULT_PCIE_DID,0);
	if (!hPCIE){
		printf("PCIE_Open failed\r\n");
	}else{
		while(!bQuit){
			UI_ShowMenu();
			int gpio;
			printf("Please input GESTURE control value:");
			scanf("%d", &nSel);

			switch (gpio){
			case REST:
				gpio = REST;
				break;
			case THUMB:
				gpio = THUMB;
				break;
			case INDEX:
				gpio = INDEX;
				break;
			case MIDDLE:
				gpio = MIDDLE;
				break;
			case RING:
				gpio = RING;
			case PINKY:
				gpio = PINKY;
				break;
			case WRIST:
				gpio = WRIST;
				break;
			case QUIT:
				bQuit = TRUE;
				printf("Bye!\r\n");
				break;
			default:
				printf("Invalid selection\r\n");
			} // switch

			GestureSelect(hPCIE, gpio);

		}// while

		PCIE_Close(hPCIE);

	}


	PCIE_Unload(lib_handle);
	return 0;
}

