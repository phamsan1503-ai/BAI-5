#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "misc.h"
#include <string.h>

#define LED_PIN     GPIO_Pin_13
#define LED_PORT    GPIOC

volatile char rx_buffer[10];
volatile uint8_t rx_index = 0;

void GPIO_Config(void);
void USART1_Config(void);

int main(void)
{
    GPIO_Config();
    USART1_Config();

    while(1)
    {

    }
}


void GPIO_Config(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitTypeDef gpio;
    gpio.GPIO_Pin = LED_PIN;          
    gpio.GPIO_Mode = GPIO_Mode_Out_PP; 
    gpio.GPIO_Speed = GPIO_Speed_2MHz; 
    GPIO_Init(LED_PORT, &gpio);

    GPIO_SetBits(LED_PORT, LED_PIN); 
}

void USART1_Config(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    NVIC_InitTypeDef nvic;

    // PA9 = TX, PA10 = RX
    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;     
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    usart.USART_BaudRate = 9600;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &usart);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    nvic.NVIC_IRQChannel = USART1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 0;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    USART_Cmd(USART1, ENABLE);
}

void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        char c = USART_ReceiveData(USART1);

        if(rx_index < sizeof(rx_buffer)-1)
        {
            rx_buffer[rx_index++] = c;
            rx_buffer[rx_index] = '\0'; 
        }

        if(strcmp((char*)rx_buffer, "ON") == 0)
        {
            GPIO_ResetBits(LED_PORT, LED_PIN); 
            rx_index = 0; // reset buffer
        }
        else if(strcmp((char*)rx_buffer, "OFF") == 0)
        {
            GPIO_SetBits(LED_PORT, LED_PIN);   
            rx_index = 0;
        }

        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}
