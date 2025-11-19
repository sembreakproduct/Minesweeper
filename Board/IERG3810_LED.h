
void IERG3810_LED_Init(){
int	led1status =0,buzzerstatus=0;

	//GPIO_InitTypeDef GPIO_InitStructure;
	//GPIO_InitTypeDef GPIO_Key2;
	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC->APB2ENR|=1<<3; //led0(5) buzzer(8) B
	RCC->APB2ENR|=1<<6; //key2(2) 1(3) led1(5) E
	RCC->APB2ENR|=1<<2; //keyup(0) A
	//GPIO_InitStructure.GPIO_Pin=GPIO_Pin_5;
	//GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
	//GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	//GPIO_Init(GPIOB, & GPIO_InitStructure);
	//GPIO_SetBits(GPIOB, GPIO_Pin_5);
	GPIOB->CRL &= 0xFF0FFFFF;
	GPIOB->CRL |= 0x00300000;
	GPIOB->CRH &= 0xFFFFFFF0;
	GPIOB->CRH |= 0x00000003;
	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	//GPIO_Key2.GPIO_Pin=GPIO_Pin_2;
	//GPIO_Key2.GPIO_Mode=GPIO_Mode_IPU;
	GPIOE->CRL &= 0xFF0F00FF;
	GPIOE->CRL |= 0x00308800;
	GPIOE->ODR |= 0x0000000C;
	//GPIO_Init(GPIOE, & GPIO_Key2);
	
	GPIOA->CRL &= 0xFFFFFFF0;
	GPIOA->CRL |= 0x00000008;

}