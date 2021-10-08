#include <stdio.h>
#include <stdint.h>

#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_flash.h"
#include "stm32f4xx_usart.h"

#define DWT_CYCCNT *(volatile uint32_t *)0xE0001004
#define DWT_CONTROL *(volatile uint32_t *)0xE0001000
#define SCB_DEMCR *(volatile uint32_t *)0xE000EDFC
uint32_t count_tic = 0;

#define measurement 0

#define adress 0x080E0000             // адрес хранени ключа
volatile uint8_t read[32], send[34];  //массивы для приема и отправки информации
volatile GPIO_InitTypeDef gpio;
volatile USART_InitTypeDef usart;
volatile uint8_t readStatus = 0;      // статус чтения, чтобы понимать, какая информация сейчас принимается
volatile uint8_t data = 0;            // переменная, в которую записываются принятые байты
volatile uint8_t readCount = 0;       // счетсик считанных байт
volatile uint8_t sendByte = 0;        // индикатор отправления символа
volatile uint8_t sendCount = 0;       // счетчик отправленных байт
volatile uint8_t sendKey = 0;         // индикатор отправления ключа
volatile char message[11] = { 0x0A,0x0D,'N','e','w',' ','k','e','y',':',' ' };  //сообщение о новом ключе
volatile char newLine[2] = { 0x0A,0x0D };    //переход на новую строку
volatile char wrong[9] = {0x0A,0x0D,'E','r','r','o','r',0x0A,0x0D};   //сообщение об ошибке
volatile uint8_t sendMessage = 0;     // индикатор отправления сообщения
volatile uint8_t sendNewLine = 0;     // индикатор переноса на новую строку
volatile uint32_t k[4], b[4];         // массивы с данными ключа и блока
volatile uint8_t isBlock = 0;         // индикатор шифрования
volatile uint8_t later = 1;           // индикатор смены ключа
volatile uint8_t sendBlock = 0;       // индикатор отправления блока
volatile uint8_t error = 0;           // индикатор ошибки в данных
volatile uint8_t sendError = 0;       // индикатор отправления сообщения об ошибке
volatile uint8_t sendtic = 0;

const uint32_t
/* Константы для генерации ключей */
con[] = { 0xf56b7aeb, 0x994a8a42, 0x96a4bd75, 0xfa854521,
          0x735b768a, 0x1f7abac4, 0xd5bc3b45, 0xb99d5d62,
          0x52d73592, 0x3ef636e5, 0xc57a1ac9, 0xa95b9b72,
          0x5ab42554, 0x369555ed, 0x1553ba9a, 0x7972b2a2,
          0xe6b85d4d, 0x8a995951, 0x4b550696, 0x2774b4fc,
          0xc9bb034b, 0xa59a5a7e, 0x88cc81a5, 0xe4ed2d3f,
          0x7c6f68e2, 0x104e8ecb, 0xd2263471, 0xbe07c765,
          0x511a3208, 0x3d3bfbe6, 0x1084b134, 0x7ca565a7,
          0x304bf0aa, 0x5c6aaa87, 0xf4347855, 0x9815d543,
          0x4213141a, 0x2e32f2f5, 0xcd180a0d, 0xa139f97a,
          0x5e852d36, 0x32a464e9, 0xc353169b, 0xaf72b274,
          0x8db88b4d, 0xe199593a, 0x7ed56d96, 0x12f434c9,
          0xd37b36cb, 0xbf5a9a64, 0x85ac9b65, 0xe98d4d32,
          0x7adf6582, 0x16fe3ecd, 0xd17e32c1, 0xbd5f9f66,
          0x50b63150, 0x3c9757e7, 0x1052b098, 0x7c73b3a7 };

const uint8_t
/* Блок замены S1*/
S1[] = { 0x6c, 0xda, 0xc3, 0xe9, 0x4e, 0x9d, 0x0a, 0x3d, 0xb8, 0x36, 0xb4, 0x38, 0x13, 0x34, 0x0c, 0xd9,
         0xbf, 0x74, 0x94, 0x8f, 0xb7, 0x9c, 0xe5, 0xdc, 0x9e, 0x07, 0x49, 0x4f, 0x98, 0x2c, 0xb0, 0x93,
         0x12, 0xeb, 0xcd, 0xb3, 0x92, 0xe7, 0x41, 0x60, 0xe3, 0x21, 0x27, 0x3b, 0xe6, 0x19, 0xd2, 0x0e,
         0x91, 0x11, 0xc7, 0x3f, 0x2a, 0x8e, 0xa1, 0xbc, 0x2b, 0xc8, 0xc5, 0x0f, 0x5b, 0xf3, 0x87, 0x8b,
         0xfb, 0xf5, 0xde, 0x20, 0xc6, 0xa7, 0x84, 0xce, 0xd8, 0x65, 0x51, 0xc9, 0xa4, 0xef, 0x43, 0x53,
         0x25, 0x5d, 0x9b, 0x31, 0xe8, 0x3e, 0x0d, 0xd7, 0x80, 0xff, 0x69, 0x8a, 0xba, 0x0b, 0x73, 0x5c,
         0x6e, 0x54, 0x15, 0x62, 0xf6, 0x35, 0x30, 0x52, 0xa3, 0x16, 0xd3, 0x28, 0x32, 0xfa, 0xaa, 0x5e,
         0xcf, 0xea, 0xed, 0x78, 0x33, 0x58, 0x09, 0x7b, 0x63, 0xc0, 0xc1, 0x46, 0x1e, 0xdf, 0xa9, 0x99,
         0x55, 0x04, 0xc4, 0x86, 0x39, 0x77, 0x82, 0xec, 0x40, 0x18, 0x90, 0x97, 0x59, 0xdd, 0x83, 0x1f,
         0x9a, 0x37, 0x06, 0x24, 0x64, 0x7c, 0xa5, 0x56, 0x48, 0x08, 0x85, 0xd0, 0x61, 0x26, 0xca, 0x6f,
         0x7e, 0x6a, 0xb6, 0x71, 0xa0, 0x70, 0x05, 0xd1, 0x45, 0x8c, 0x23, 0x1c, 0xf0, 0xee, 0x89, 0xad,
         0x7a, 0x4b, 0xc2, 0x2f, 0xdb, 0x5a, 0x4d, 0x76, 0x67, 0x17, 0x2d, 0xf4, 0xcb, 0xb1, 0x4a, 0xa8,
         0xb5, 0x22, 0x47, 0x3a, 0xd5, 0x10, 0x4c, 0x72, 0xcc, 0x00, 0xf9, 0xe0, 0xfd, 0xe2, 0xfe, 0xae,
         0xf8, 0x5f, 0xab, 0xf1, 0x1b, 0x42, 0x81, 0xd6, 0xbe, 0x44, 0x29, 0xa6, 0x57, 0xb9, 0xaf, 0xf2,
         0xd4, 0x75, 0x66, 0xbb, 0x68, 0x9f, 0x50, 0x02, 0x01, 0x3c, 0x7f, 0x8d, 0x1a, 0x88, 0xbd, 0xac,
         0xf7, 0xe4, 0x79, 0x96, 0xa2, 0xfc, 0x6d, 0xb2, 0x6b, 0x03, 0xe1, 0x2e, 0x7d, 0x14, 0x95, 0x1d };

const uint8_t
/* Таблица замен для преобразований блока S0 */
SS[4][16] = { {0xe, 0x6, 0xc, 0xa, 0x8, 0x7, 0x2, 0xf, 0xb, 0x1, 0x4, 0x0, 0x5, 0x9, 0xd, 0x3},
              {0x6, 0x4, 0x0, 0xd, 0x2, 0xb, 0xa, 0x3, 0x9, 0xc, 0xe, 0xf, 0x8, 0x7, 0x5, 0x1},
              {0xb, 0x8, 0x5, 0xe, 0xa, 0x6, 0x4, 0xc, 0xf, 0x7, 0x2, 0x3, 0x1, 0x0, 0xd, 0x9},
              {0xa, 0x2, 0x6, 0xd, 0x3, 0x4, 0x5, 0xe, 0x0, 0x7, 0x8, 0x9, 0xb, 0xf, 0xc, 0x1} };

volatile uint32_t RoundKeys[36] = {0};   // массив раундовых ключей шифрования

/*Инициализация портов для передачи данных с компьютером */
void InitAll() {
    __enable_irq();
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    /* Настройка портов приемы и вывода*/

    GPIO_StructInit(&gpio);
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_Pin = GPIO_Pin_2;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_Pin = GPIO_Pin_3;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &gpio);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

    USART_StructInit(&usart);
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    usart.USART_BaudRate = 9600;
    USART_Init(USART2, &usart);

    NVIC_EnableIRQ(USART2_IRQn);
    USART_Cmd(USART2, ENABLE);
}

/* Запись ключа во Flash память */
void WriteKey() {
    FLASH_Unlock();
    FLASH_EraseSector(FLASH_Sector_11, VoltageRange_3);
    for (uint8_t i = 0; i < 4; i++)
        FLASH_ProgramWord(adress + i*4, k[i]);
    FLASH_Lock();
}

/* Получение ячейки из Flash памяти */
uint32_t flash_read(uint32_t ad) {
    return (*(__IO uint32_t*) ad);
}

/* Чтение ключа из Flash памяти */
void ReadKey() {
    for (uint8_t i = 0; i < 4; i++) {
        k[i] = flash_read(adress + i*4);
    }
}

/* Преобразование массива из 32-х байт в массив из четырех элементов по 4 байта */
void GetBlock(uint8_t from[], uint32_t to[]) {
    for (uint8_t i=0;i<4;i++){
    	to[i] = 0;
    	for (uint8_t  j=0;j<8;j++){
    		if(from[i*8+j] == '0') to[i] = (to[i] << 4)|0x0;
    		else if (from[i*8+j] == '1') to[i] = (to[i] << 4)|0x1;
    		else if (from[i*8+j] == '2') to[i] = (to[i] << 4)|0x2;
    		else if (from[i*8+j] == '3') to[i] = (to[i] << 4)|0x3;
    		else if (from[i*8+j] == '4') to[i] = (to[i] << 4)|0x4;
    		else if (from[i*8+j] == '5') to[i] = (to[i] << 4)|0x5;
    		else if (from[i*8+j] == '6') to[i] = (to[i] << 4)|0x6;
    		else if (from[i*8+j] == '7') to[i] = (to[i] << 4)|0x7;
    		else if (from[i*8+j] == '8') to[i] = (to[i] << 4)|0x8;
    		else if (from[i*8+j] == '9') to[i] = (to[i] << 4)|0x9;
    		else if ((from[i*8+j] == 'A')||(from[i*8+j] == 'a')) to[i] = (to[i] << 4)|0xA;
    		else if ((from[i*8+j] == 'B')||(from[i*8+j] == 'b')) to[i] = (to[i] << 4)|0xB;
    		else if ((from[i*8+j] == 'C')||(from[i*8+j] == 'c')) to[i] = (to[i] << 4)|0xC;
    		else if ((from[i*8+j] == 'D')||(from[i*8+j] == 'd')) to[i] = (to[i] << 4)|0xD;
    		else if ((from[i*8+j] == 'E')||(from[i*8+j] == 'e')) to[i] = (to[i] << 4)|0xE;
    		else if ((from[i*8+j] == 'F')||(from[i*8+j] == 'f')) to[i] = (to[i] << 4)|0xF;
    	}
    }
}

/* Преобразования массива в байты для отправления */
void LittleBlock(uint32_t x[]) {
    send[0] = 0x0A;
    send[1] = 0x0D;
    for (uint8_t i=0;i<4;i++){
    	for (uint8_t j=0;j<8;j++){
    		uint8_t sym = (x[i]>>(28-4*j)) & 0xF;
    		if (sym == 0x0) send[2+i*8+j] = '0';
    		else if (sym == 0x1) send[2+i*8+j] = '1';
    		else if (sym == 0x2) send[2+i*8+j] = '2';
    		else if (sym == 0x3) send[2+i*8+j] = '3';
    		else if (sym == 0x4) send[2+i*8+j] = '4';
    		else if (sym == 0x5) send[2+i*8+j] = '5';
    		else if (sym == 0x6) send[2+i*8+j] = '6';
    		else if (sym == 0x7) send[2+i*8+j] = '7';
    		else if (sym == 0x8) send[2+i*8+j] = '8';
    		else if (sym == 0x9) send[2+i*8+j] = '9';
    		else if (sym == 0xA) send[2+i*8+j] = 'A';
    		else if (sym == 0xB) send[2+i*8+j] = 'B';
    		else if (sym == 0xC) send[2+i*8+j] = 'C';
    		else if (sym == 0xD) send[2+i*8+j] = 'D';
    		else if (sym == 0xE) send[2+i*8+j] = 'E';
    		else if (sym == 0xF) send[2+i*8+j] = 'F';
    	}
    }
}

/* Работа с получением данных и их передачей */
void USART2_IRQHandler() {

    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
        data = USART_ReceiveData(USART2);        // считывание символа
        sendByte = 1;
        if (readStatus == 0) {        // изменение статуса чтения, если он был нулевым
            if ((data == 0x31) || (data == 0x32) || (data == 0x33)||(data == 0x34)) {
                readStatus = data;
                sendNewLine = 1;
            }
        }
        else if ((readStatus == 0x31) || (readStatus == 0x32)) {   // readStatus = 0x31 - шифрование; readStatus = 0x32 - расшифрование
            read[readCount] = data;                                // образование блока из 32 шестнадцатеричных символов
            readCount++;
            if (readCount == 32) {
                readCount = 0;
                isRight();
                if (error) sendError = 1;                         // проверка введенных символов
                else isBlock = 1;
             }
        }
        else if (readStatus == 0x33) {   // readStatus = 0x33 - запись нового ключа
            read[readCount] = data;// формирование блока из 32 шестнадцатеричных символов
            readCount++;
            if (readCount == 32) {
            	readStatus = 0;
                readCount = 0;
                isRight();
                if (error) sendError = 1;   // проверка введенных символов
                else {
                	GetBlock(read, k);
                	WriteKey();             // запись ключа в память
                	ReadKey();
                	LittleBlock(k);
                	sendMessage = 1;
                	later = 1;
                }
            }
        }
    }
    if (USART_GetITStatus(USART2, USART_IT_TC) != RESET) {
    	if (sendByte) {
            USART_ClearITPendingBit(USART2, USART_IT_TC);    //вывод только что введенного символа
            USART_SendData(USART2, data);
            sendByte = 0;

        }
        else if (sendBlock) {
        	USART_ClearITPendingBit(USART2, USART_IT_TC);    // вывод зашифрованного/расшифрованного блока данных
            USART_SendData(USART2, send[sendCount]);
            sendCount++;
            if (sendCount == 34) {
            	sendCount = 0;
                sendBlock = 0;
                sendNewLine = 1;
             }
       }
        else if (sendMessage) {
            USART_ClearITPendingBit(USART2, USART_IT_TC);    //вывод сообщения о новом ключе
            USART_SendData(USART2, message[sendCount]);
            sendCount++;
            if (sendCount == 11) {
                sendMessage = 0;
                sendCount = 0;
                sendKey = 1;
            }
        }
        else if (sendKey) {
            USART_ClearITPendingBit(USART2, USART_IT_TC);   // вывод нового ключа
            USART_SendData(USART2, send[sendCount+2]);
            sendCount++;
            if (sendCount == 32) {
                sendCount = 0;
                sendKey = 0;
                sendNewLine = 1;
                //readStatus = 0;
            }
        }
        else if (sendNewLine) {
            USART_ClearITPendingBit(USART2, USART_IT_TC);   // перенос строки
            USART_SendData(USART2, newLine[sendCount]);
            sendCount++;
            if (sendCount == 2) {
                sendCount = 0;
                sendNewLine = 0;
            }
        }

        else if (sendError){
        	USART_ClearITPendingBit(USART2, USART_IT_TC);   // вывод сообщения об ошибке
        	USART_SendData(USART2, wrong[sendCount]);
        	sendCount++;
        	if (sendCount == 9) {
        		sendCount = 0;
        	    sendError = 0;
        	    readStatus = 0;
        	}
        }
    }
}

/* Умножение на 2 в поле Галуа для блока замены S0 */
uint8_t S0x2(uint8_t x) {
    unsigned int gf = 0x13;
    x = x << 1;
    if (x & (1 << 4)) x ^= gf;
    return x;
}

/* Реализация блока замены S0 */
volatile uint8_t S0(uint8_t x) {
    uint8_t x0 = x >> 4, x1 = x & 0x0f, u0 = 0, u1 = 0;
    x0 = SS[0][x0];
    x1 = SS[1][x1];
    u0 = x0 ^ S0x2(x1);
    u1 = S0x2(x0) ^ x1;
    u0 = SS[2][u0];
    u1 = SS[3][u1];
    x = (u0 << 4) | u1;
    return x;
}

/* Умножение на 2 в поле Галуа для перемножения матриц */
uint8_t Mx2(uint8_t x) {
    unsigned int gf = 0x11d, y = x << 1;
    if (y & (1 << 8)) y ^= gf;
    return y;
}

/* Умножение на 4 в поле Галуа для перемножения матриц */
uint8_t Mx4(uint8_t x) {
    uint8_t y = Mx2(Mx2(x));
    return y;
}

/* Умножение на 6 в поле Галуа для перемножения матриц */
uint8_t Mx6(uint8_t x) {
    uint8_t y = Mx2(x) ^ Mx4(x);
    return y;
}

/* Умножение на 8 в поле Галуа для перемножения матриц */
uint8_t Mx8(uint8_t x) {
    uint8_t y = Mx2(Mx2(Mx2(x)));
    return y;
}

/* Умножение на 10 в поле Галуа для перемножения матриц */
uint8_t Mxa(uint8_t x) {
    uint8_t y = Mx2(x) ^ Mx8(x);
    return y;
}

/* Функция F0 в сети Фейстеля*/
uint32_t F0(uint32_t x, uint32_t rk) {
    x = x ^ rk;
    uint8_t x0 = x >> 24, x1 = (x >> 16) & 0xff, x2 = (x >> 8) & 0xff, x3 = x & 0xff;
    x0 = S0(x0);
    x1 = S1[x1];
    x2 = S0(x2);
    x3 = S1[x3];
    uint32_t y = x0 ^ Mx2(x1) ^ Mx4(x2) ^ Mx6(x3);
    y = (y << 8) | (Mx2(x0) ^ x1 ^ Mx6(x2) ^ Mx4(x3));
    y = (y << 8) | (Mx4(x0) ^ Mx6(x1) ^ x2 ^ Mx2(x3));
    y = (y << 8) | (Mx6(x0) ^ Mx4(x1) ^ Mx2(x2) ^ x3);
    return y;
}

/* Функция F1 в сети Фейстеля*/
uint32_t F1(uint32_t x, uint32_t rk) {
    x = x ^ rk;
    uint8_t x0 = x >> 24, x1 = (x >> 16) & 0xff, x2 = (x >> 8) & 0xff, x3 = x & 0xff;
    x0 = S1[x0];
    x1 = S0(x1);
    x2 = S1[x2];
    x3 = S0(x3);
    uint32_t y = x0 ^ Mx8(x1) ^ Mx2(x2) ^ Mxa(x3);
    y = (y << 8) | (Mx8(x0) ^ x1 ^ Mxa(x2) ^ Mx2(x3));
    y = (y << 8) | (Mx2(x0) ^ Mxa(x1) ^ x2 ^ Mx8(x3));
    y = (y << 8) | (Mxa(x0) ^ Mx2(x1) ^ Mx8(x2) ^ x3);
    return y;
}

/* Реализация сети Фейстеля
 * x - шифруемый блок из 4-х элементов размером 4 байта
 * RK - массив раундовых ключей
 * rounds - количество раундов
 * enc - индикатор. Равен 1, если производится зашифрование. Иначе - 0*/
void GFN(uint32_t x[], uint32_t RK[], int rounds, uint8_t enc) {
    for (int i = 0; i < rounds; i++) {
    	if (enc){
    		x[1] = x[1] ^ F0(x[0], RK[i * 2]);
    		x[3] = x[3] ^ F1(x[2], RK[i * 2 + 1]);
    		uint32_t y0 = x[0];
    		x[0] = x[1];
    		x[1] = x[2];
    		x[2] = x[3];
    		x[3] = y0;
    	}
    	else{
            x[1] = x[1] ^ F0(x[0], RK[rounds * 2 - 2 - i * 2]);
            x[3] = x[3] ^ F1(x[2], RK[rounds * 2 - 1 - i * 2]);
            uint32_t y0 = x[0];
            x[0] = x[3];
            x[3] = x[2];
            x[2] = x[1];
            x[1] = y0;
    	}
    }
    uint32_t y0 = x[0];
    if (enc){
    	x[0] = x[3];
    	x[3] = x[2];
    	x[2] = x[1];
    	x[1] = y0;
    }
    else{
        x[0] = x[1];
        x[1] = x[2];
        x[2] = x[3];
        x[3] = y0;
    }
}

/* DoubleSwap для генерирования раундовых ключей
 * x - массив из 4-х элементов размером 4 байта*/
void DoubleSwap(uint32_t x[]) {
    uint32_t y1 = 0, y2 = 0;
    y1 = x[0] & 0xfe000000;
    y2 = x[3] & 0x0000007f;
    x[0] = (x[0] << 7) | (x[1] >> 25);
    x[1] = (x[1] << 7) | y2;
    x[3] = (x[2] << 25) | (x[3] >> 7);
    x[2] = y1 | (x[2] >> 7);
}

/* Генерация раундовых ключей
 * k - основной ключ
 * RK - массив для хранения раундовых ключей*/
void Keys(uint32_t k[], uint32_t RK[]) {
    uint32_t l[4] = {k[0], k[1], k[2], k[3]};
    GFN(l, con, 12, 1);
    for (int i = 0; i <= 8; i++) {
        uint32_t t0 = 0, t1 = 0, t2 = 0, t3 = 0;
        t0 = l[0] ^ con[24 + 4 * i];
        t1 = l[1] ^ con[24 + 4 * i + 1];
        t2 = l[2] ^ con[24 + 4 * i + 2];
        t3 = l[3] ^ con[24 + 4 * i + 3];
        if (i % 2 == 1) {
            t0 ^= k[0];
            t1 ^= k[1];
            t2 ^= k[2];
            t3 ^= k[3];
        }
        RK[4 * i] = t0;
        RK[4 * i + 1] = t1;
        RK[4 * i + 2] = t2;
        RK[4 * i + 3] = t3;
        DoubleSwap(l);
    }
}

/* Проверка введенных символов.
 * Если есть символ, не принадлежащий шестнадцатиричной системе - возвращает ошибку*/
void isRight(){
	uint8_t symbols[22] = {'0','1','2','3','4','5','6','7','8','9','A','a','B','b','C','c','D','d','E','e','F','f'};
	for(uint8_t i=0;i<32;i++){
		uint8_t p = 1;
		for (uint8_t j=0;j<22;j++)
			if (read[i] == symbols[j]){
				p = 0;
				break;
			}
		if(p){
			error = 1;
			return;
		}
	}
	error = 0;
}

int main()
{
    InitAll();

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART2, USART_IT_TC, ENABLE);

    while (1) {
        if (later) {           // Задание и генерация ключей
            ReadKey();
            Keys(k, RoundKeys);
            later = 0;
        }
        if (isBlock) {
            USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
            USART_ITConfig(USART2, USART_IT_TC, DISABLE);

            GetBlock(read, b);
#if (measurement)
            SCB_DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
            DWT_CYCCNT = 0;
            DWT_CONTROL |= 1;         //запуск счетчика
#endif
            if (readStatus == 0x31){
            	b[1] = b[1] ^ k[0];          // Шифрование блока
            	b[3] = b[3] ^ k[1];
            	GFN(b, RoundKeys, 18, 1);
            	b[1] = b[1] ^ k[2];
            	b[3] =b[3] ^ k[3];
            }
            else {
                b[1] = b[1] ^ k[2];         // Расшифрование блока
                b[3] = b[3] ^ k[3];
                GFN(b, RoundKeys, 18, 0);
                b[1] = b[1] ^ k[0];
                b[3] = b[3] ^ k[1];
            }

#if (measurement)
            count_tic = DWT_CYCCNT;
            uint32_t tic[4] = {0,0,0, count_tic};
            LittleBlock(tic);
#else
            LittleBlock(b);
#endif
            sendBlock = 1;
            isBlock = 0;
            readStatus = 0;
            USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
            USART_ITConfig(USART2, USART_IT_TC, ENABLE);
        }
    }
}
