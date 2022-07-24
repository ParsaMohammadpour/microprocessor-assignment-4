#include<stm32f4xx.h>

//Defining a variable to store the port number 7 use them easier
// LCD pins number in port A
#define d0 0 //PA0
#define d1 1 //PA1
#define d2 2 //PA2
#define d3 3 //PA3
#define d4 4 //PA4
#define d5 5 //PA5
#define d6 6 //PA6
#define d7 7 //PA07

#define E 8 //PA8
#define RW 9 //PA9
#define RS 10 //PA10

// Keypad pins number in port B
#define A 4//PB0
#define B 5//PB1
#define C 6//PB2
#define D 7//PB3

#define one 0 //PB4
#define two 1 //PB5
#define three 2 //PB6
#define four 3 //PB7

//Button 1 pins in port B
#define sw1 8 //PB8
#define sw2 9 //PB9

//OR gates for intrrupts in port C
#define or1 0 // PC0
#define or2 1 // PC1

//MASKS:
#define SET1(x) (1ul << (x))
#define SET0(x) (~(1ul << (x)))

//variables for calculating the sum
static char array[16];
static int size = 0; // return the first empty index
static int status = 0; // 0 = usual      1 = finding result       2 = clear

//Defining Functions
void initialize(void);
void findKeypadButton(void);
void LCD_put_char(char data);
void delay(volatile int n);
void LCD_init(void);
void LCD_command(unsigned char command);
void LCD_resetCommand(void);
void LCD_setCommand(void);
int getNumber(char c);
void inc_dec_number(int value);
char getChar(int digit);
void calculate(void);
void EXTI0_IRQHandler(void);
void EXTI1_IRQHandler(void);
void resetArray(void);

/*
 ******************************************** FUNCTIONS ************************************************
 */
void initialize(void){
	RCC -> AHB1ENR |= RCC_AHB1ENR_GPIOAEN; // turning on the clocks for GPIOA
	RCC -> AHB1ENR |= RCC_AHB1ENR_GPIOBEN; // turning on the clocks for GPIOB
	RCC -> AHB1ENR |= RCC_AHB1ENR_GPIOCEN; // turning on the clocks for GPIOC
	
	//GPIOA is output. So we set the MODE for them (pin 0 to 10) writing mode which is "01". There are 11 pins, so 
	//the number will be 01 0101 0101 0101 0101 0101
	GPIOA -> MODER = 0x155555;
	
	//in GPIOB the first 4 pins are keypads output(micro's input), next 4 pins are keypads input(micro's output) and 2 last pins
	//are buttens output(micro's input). reading MODE : "00"       writing MODE : "01"    => 0101 0101 0101 0000 0000
	GPIOB -> MODER = 0x55500;
	
	//in GPIOC 2 first pins are input. MODE code for input is "00". So the number will be: 0000
	GPIOC -> MODER = 0x0;
	
	//in keypad, first 4 pins in GPIOB which are connected to keypads output(micro's input) should be pull-down. and 2 last pins in GPIOB 
	//which are connected to buttons, should be pull-down. pull-down : "10"         => 1010 0000 0000 1010 1010
	GPIOB -> PUPDR = 0xA00AA;
	
	//in GPIOC both input pins are pull-down and pull-down code is "10". So the number will be : 1010
	GPIOC -> MODER = 0xA;
	
	//setting keypad input to 1
	GPIOB -> ODR |= SET1(A) | SET1(B) | SET1(C) | SET1(D);
	
	//writing names to LCD
	LCD_init();
	delay(100);
	LCD_setCommand();
	
	//turning on the system configuration clock
	RCC -> APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	
	//onnecting intrrupt line 1 and 2 to port B
	SYSCFG -> EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PC; // keypad
	SYSCFG -> EXTICR[0] |= SYSCFG_EXTICR1_EXTI1_PC; // buttons
	
	//removing intrrupt mask register for line 1 and 2. And we also set them in rising edge
	EXTI -> IMR |= SET1(0) | SET1(1);
	EXTI -> RTSR |= SET1(0) | SET1(1);
	
	//turning on the intrrupts line
	__enable_irq();
	
	//setting priority
	NVIC_SetPriority(EXTI0_IRQn, 0);
	NVIC_SetPriority(EXTI1_IRQn, 0);
	
	//clearing pending registers
	NVIC_ClearPendingIRQ(EXTI0_IRQn);
	NVIC_ClearPendingIRQ(EXTI1_IRQn);
	
	//enabling
	NVIC_EnableIRQ(EXTI0_IRQn);
	NVIC_EnableIRQ(EXTI1_IRQn);
	
	// setting all array indexes to "&". (we consider this as a null index in array)
	resetArray();
}


void findKeypadButton(void){
	if(GPIOB -> IDR & SET1(one)){
		GPIOB -> ODR = 0xE0;// 0b11100000
		if((GPIOB -> IDR & SET1(one)) == 0){
			array[size++] = '7';
			return;
		}
		GPIOB -> ODR = 0xD0;// 0b11010000
		if((GPIOB -> IDR & SET1(one)) == 0){
			array[size++] = '4';
			return;
		}
		GPIOB -> ODR = 0xB0;// 0b10110000
		if((GPIOB -> IDR & SET1(one)) == 0){
			array[size++] = '1';
			return;
		}
		GPIOB -> ODR = 0x70; //0b01110000
		if((GPIOB -> IDR & SET1(one)) == 0){
			status = 2; // clear status
			return;
		}
	}else if(GPIOB -> IDR & SET1(two)){
		GPIOB -> ODR = 0xE0;
		if((GPIOB -> IDR & SET1(two)) == 0){
			array[size++] = '8';
			return;
		}
		GPIOB -> ODR = 0xD0;
		if((GPIOB -> IDR & SET1(two)) == 0){
			array[size++] = '5';
			return;
		}
		GPIOB -> ODR = 0xB0;
		if((GPIOB -> IDR & SET1(two)) == 0){
			array[size++] = '2';
			return;
		}
		GPIOB -> ODR = 0x70;
		if((GPIOB -> IDR & SET1(two)) == 0){
			array[size++] = '0';
			return;
		}
	}else if(GPIOB -> IDR & SET1(three)){
		GPIOB -> ODR = 0xE0;
		if((GPIOB -> IDR & SET1(three)) == 0){
			array[size++] = '9';
			return;
		}
		GPIOB -> ODR = 0xD0;
		if((GPIOB -> IDR & SET1(three)) == 0){
			array[size++] = '6';
			return;
		}
		GPIOB -> ODR = 0xB0;
		if((GPIOB -> IDR & SET1(three)) == 0){
			array[size++] = '3';
			return;
		}
		GPIOB -> ODR = 0x70;
		if((GPIOB -> IDR & SET1(three)) == 0){
			status = 1; // finding the result
			return;
		}
	}else if(GPIOB -> IDR & SET1(four)){
		GPIOB -> ODR = 0xE0;
		if((GPIOB -> IDR & SET1(four)) == 0){
			array[size++] = '/';
			return;
		}
		GPIOB -> ODR = 0xD0;
		if((GPIOB -> IDR & SET1(four)) == 0){
			array[size++] = '*';
			return;
		}
		GPIOB -> ODR = 0xB0;
		if((GPIOB -> IDR & SET1(four)) == 0){
			array[size++] = '-';
			return;
		}
		GPIOB -> ODR = 0x70;
		if((GPIOB -> IDR & SET1(four)) == 0){
			array[size++] = '+';
			return;
		}
	}
}




void LCD_put_char(char data) {
	GPIOA -> ODR = data;
  GPIOA -> ODR |= SET1(RS);
	GPIOA -> ODR &= SET0(RW);
  GPIOA -> ODR |= SET1(E);
	GPIOA -> ODR &= SET0(E);
}

void LCD_init(void){
	LCD_command(0x38);
	delay(100);
	LCD_command(0x06);
	delay(100);
	LCD_command(0x08);
	delay(100);
	LCD_command(0x0F);
	delay(100);
}

void LCD_command(unsigned char command){
	GPIOA -> ODR = command;
	GPIOA -> ODR &= SET0(RS);
	GPIOA -> ODR &= SET0(RW);
  GPIOA -> ODR |= SET1(E);
  delay(0);
  GPIOA -> ODR &= SET0(E);

  if (command < 4)
      delay(2);           /* command 1 and 2 needs up to 1.64ms */
  else
      delay(1);           /* all others 40 us */
}


// delay n milliseconds
void delay(volatile int n) {
    int i;
    for (; n > 0; n--)
        for (i = 0; i < 3195; i++) ;
}

int getNumber(char c){
	switch(c){
		case '0' : return 0;
		case '1' : return 1;
		case '2' : return 2;
		case '3' : return 3;
		case '4' : return 4;
		case '5' : return 5;
		case '6' : return 6;
		case '7' : return 7;
		case '8' : return 8;
		case '9' : return 9;
	}
	return -1;
}
void LCD_setCommand(void){
	LCD_command(0x01);
	delay(10000);
	LCD_command(0x38);
	delay(100);
	LCD_put_char('S');
	delay(100);
	LCD_put_char('e');
	delay(100);
	LCD_put_char('i');
	delay(100);
	LCD_put_char('f');
	delay(100);
	LCD_put_char('i');
	delay(100);
	LCD_put_char('-');
	delay(100);
	LCD_put_char('m');
	delay(100);
	LCD_put_char('h');
	delay(100);
	LCD_put_char('m');
	delay(100);
	LCD_put_char('d');
	delay(100);
	LCD_put_char('p');
	delay(100);
	LCD_put_char('r');
	delay(100);
	LCD_command(0xC0);
	delay(100);
}

void LCD_resetCommand(void){
	LCD_command(0xC0);
	delay(100);
	for(int i = 0; i < 16; i++){
		LCD_put_char(' ');
		delay(100);
	}
	LCD_command(0xC0);
	delay(100);
	int i = 0;
	while(i < size){
		LCD_put_char(array[i++]);
		delay(100);
	}
}
void inc_dec_number(int value){
	int number = 0;
	int counter = 0;
	if(getNumber(array[counter]) == -1){
		counter = 1;
		delay(100); // this is necessary, otherwise it won't work properly.(it seems, it doesn't have enough time to be executed)
	}
	while((counter < size) && (getNumber(array[counter]) >= 0)){
		number *= 10;
		number += getNumber(array[counter++]);
	}
	if(getNumber(array[0]) == -1){
		number = -number;
	}
	number += value;
	resetArray();
	if(number == 0){
		array[size++]='0';
		return;
	}
	char digit[8];
	if(number < 0){
		array[size++] = '-';
		number = -number;
	}
	int index = 0;
	while(number > 0){
		digit[index++] = getChar(number % 10);
		number /= 10;
	}
	while(index > 0)
		array[size++] = digit[--index];
}

char getChar(int digit){
	switch(digit){
		case 0 : return '0';
		case 1 : return '1';
		case 2 : return '2';
		case 3 : return '3';
		case 4 : return '4';
		case 5 : return '5';
		case 6 : return '6';
		case 7 : return '7';
		case 8 : return '8';
		case 9 : return '9';
	}
	return '&';
}
void resetArray(void){
	size = 0;
	for(int i = 0; i< 16; i++)
		array[i] = '&';
}

void calculate(void){
	int op1 = 0, op2 = 0, total;
	int counter = 0;
	if(getNumber(array[counter]) == -1){
		counter = 1;
		delay(100); // this is necessary, otherwise it won't work properly.(it seems, it doesn't have enough time to be executed)
	}
	while((counter < size) && (getNumber(array[counter]) >= 0)){
		op1 *= 10;
		op1 += getNumber(array[counter++]);
	}
	int operatorIndex = counter;
	if(getNumber(array[0]) == -1){
		op1 = -op1;
	}
	total = op1;
	if(counter == size || array[counter] == '&'){
		return;
	}
	counter++;
	while(counter < size){
		op2 *= 10;
		op2 += getNumber(array[counter++]);
	}
	switch(array[operatorIndex]){
		case '+' : total = op1 + op2; break;
		case '-' : total = op1 - op2; break;
		case '*' : total = op1 * op2; break;
		case '/' : total = op1 / op2; break;
	}
	resetArray();
	if(total == 0){
		array[size++]='0';
		return;
	}
	char digit[8];
	if(total < 0){
		array[size++] = '-';
		total = -total;
	}
	int index = 0;
	while(total > 0){
		digit[index++] = getChar(total % 10);
		total /= 10;
	}
	while(index > 0)
		array[size++] = digit[--index];
}
/*
 ************************************* INTRRUPT HANDLER ********************************
 */
	// keypad first column
void EXTI0_IRQHandler(void){ // keypad
	//masking pending register
	EXTI -> PR |= SET1(0);
	
	//masking this intrrupt to avoid repeating this intrrupt being executed
	EXTI -> IMR &= SET0(0);
	
	//clearing pending IRQ
	NVIC_ClearPendingIRQ(EXTI0_IRQn);
	
	//finding kepad Button
	findKeypadButton();
	
	//for finding the number we changed them. now we reset them
	GPIOB -> ODR |= SET1(A) | SET1(B) | SET1(C) | SET1(D);

	if(status == 0){ // adding a character 
		if(getNumber(array[size]) < 0){
			char newChar = array[size - 1];
			int operatorNumber = 0;
			for(int i = 1; i < size ; i++){
				if(getNumber(array[i]) < 0){
					operatorNumber++;
				}
			}
			delay(100);
			if(operatorNumber == 2){
				size--;
				delay(100);
				array[size] = '&';
				calculate();
				array[size++] = newChar;
				LCD_resetCommand();
			}else{
				LCD_put_char(array[size - 1]);
			}
		}else{
			LCD_put_char(array[size - 1]);
		}
	}else if(status == 1){ // finding result
		calculate();
		LCD_resetCommand();
		status = 0;
	}else if(status == 2){ // resetting the LCD
		resetArray();
		LCD_resetCommand();
		status = 0;
	}
	
	//unmasking this intrrupt
	EXTI -> IMR |= SET1(0);
}


	// keypad second column
void EXTI1_IRQHandler(void){ // buttons
	//masking pending register
	EXTI -> PR |= SET1(1);
	
	//masking this intrrupt to avoid repeating this intrrupt being executed
	EXTI -> IMR &= SET0(1);
	
	//clearing pending IRQ
	NVIC_ClearPendingIRQ(EXTI0_IRQn);
	
	if(GPIOB -> IDR & SET1(sw1)){
		calculate();
		inc_dec_number(-1);
		LCD_resetCommand();
	}else if(GPIOB -> IDR & SET1(sw2)){
		calculate();
		inc_dec_number(1);
		LCD_resetCommand();
	}
	
	//unmasking this intrrupt
	EXTI -> IMR |= SET1(1);
}


int main(void){
	
	initialize();
	
	while(1){
		
	}
}
