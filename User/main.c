#include <stdio.h>
#include "stm32f10x.h"
#include "FONT.H"
#include "global.h"
#define GRID_WIDTH 10
#define GRID_HEIGHT 10
#define NUM_MINES 98
#define CELL_SIZE 22
#define PADDING 2
#define CELL_HIDDEN 0
#define CELL_REVEALED 1
#define CELL_FLAGGED 2
#define MINE -1
#define EMPTY 0
int grid[GRID_WIDTH][GRID_HEIGHT];
int cellState[GRID_WIDTH][GRID_HEIGHT];
int cursorX = 0;
int cursorY = 0;
u8 task1HeartBeat=0;
u8 task2HeartBeat=0;
int elapsedTime = 0;  // This will store the total elapsed time in seconds
int remainingFlags = NUM_MINES;  // This will store the number of flags the player can still place
int remainingCELL = GRID_WIDTH*GRID_HEIGHT;
u32 digitalcolor[] = {0x07FF,0xFFE0,0xF800,0xF81F,0xFFE0,0x07E0,0x0000,0xFFFF};
void delay(u32 count){
	u32 i;
	for (i=0; i<count; i++);
}

typedef struct{
	u16 LCD_REG;
	u16 LCD_RAM;
} LCD_TypeDef;
#define LCD_BASE 	((u32)(0x6c000000 | 0x000007FE))
#define LCD				((LCD_TypeDef *) LCD_BASE)
void IERG3810_TFTLCD_WrReg(u16 regval)
{
    LCD->LCD_REG=regval;
}
void IERG3810_TFTLCD_WrData(u16 data)
{
    LCD->LCD_RAM=data;
}
void LCD_Set9341_Parameter(void)
{
    IERG3810_TFTLCD_WrReg(0x01);     // Software reset
    IERG3810_TFTLCD_WrReg(0x11);     // Exit_sleep_mode
    IERG3810_TFTLCD_WrReg(0x3A);     // Set_pixel_format
    IERG3810_TFTLCD_WrData(0x55);    // 65536 colors
    IERG3810_TFTLCD_WrReg(0x29);     // Display ON
    IERG3810_TFTLCD_WrReg(0x36);     // Memory Access Control
    IERG3810_TFTLCD_WrData(0xc8);    // control Display direction
}

void LCD_Set7789_Parameter(void)
{
    IERG3810_TFTLCD_WrReg(0x01);     // Software reset
				delay(1000000);       // wait after Software reset
    IERG3810_TFTLCD_WrReg(0x11);     // Sleep Out
			delay(1000000);       // wait after SLEEP OUT
    IERG3810_TFTLCD_WrReg(0x3A);     // Memory Data Access Control
			IERG3810_TFTLCD_WrData(0x05);    
    IERG3810_TFTLCD_WrReg(0x29);     // display on
    IERG3810_TFTLCD_WrReg(0x36);     // Memory Data Access Control
			IERG3810_TFTLCD_WrData(0x80);	// control Display direction
}
void IERG3810_LED_Init(){
	
	RCC -> APB2ENR &= 0xFFFFFFF7;
	RCC -> APB2ENR |= 0x00000008;	
	GPIOB -> CRL &= 0xFF0FFFFF;
	GPIOB -> CRL |= 0x00300000;
	GPIOB -> BSRR = 1 << 5;
	RCC -> APB2ENR &= 0xFFFFFFBF;
	RCC -> APB2ENR |= 0x00000040;
	GPIOE -> CRL &= 0xFF0FFFFF;
	GPIOE -> CRL |= 0x00300000;
	GPIOE -> BSRR = 1 << 5;
}
void IERG3810_clock_tree_init(void){
	
	u8 PLL = 7;
	unsigned char temp = 0;
	RCC->CFGR &= 0xF8FF0000;
	RCC->CR &= 0xFEF6FFFF;
	RCC->CR |= 0x00010000;
	while (!(RCC->CR >> 17));
	//RCC -> CFGR = 0x00000400;
	RCC->CFGR = 0x00002400; //***CHNAGED***
	RCC->CFGR |= PLL << 18;
	RCC->CFGR |= 1 << 16;
	FLASH->ACR |= 0x32;
	RCC->CR |= 0x01000000;
	while (!(RCC->CR >> 25));
	RCC->CFGR |= 0x00000002;
	while (temp != 0x02) {
		temp = RCC->CFGR >> 2;
		temp &= 0x03;
	}
}
/*void USART_print(u8 USARTport, char *st){
	u8 i=0;
	while (st[i] != 0x00)
	{
		if(USARTport==1) USART1->DR=st[i];
		if(USARTport==2) USART1->DR=st[i];
		while(!(USART1->SR >> 7)); 
		if(i==255) break;
		i++;
	}
}*/
void IERG3810_NVIC_SetPriorityGroup (u8 prigroup){

	/*Set PRIGROUP AIRCR[10:8]*/
	u32 temp, temp1;
	temp1 = prigroup & 0x00000007; //only consider 3 bits
																 //so the 0<= prigroup <=7
	temp1 <<= 8; // shift to bit [10:8]
	temp = SCB->AIRCR; //put the data into the AIRCR register
										 //DDI0337E Page.8-22
	temp &= 0x0000F8FF;//?
										 //DDI0337E Page.8-22 
	temp |= 0x05FA0000;//?
										 //DDI0337E Page.8-22
	temp |= temp1;
	SCB->AIRCR=temp;	
}

 /*void IERG3810_keyUP_ExtiInit(void){
	//KEY2 at PA0, EXTI-0, IRQ#6
	RCC->APB2ENR |= 1 << 2;
	GPIOA -> CRL &= 0xFFFFFFF0; 
	GPIOA -> CRL |= 0x00000008;
	GPIOA -> ODR &= 0xFFFFFFF0;
	
	RCC->APB2ENR |= 0x01; 
	AFIO -> EXTICR[0] &= 0xFFFFFFF0;
	AFIO -> EXTICR[0] |= 0x00000000;
	// (RM0008, AFIO_EXTICR1, Page-211&212)
	EXTI->IMR         |= 1; //edge trigger
	EXTI->RTSR        |= 1; //falling edge
	
	NVIC->IP[6] = 0x95;				//set priority of this interrupt  
	NVIC->ISER[0] &= ~(1<<6); //set NVIC 'SET ENABLE REGISTER'
	NVIC->ISER[0] |=  (1<<6); //IRQ6
}*/

void IERG3810_PS2key_ExtiInit(void){
	/*PS2KEY CLK at PC11, EXTI-11, IRQ40,DATA at PC10*/
	RCC->APB2ENR |= 1<<4;
	GPIOC -> CRH &= 0xFFFF0FFF; 
	GPIOC -> CRH |= 0x00008000;
	GPIOC -> ODR |= 1 << 11;
	GPIOC -> CRH &= 0xFFFFF0FF; 
	GPIOC -> CRH |= 0x00000800;
	RCC->APB2ENR |= 0x01;
	AFIO -> EXTICR[2] &= 0xFFFF0FFF;
	AFIO -> EXTICR[2] |= 0x00002000;
	// (RM0008, AFIO_EXTICR3, Page-211&212)
	EXTI->IMR     |= 1<<11; //edge trigger
	EXTI->FTSR |= 1<<11;
	NVIC->IP[40] = 0x65;
	NVIC->ISER[1] &= ~(1<<8); //set NVIC 'SET ENABLE REGISTER'
	NVIC->ISER[1] |=  (1<<8); //IRQ40
}


/* void IERG3810_key2_ExtiInit(void){

	//KEY2 at PE2, EXTI-2, IRQ#8
	RCC->APB2ENR |= 1 << 6;  
	GPIOE -> CRL &= 0xFFFFF0FF; 
	GPIOE -> CRL |= 0x00000800;
	GPIOE -> ODR |= 1 << 2;
	
	RCC->APB2ENR |= 0x01; 
	AFIO -> EXTICR[0] &= 0xFFFFF0FF;
	AFIO -> EXTICR[0] |= 0x00000400;
	// (RM0008, AFIO_EXTICR1, Page-211&212)
	EXTI->IMR         |= 1<<2; //edge trigger
	EXTI->FTSR        |= 1<<2; //falling edge
	
	NVIC->IP[8] = 0x65;				//set priority of this interrupt  
	NVIC->ISER[0] &= ~(1<<8); //set NVIC 'SET ENABLE REGISTER'
	NVIC->ISER[0] |=  (1<<8); //IRQ8
}	*/
void IERG3810_TFTLCD_DrawDot(u16 x, u16 y, u16 color)
{
    IERG3810_TFTLCD_WrReg(0x2A); // set x position
			IERG3810_TFTLCD_WrData(x >> 8);
			IERG3810_TFTLCD_WrData(x & 0xFF);
			IERG3810_TFTLCD_WrData(0x01);
			IERG3810_TFTLCD_WrData(0x3F);
    IERG3810_TFTLCD_WrReg(0x2B); // set y position
			IERG3810_TFTLCD_WrData(y >> 8);
			IERG3810_TFTLCD_WrData(y & 0xFF);
			IERG3810_TFTLCD_WrData(0x01);
			IERG3810_TFTLCD_WrData(0xDF);
    IERG3810_TFTLCD_WrReg(0x2C); // set point with color
    IERG3810_TFTLCD_WrData(color);
}
void IERG3810_TFTLCD_Init(void) //set FSMC
{
    RCC->AHBENR |= 1 << 8;      // FSMC
    RCC->APB2ENR |= 1 << 3;      // PORTB
    //RCC->APB2ENR |= 1 << 4;      // PORTC
    RCC->APB2ENR |= 1 << 5;      // PORTD
    RCC->APB2ENR |= 1 << 6;      // PORTE
    RCC->APB2ENR |= 1 << 8;      // PORTG
    GPIOB->CRL&= 0XFFFFFFF0;    // PB0
	  GPIOB->CRL |= 0X00000003;
    // PORTD
    GPIOD->CRH &= 0X00FFF000;
    GPIOD->CRH |= 0XBB000BBB;
    GPIOD->CRL &= 0XFF00FF00;
    GPIOD->CRL |= 0X00BB00BB;
    // PORTE
    GPIOE->CRH &= 0X00000000;
    GPIOE->CRH |= 0XBBBBBBBB;
    GPIOE->CRL &= 0X0FFFFFFF;
    GPIOE->CRL |= 0XB0000000;
    // PORTG12
    GPIOG->CRH &= 0XFFF0FFFF;
    GPIOG->CRH |= 0X000B0000;
    GPIOG->CRL &= 0XFFFFFFF0;    // PG0->RS
    GPIOG->CRL |= 0X0000000B;

    // LCD uses FSMC Bank 4 memory bank.
    // Use Mode A
    FSMC_Bank1->BTCR[6] = 0x00000000;  // FSMC_BCR4 (reset)
    FSMC_Bank1->BTCR[7] = 0x00000000;  // FSMC_BTR4 (reset)
    FSMC_Bank1E->BWTR[6] = 0x00000000;  // FSMC_BWTR4 (reset)
    FSMC_Bank1->BTCR[6] |= 1 << 12;     // FSMC_BCR4 -> WREN
    FSMC_Bank1->BTCR[6] |= 1 << 14;     // FSMC_BCR4 -> EXTMOD
    FSMC_Bank1->BTCR[6] |= 1 << 4;      // FSMC_BCR4 -> MWID
    FSMC_Bank1->BTCR[7] |= 0 << 28;     // FSMC_BTR4 -> ACCMOD
    FSMC_Bank1->BTCR[7] |= 1<<0;        // FSMC_BTR4 -> ADDSET
    FSMC_Bank1E->BWTR[6] |= 0XF << 8;   // FSMC_BWTR4 -> DATASET
    FSMC_Bank1E->BWTR[6] |= 0 << 28;  // FSMC_BWTR4 -> ACCMOD
		FSMC_Bank1E->BWTR[6] |= 0 << 0;    // FSMC_BWTR4 -> ADDSET
    FSMC_Bank1E->BWTR[6] |= 3 << 8;     // FSMC_BWTR4 -> DATASET
    FSMC_Bank1->BTCR[6] |= 1 << 0;     // FSMC_BCR4 -> FACCEN
    LCD_Set9341_Parameter(); // special setting for LCD module
		GPIOB->BSRR |= 0x1;
		//LCD_LIGHT_ON;
}
void IERG3810_TFTLCD_FillRectangle(u16 color, u16 start_x, u16 length_x, u16 start_y, u16 length_y)
{
    u32 index = 0;
    IERG3810_TFTLCD_WrReg(0x2A);
    IERG3810_TFTLCD_WrData(start_x >> 8);
    IERG3810_TFTLCD_WrData(start_x & 0xFF);
    IERG3810_TFTLCD_WrData((length_x + start_x - 1) >> 8);
    IERG3810_TFTLCD_WrData((length_x + start_x - 1) & 0xFF);
    IERG3810_TFTLCD_WrReg(0x2B);
    IERG3810_TFTLCD_WrData(start_y >> 8);
    IERG3810_TFTLCD_WrData(start_y & 0xFF);
    IERG3810_TFTLCD_WrData((length_y + start_y - 1) >> 8);
    IERG3810_TFTLCD_WrData((length_y + start_y - 1) & 0xFF);
    IERG3810_TFTLCD_WrReg(0x2C); // LCD_WriteRAM_Prepare()
    for (index = 0; index < length_x * length_y; index++) {
        IERG3810_TFTLCD_WrData(color);
    }}
		void IERG3810_TFTLCD_ShowChar(u16 x, u16 y, u8 ascii, u16 color, u16 bgcolor)
{
    u8 i, j;
    u8 index;
    u8 height = 16, length = 8;
    if (ascii < 32 || ascii > 127) return;
    ascii -= 32;
    IERG3810_TFTLCD_WrReg(0x2A);
			IERG3810_TFTLCD_WrData(x >> 8);
			IERG3810_TFTLCD_WrData(x & 0xFF);
			IERG3810_TFTLCD_WrData((length + x - 1) >> 8);
    IERG3810_TFTLCD_WrData((length + x - 1) & 0xFF);
    IERG3810_TFTLCD_WrReg(0x2B);
    IERG3810_TFTLCD_WrData(y >> 8);
    IERG3810_TFTLCD_WrData(y & 0xFF);
    IERG3810_TFTLCD_WrData((height + y - 1) >> 8);
    IERG3810_TFTLCD_WrData((height + y - 1) & 0xFF);
    IERG3810_TFTLCD_WrReg(0x2C); // LCD_WriteRAM_Prepare();
    for (j = 0; j < height / 8; j++) {
        for (i = 0; i < height / 2; i++) {
            for (index = 0; index < length; index++) {
                if ((asc2_1608[ascii][index * 2 +1- j] >> i) & 0x01){
                    IERG3810_TFTLCD_WrData(color);
									}
                 else
                    IERG3810_TFTLCD_WrData(bgcolor);
            }
        }
    }
}

//physical pin EXTI2
//EXTI0~4, all have a corresponding IRQ software
//The below function is viewed as an IRQ for EXTI2  
void EXTI2_IRQHandler(void){
	
	u8 i;
	for (i=0; i<10; i++){
		//DS0_ON;
		GPIOB -> BRR = 1 << 5;
		delay(1000000);
		//DS0_OFF;
		GPIOB -> BSRR = 1 << 5;
		delay(1000000);
	}
	
	EXTI->PR = 1<<2;
}
void EXTI0_IRQHandler(void){
	
	u8 i;
	for (i=0; i<10; i++){
		GPIOE -> BRR = 1 << 5;
		delay(1000000);
		GPIOE -> BSRR = 1 << 5;
		delay(1000000);
	}
	
	EXTI->PR = 1;
}
u32 sheep = 0;
u32 timeout=10000;
u32	ps2key= 0;
u32 ps2count = 0;
u8 ps2dataReady = 0;
/*void EXTI15_10_IRQHandler(void){
	u16 i = ((GPIOC->IDR) & (1<<10)) >> 10;
	if (ps2count > 0 && ps2count < 9)
	ps2key |= i << (ps2count-1);
	ps2count++;
	if(ps2count >10){
		ps2dataReady =1;
		ps2count = 0;}
	delay(30);
	EXTI->PR=1<<11;
}*/
volatile u8 ps2InterruptFlag = 0;  // Flag to indicate PS2 interrupt occurred
volatile u16 ps2BitValue = 0;      // Store the current bit value read from the PS2 pin

void EXTI15_10_IRQHandler(void) {
    ps2BitValue = ((GPIOC->IDR) & (1 << 10)) >> 10;  // Read the bit from PS2 pin (PC10)
    ps2InterruptFlag = 1;  // Set the flag to indicate that the main loop should process this bit
		// Transfer the workload from the previous interrupt handler
    if (ps2count > 0 && ps2count < 9) {
      ps2key |= ps2BitValue << (ps2count - 1);  // Shift the bit into the correct position
     }
    ps2count++;
		delay(30);
    EXTI->PR = 1 << 11;  // Clear the interrupt flag (EXTI line 11)
		
}
void IERG3810_TIM3_Init(u16 arr, u16 psc)
{
    // TIM3, IRQ#29
    RCC->APB1ENR |= 1 << 1;    // RM0008 page-115, refer to lab-1
    TIM3->ARR = arr;           // RM0008 page-419
    TIM3->PSC = psc;           // RM0008 page-418
    TIM3->DIER |= 1 << 0;      // RM0008 page-409
    TIM3->CR1 = 0x01;          // RM0008 page-404
    NVIC->IP[29] = 0x45;       // refer to lab-4, DDI0337E
    NVIC->ISER[0] |= (1 << 29); // refer to lab-4, DDI0337E
}
/*void IERG3810_TIM4_Init(u16 arr, u16 psc){
	//TIM4, IRQ#30
		RCC->APB1ENR |= 1 << 2;    // RM0008 page-115, refer to lab-1
    TIM4->ARR = arr;           // RM0008 page-419
    TIM4->PSC = psc;      
		TIM4->DIER |= 1 << 0;      // RM0008 page-409
    TIM4->CR1 = 0x01; 
		NVIC->IP[30] = 0x45;       // refer to lab-4, DDI0337E
		NVIC->ISER[0] |= (1 << 30); // refer to lab-4, DDI0337E
}*/
void drawTopBar(void) {
	char buffer[10],buffer2[4],buffer3[]={"2:down 4:left 6:right 8:down+:flag -:unflag 5:reveal"};
		int minutes,seconds,i=0;
    // Clear the top part of the screen (e.g., 30 pixels high)
    //IERG3810_TFTLCD_FillRectangle(0x0000, 0, 240, 241, 320);  // Black background for the top bar
		
    // Display remaining flags
	sprintf(buffer, "Flags: %02d", remainingFlags);
		while(buffer[i]!='\0'){
    IERG3810_TFTLCD_ShowChar(10+8*i, 246, buffer[i], 0x07FF, 0x0000);  // White text for remaining flags
    i++;}
    for(i=0;i<28;i++)
			IERG3810_TFTLCD_ShowChar(8+8*i, 300, buffer3[i], 0xFFFF, 0x0000);
		for(i=28;i<52;i++)
			IERG3810_TFTLCD_ShowChar(24+8*(i-28), 270, buffer3[i], 0xFFFF, 0x0000);
    // Display elapsed time (count-up timer)
    //minutes = elapsedTime / 60;
    seconds = elapsedTime % 60;
    sprintf(buffer2, "%04d",elapsedTime);
		for(i=0;i<4;i++)
    IERG3810_TFTLCD_ShowChar(200+8*i, 246, buffer2[i], 0xF800, 0x0000);  // Display minutes and seconds
}
int lose=0;
int start=0;
int win=0;
void TIM3_IRQHandler(void) {
    if (TIM3->SR & 1 << 0) {  // Check if update interrupt flag is set
        if(start)
				elapsedTime++;  // Increment the elapsed time by 1 second
        if(!lose && start && !win)
					drawTopBar();   // Update the top bar to show the new time
        TIM3->SR &= ~(1 << 0);  // Clear the update interrupt flag
    }
}
void TIM4_IRQHandler(void){
    if (TIM4->SR & 1 << 0)     // check UIF, RM0008 page-410
    {
        GPIOE->ODR ^= 1 << 5;  // toggle DS1 with read-modify-write
    }
    TIM4->SR &= ~(1 << 0);     // clear UIF, RM0008 page-410
}
void IERG3810_SYSTICK_Init10ms(void)
{
    // SYSTICK
    SysTick->CTRL = 0;       // clear
    SysTick->LOAD = 90000-1;     // What should be filled? Refer to DDI-0337E
    SysTick->VAL=0;                         // Refer to 0337E page 8-10.
    // CLKSOURCE=0: STCLK (FCLK/8)
    // CLKSOURCE=1: FCLK/8
    // set CLKSOURCE=0 is synchronized and better than CLKSOURCE=1
    // Refer to Clock tree on RM0008 page-93.
    SysTick->CTRL |= 0x3;    // What should be filled?
                             // set internal clock, use interrupt, start count
}
void IERG3810_TIM3_PwmInit(u16 arr, u16 psc){
    RCC->APB2ENR |= 1 << 3;        // lab-1
    GPIOB->CRL &= 0xFF0FFFFF;      // lab-1
    GPIOB->CRL |= 0x00B00000;      // lab-1
    RCC->APB2ENR |= 1 << 0;        // lab-4
    AFIO->MAPR &= 0xFFFFF3FF;      // RM0008 page-184
    AFIO->MAPR |= 1 << 11;         // 
    RCC->APB1ENR |= 1 << 1;        // RM0008 page-115
    TIM3->ARR = arr;               // RM0008 page-419
    TIM3->PSC = psc;               // RM0008 page-418
    TIM3->CCMR1 |= 7 << 12;        // RM0008 page-419
    TIM3->CCMR1 |= 1 << 11;         // 
    TIM3->CCER |= 1 << 4;          // RM0008 page-417
    TIM3->CR1 = 0x0080;            // RM0008 page-404
    TIM3->CR1 |= 0x01;             // 
}
void drawGrid(void) {
    int x, y;
    int gridStartY,startX, startY = 0;  // Start drawing the grid from the top (y = 0)

    // Loop through each cell and draw it on the screen
    for (x = 0; x < GRID_WIDTH; x++) {
			startX = x * (CELL_SIZE + PADDING);
        for (y = 0; y < GRID_HEIGHT; y++) {
            u16 color = 0xFFFF;  // Default color for hidden cells (white)
            // Calculate the actual position of the cell
            gridStartY = startY + y * (CELL_SIZE + PADDING);  // Adjust the y-coordinate for each cell
						
            // Draw the padding (background) first
            //IERG3810_TFTLCD_FillRectangle(0xF800, startX - PADDING, PADDING, gridStartY - PADDING, CELL_SIZE + 2 * PADDING);
            //IERG3810_TFTLCD_FillRectangle(0xF800, startX - PADDING, CELL_SIZE + 2 * PADDING, gridStartY - PADDING, PADDING);

            // Now draw the cell
            if (cellState[x][y] == CELL_REVEALED) {
                if (grid[x][y] == MINE) {
                    color = 0xF800;  // Red for mines
                    IERG3810_TFTLCD_FillRectangle(color, startX, CELL_SIZE, gridStartY, CELL_SIZE);
                    IERG3810_TFTLCD_ShowChar(startX + 8, gridStartY + 4, 'M', 0xFFFF, color);  // Show 'M' for mines
                } else {
                    color = 0x0320;  // Green for numbers
                    IERG3810_TFTLCD_FillRectangle(color, startX, CELL_SIZE, gridStartY, CELL_SIZE);
                    if (grid[x][y] > 0) {
                        IERG3810_TFTLCD_ShowChar(startX + 8, gridStartY + 4, '0' + grid[x][y], digitalcolor[grid[x][y]-1], color);  // Show number of adjacent mines
                    }
                }
            } else if (cellState[x][y] == CELL_FLAGGED) {
                color = 0x001F;  // Blue for flagged cells
                IERG3810_TFTLCD_FillRectangle(color, startX, CELL_SIZE, gridStartY, CELL_SIZE);
                IERG3810_TFTLCD_ShowChar(startX + 8, gridStartY + 4, 'F', 0xFFFF, color);  // Show 'F' for flagged cells
            } else {
                IERG3810_TFTLCD_FillRectangle(0xFFFF, startX, CELL_SIZE, gridStartY, CELL_SIZE);  // Hidden cell (white)
            }
        }
    }
}
	int prevCursorX = 0;
	int prevCursorY = 0;

void drawCursor(void) {
    int prevStartX, prevStartY, startX, startY;
    u16 cursorColor = 0xFFE0;  // Default yellow for cursor

    // Ensure the cursor is within grid bounds
    if (cursorX < 0 || cursorX >= GRID_WIDTH || cursorY < 0 || cursorY >= GRID_HEIGHT) {
        return;  // Don't draw the cursor if it's out of bounds
    }

    // Redraw the previous cell under the cursor only if necessary
    if (prevCursorX >= 0 && prevCursorX < GRID_WIDTH && prevCursorY >= 0 && prevCursorY < GRID_HEIGHT) {
        prevStartX = prevCursorX * (CELL_SIZE + PADDING);
        prevStartY = prevCursorY * (CELL_SIZE + PADDING);

        if (cellState[prevCursorX][prevCursorY] == CELL_REVEALED) {
            // Redraw revealed cell
            if (grid[prevCursorX][prevCursorY] == MINE) {
                IERG3810_TFTLCD_FillRectangle(0xF800, prevStartX, CELL_SIZE, prevStartY, CELL_SIZE);  // Red mine
                IERG3810_TFTLCD_ShowChar(prevStartX + 8, prevStartY + 4, 'M', 0xFFFF, 0xF800);
            } else {
                IERG3810_TFTLCD_FillRectangle(0x0320, prevStartX, CELL_SIZE, prevStartY, CELL_SIZE);  // Green for numbers
                if (grid[prevCursorX][prevCursorY] > 0) {
                    IERG3810_TFTLCD_ShowChar(prevStartX + 8, prevStartY + 4, '0' + grid[prevCursorX][prevCursorY], digitalcolor[grid[prevCursorX][prevCursorY]-1], 0x0320);
                }
            }
        } else if (cellState[prevCursorX][prevCursorY] == CELL_FLAGGED) {
            IERG3810_TFTLCD_FillRectangle(0x001F, prevStartX, CELL_SIZE, prevStartY, CELL_SIZE);  // Blue for flagged cells
            IERG3810_TFTLCD_ShowChar(prevStartX + 8, prevStartY + 4, 'F', 0xFFFF, 0x001F);
        } else {
            IERG3810_TFTLCD_FillRectangle(0xFFFF, prevStartX, CELL_SIZE, prevStartY, CELL_SIZE);  // Hidden cell (white)
        }
    }

    // Now draw the cursor at the current position
    startX = cursorX * (CELL_SIZE + PADDING);
    startY = cursorY * (CELL_SIZE + PADDING);

    // Customize cursor color based on the cell state
    if (cellState[cursorX][cursorY] == CELL_REVEALED) {
        cursorColor = 0xFF80;  // Light green for cursor on revealed cells
    } else if (cellState[cursorX][cursorY] == CELL_FLAGGED) {
        cursorColor = 0xFF00;  // Light blue for cursor on flagged cells
    }

    // Draw the cursor
    IERG3810_TFTLCD_FillRectangle(cursorColor, startX, CELL_SIZE, startY, CELL_SIZE);  // Cursor color based on cell state

    // Update the previous cursor position
    prevCursorX = cursorX;
    prevCursorY = cursorY;
}

void moveCursor(int dx, int dy) {
    // Calculate the new position
    cursorX += dx;
    cursorY += dy;

    // Ensure the cursor stays within the grid boundaries
    if (cursorX < 0) cursorX = 0;
    if (cursorX >= GRID_WIDTH) cursorX = GRID_WIDTH - 1;
    if (cursorY < 0) cursorY = 0;
    if (cursorY >= GRID_HEIGHT) cursorY = GRID_HEIGHT - 1;
		
}
void placeMines(void) {
    int x,y,minesPlaced = 0;

    // Continue placing mines until we reach the desired number of mines
    while (minesPlaced < NUM_MINES) {
        x = rand() % GRID_WIDTH;   // Random X position
        y = rand() % GRID_HEIGHT;  // Random Y position

        // Only place a mine if this cell doesn't already have one
        if (grid[x][y] != MINE) {
            grid[x][y] = MINE;
            minesPlaced++;
        }
    }
}
void calculateNumbers(void) {
    int dx, dy,x,y,nx,ny,mineCount;
    // Loop through every cell in the grid
    for (x = 0; x < GRID_WIDTH; x++) {
        for (y = 0; y < GRID_HEIGHT; y++) {
            // Skip cells that are already mines
            if (grid[x][y] == MINE) {
                continue;
            }
            // Count how many neighboring cells contain mines
            mineCount = 0;

            for (dx = -1; dx <= 1; dx++) {
                for (dy = -1; dy <= 1; dy++) {
                    nx = x + dx;  // Neighboring X coordinate
                     ny = y + dy;  // Neighboring Y coordinate

                    // Check if the neighbor is within bounds and if it contains a mine
                    if (nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT && grid[nx][ny] == MINE) {
                        mineCount++;
                    }
                }
            }
            // Store the number of adjacent mines in the grid
            grid[x][y] = mineCount;
        }
    }
}
int revealAllMines(void) {
	int x,y;
    for (x = 0; x < GRID_WIDTH; x++) {
        for (y = 0; y < GRID_HEIGHT; y++) {
            if (grid[x][y] == MINE && cellState[x][y] == CELL_HIDDEN) {
                cellState[x][y] = CELL_REVEALED;  // Reveal the mine
                drawGrid();  // Redraw the grid to show the revealed mine
                delay(500000);  // Add a delay between revealing each mine
            }
        }
    }
		return 1;
}
void revealCell(int x, int y) {
    int stack[GRID_WIDTH * GRID_HEIGHT][2];  // A stack to hold the cells to be revealed
    int top = 0;  // Stack pointer
    int nx, ny, dx, dy;
		int i,j;
    // Push the initial cell onto the stack
    stack[top][0] = x;
    stack[top][1] = y;
    top++;

    // Process cells on the stack
    while (top > 0) {
        // Pop the top cell
        top--;
        x = stack[top][0];
        y = stack[top][1];
				
        // Only reveal hidden cells
        if (cellState[x][y] != CELL_HIDDEN) continue;
				while (grid[x][y] == MINE && remainingCELL == GRID_WIDTH*GRID_HEIGHT){
					for(i =0; i < GRID_WIDTH;i++){
						for(j =0; j < GRID_HEIGHT ;j++){
							grid[i][j] = EMPTY;
							cellState[i][j] = CELL_HIDDEN;}}
					placeMines();
					calculateNumbers();
				}
        // Mark the cell as revealed
        cellState[x][y] = CELL_REVEALED;
				remainingCELL--;
        if (grid[x][y] == MINE) {
            revealAllMines();
            lose = 1;
            return;  // Stop further processing if a mine is revealed
        } 
				if(remainingCELL == NUM_MINES){
            win = 1;
						delay(3000000);
            return;  // Stop further processing if a mine is revealed
        } 
				if (grid[x][y] == EMPTY) {
            // Push neighboring cells onto the stack
            for (dx = -1; dx <= 1; dx++) {
                for (dy = -1; dy <= 1; dy++) {
                    nx = x + dx;
                    ny = y + dy;

                    // Ensure the neighbor is within bounds
                    if (nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT) {
                        // Only add hidden cells to the stack
                        if (cellState[nx][ny] == CELL_HIDDEN) {
                            stack[top][0] = nx;
                            stack[top][1] = ny;
                            top++;
                        }
                    }
                }
            }
        }
				
        // Redraw the grid after revealing each cell
        drawGrid();
        //drawCursor();
    }
}
int restart = 0;
void processInput(u32 ps2key) {
	static u8 keyReleased = 0;
    
    // Check if this is a break code (key release) event
    if (ps2key == 0xF0) {
        keyReleased = 1;  // Indicate that the next key is a release event
        return;           // Wait for the next key to arrive
    }

    // If the previous key was a release event, ignore this key
    if (keyReleased) {
        keyReleased = 0;  // Reset release flag
        return;           // Ignore the key release event
    }
		//IERG3810_TFTLCD_ShowChar(120, 300, ps2key, 0xF800, 0x0000);
    switch (ps2key) {
        case 0x75:  // '8' on number pad (Up)
            moveCursor(0, 1);  // Move cursor up
            break;
        case 0x72:  // '2' on number pad (Down)
            moveCursor(0, -1);   // Move cursor down
            break;
        case 0x6B:  // '4' on number pad (Left)
            moveCursor(-1, 0);  // Move cursor left
            break;
        case 0x74:  // '6' on number pad (Right)
            moveCursor(1, 0);   // Move cursor right
            break;
        case 0x73:  // '5' on number pad (Reveal cell)
            revealCell(cursorX, cursorY);  // Reveal the current cell
            break;
        case 0x79:  // '+' on number pad (Flag cell)
            
						if (cellState[cursorX][cursorY] != CELL_HIDDEN || remainingFlags == 0)
							break;
						cellState[cursorX][cursorY] = CELL_FLAGGED;  // Mark the cell as flagged
						remainingFlags--;  // Decrease the remaining flags count
						drawTopBar();      // Update the top bar to show the new flag count
						drawGrid();
            break;
				case 0x7B:  // '-' on number pad (Unflag cell)
            //unflagCell(cursorX, cursorY);    // Flag the current cell
				if (cellState[cursorX][cursorY] != CELL_FLAGGED)
					break;
        cellState[cursorX][cursorY] = CELL_HIDDEN;  // Unflag the cell
        remainingFlags++;  // Increase the remaining flags count
        drawTopBar();      // Update the top bar to show the new flag count
				drawGrid();
            break;
				//case 0x7D:
					//win = 1;
				  //break;
        default:
					ps2key = 0;
            break;
    }
		delay(10);
}
void initGame(void){
	int x=0,y=0;
	srand(SysTick->VAL*1000);
	remainingFlags = NUM_MINES;
	remainingCELL = GRID_WIDTH*GRID_HEIGHT;
	for(x =0; x < GRID_WIDTH;x++){
		for(y =0; y < GRID_HEIGHT ;y++){
			grid[x][y] = EMPTY;
			cellState[x][y] = CELL_HIDDEN;}}
	placeMines();
	calculateNumbers();
}
typedef enum {
    PS2_INTERRUPT,  // PS2 interrupt flag is set
    PS2_PROCESS,    // PS2 key data is ready to be processed
    GAME_OVER,      // Game over condition
    IDLE,            // Idle state (waiting for input or other tasks)
		RESTART,
		WIN
} GameState;

int main(void) {
		u8 tick;
		int i,done = 0;
		char buffer[20],startpage[18],minesweeper[11];
    GameState state = IDLE;  // Start in the idle state
    SysTick_Config(SystemCoreClock / 1000);  // Initialize SysTick timer (1 ms ticks)
    IERG3810_clock_tree_init();
    IERG3810_LED_Init();
    IERG3810_TFTLCD_Init();
    delay(500000);  // Initial delay
    IERG3810_PS2key_ExtiInit();
    IERG3810_TIM3_Init(10000, 7200);  // Initialize TIM3 for 1 Hz count-up timer
		sprintf(buffer, "PRESS ANY TO RESTART");
		sprintf(startpage, "PRESS ANY TO START");
		sprintf(minesweeper, "Minesweeper");
    initGame();  // Initialize the game
    IERG3810_TFTLCD_FillRectangle(0x0000, 0, 240, 0, 320);  // Clear the screen
		
			for(i=0;i<18;i++){
				if(i<11)
					IERG3810_TFTLCD_ShowChar(78+8*i, 200, minesweeper[i], 0xF800, 0x0000);
				IERG3810_TFTLCD_ShowChar(48+8*i, 100, startpage[i], 0xF800, 0x0000);}
		while(!ps2InterruptFlag);
		start =1;
		IERG3810_TFTLCD_FillRectangle(0x0000, 0, 240, 0, 320);
    drawGrid();  // Draw the initial game grid
    drawCursor();  // Draw the initial cursor

    while (1) {
			tick = task1HeartBeat;
			
        switch (state) {
						case RESTART:
							elapsedTime = 0;
							initGame();  // Initialize the game
							IERG3810_TFTLCD_FillRectangle(0x0000, 0, 240, 0, 320);  // Clear the screen
							drawGrid();  // Draw the initial game grid
							drawCursor();  // Draw the initial cursor
							restart = 0;
							state = IDLE;
							break;
            case PS2_INTERRUPT:
                // Handle PS2 interrupt processing
                if (ps2InterruptFlag) {
                    
                    // Check if we've received the full PS2 packet (11 bits: start, 8 data, parity, stop)
                    if (ps2count > 10) {
                        ps2dataReady = 1;  // Indicate PS2 data is ready to be processed
                        ps2count = 0;      // Reset the count for the next key
                    }

                    ps2InterruptFlag = 0;  // Clear the flag after processing
                    state = IDLE;  // Move back to the IDLE state after handling interrupt
                }
                break;

            case PS2_PROCESS:
                // Handle PS2 data processing
								if(tick % 100 ==0 && done == 0){
                if (ps2dataReady) {
                    processInput(ps2key);  // Process the PS2 input
                    ps2dataReady = 0;      // Clear the PS2 data ready flag
                    ps2key = 0;            // Reset the PS2 key
                    drawCursor();           // Redraw the cursor after processing input
                    delay(1000);            // Small delay after processing input
                    state = IDLE;           // Move back to IDLE after processing
                }
								done =1;}
								else if (tick %100 != 0 && done ==1)
								done = 0;
                break;
						case WIN:
							IERG3810_TFTLCD_FillRectangle(0x0000, 0, 240, 0, 320);  // Clear the screen
							IERG3810_TFTLCD_ShowChar(75, 150, 'C', 0x07FF, 0x0000);  // Display "CONGRATS"
							IERG3810_TFTLCD_ShowChar(85, 150, 'O', 0x07FF, 0x0000);
							IERG3810_TFTLCD_ShowChar(95, 150, 'N', 0x07FF, 0x0000);
							IERG3810_TFTLCD_ShowChar(105, 150, 'G', 0x07FF, 0x0000);
							IERG3810_TFTLCD_ShowChar(115, 150, 'R', 0x07FF, 0x0000);
							IERG3810_TFTLCD_ShowChar(125, 150, 'A', 0x07FF, 0x0000);
							IERG3810_TFTLCD_ShowChar(135, 150, 'T', 0x07FF, 0x0000);
							IERG3810_TFTLCD_ShowChar(145, 150, 'S', 0x07FF, 0x0000);
							IERG3810_TFTLCD_ShowChar(155, 150, '!', 0x07FF, 0x0000);

							for (i = 0; i < 20; i++) {
									IERG3810_TFTLCD_ShowChar(40 + 8 * i, 100, buffer[i], 0x07FF, 0x0000);
							}
							while(1){
							if (ps2InterruptFlag) {
									state = RESTART;
									win = 0;
									break;
							}}
							break;
            case GAME_OVER:
                // Handle Game Over state
                
                    IERG3810_TFTLCD_FillRectangle(0x0000, 0, 240, 0, 320);  // Clear the screen
                    IERG3810_TFTLCD_ShowChar(53, 150, 'G', 0xF800, 0x0000);  // Display "GAME OVER"
										delay(1000000);
                    IERG3810_TFTLCD_ShowChar(69, 150, 'A', 0xF800, 0x0000);
										delay(1000000);
                    IERG3810_TFTLCD_ShowChar(85, 150, 'M', 0xF800, 0x0000);
										delay(1000000);
                    IERG3810_TFTLCD_ShowChar(101, 150, 'E', 0xF800, 0x0000);
										delay(1000000);
                    IERG3810_TFTLCD_ShowChar(137, 150, 'O', 0xF800, 0x0000);
										delay(1000000);
                    IERG3810_TFTLCD_ShowChar(153, 150, 'V', 0xF800, 0x0000);
										delay(1000000);
                    IERG3810_TFTLCD_ShowChar(169, 150, 'E', 0xF800, 0x0000);
										delay(1000000);
                    IERG3810_TFTLCD_ShowChar(185, 150, 'R', 0xF800, 0x0000);
										delay(1000000);
										for(i=0;i<20;i++)
											IERG3810_TFTLCD_ShowChar(40+8*i, 100, buffer[i], 0xF800, 0x0000);
										//delay(15000000);
									GPIOB->ODR ^= (1 << 5);  
								// Halt the game
								while(1){
                if (ps2InterruptFlag) {
									state = RESTART;
									GPIOB->BSRR = (1<<5);
									lose = 0;
									break;
							}}
                break;

            case IDLE:
                // In the idle state, check for any flags or conditions to handle
                if (ps2InterruptFlag) {
                    state = PS2_INTERRUPT;  // Switch to PS2 interrupt handling state
                } else if (ps2dataReady) {
                    state = PS2_PROCESS;    // Switch to PS2 data processing state
                } else if (lose) {
                    state = GAME_OVER;      // Switch to Game Over state
                } else if (restart) {
                    state = RESTART;      // Switch to Game Over state
                } else if (win) {
                    state = WIN;      // Switch to Game Over state
                }else {
                    // Remain in idle if there's nothing to do
                    // Delay can be added here to reduce CPU usage if necessary
                    delay(10);  // Small delay to prevent CPU overuse
                }
                break;

            default:
                state = IDLE;  // Default state is idle
                break;
        }
    
		
		}
}