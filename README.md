# Bài 5 - Truyền thông nối tiếp UART

## Nội dung bài tập

_Bài tập yêu cầu:_

- Giao tiếp nối tiếp giữa STM32 và máy tính

- Gửi chuỗi "Hello from STM32" tới máy tính.

- khi gửi một ký tự từ máy tính, STM32 sẽ phản hồi lại ký tự đó

- Khi gửi chuỗi "ON" thì bật đèn, "OFF" thì tắt đèn

- Sử dụng ngắt để nhận chuỗi dữ liệu

***File code kết quả: [Link](https://github.com/NguyenVuTatKhang/EmbeddedSystemNhom1/blob/main/bai5/user/main.c)***


### 1. Cấu hình UART

- Bật clock cho GPIOA.

	+ PA9: TX, chế độ AF_PP.

	+ PA10: RX, chế độ IN_FLOATING.

  + Cấu hình ngắt cho USART1

```c
void USART1_Config(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    // PA9 (TX)
    GPIO_InitTypeDef gpio;
    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);

    // PA10 (RX)
    gpio.GPIO_Pin = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    USART_InitTypeDef usart;
    usart.USART_BaudRate = 9600;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &usart);

    USART_Cmd(USART1, ENABLE);


    NVIC_InitTypeDef nvic;
    nvic.NVIC_IRQChannel = USART1_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 0;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
}
```

### 2. Gửi chuỗi "Hello from STM32" tới máy tính

- Bật clock cho TIM2.

- Cấu hình Timer2 với tần số ngắt định kỳ (mỗi ~1 giây).

- Cho phép ngắt TIM2 và bật trong NVIC.

```c
void TIM2_Config(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseInitTypeDef tim;
    tim.TIM_Prescaler = 7200 - 1;      
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    tim.TIM_Period = 10000 - 1;       
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    tim.TIM_RepetitionCounter = 0;

    TIM_TimeBaseInit(TIM2, &tim);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);

    NVIC_InitTypeDef nvic;
    nvic.NVIC_IRQChannel = TIM2_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
}
```

- Copy chuỗi cần gửi vào buffer txBuffer.

- Gửi ký tự đầu tiên qua USART.

- Bật ngắt TXE để gửi các ký tự tiếp theo trong ngắt.
```c
	void USART1_SendString_IT(char *str)
{
    strcpy(txBuffer, str);
    txIndex = 0;
    USART_SendData(USART1, txBuffer[txIndex++]);
    USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
}
```

- Khi TXE = 1 (USART sẵn sàng gửi):

- Nếu còn ký tự → gửi tiếp.

- Nếu hết chuỗi → tắt ngắt TXE.

```c
	void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_TXE) != RESET)
    {
        if (txBuffer[txIndex] != '\0')
        {
            USART_SendData(USART1, txBuffer[txIndex++]);
        }
        else
        {
            USART_ITConfig(USART1, USART_IT_TXE, DISABLE); 
        }
    }
}
```

- Khi Timer2 tràn (hết chu kỳ):

- Xóa cờ ngắt.

- Gọi USART1_SendString_IT("Hello from STM32\r\n") để gửi chuỗi.
```c
	void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

        USART1_SendString_IT("Hello from STM32\r\n");
    }
}
```
### 3. Gửi một ký tự từ máy tính, STM32 sẽ phản hồi lại ký tự đó

_Máy tính gửi → USART STM32 nhận → ngắt RXNE gọi hàm xử lý → STM32 ghi dữ liệu vào TDR để gửi ngược lại máy tính.:_

- Chờ đến khi cờ TXE (Transmit Data Register Empty) = 1.

- Gửi 1 ký tự ra cổng USART1.
```c
void USART1_SendChar(char c)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, c);
}
```

- Khi có ngắt RXNE (nhận dữ liệu xong 1 ký tự):

- Đọc ký tự từ thanh ghi nhận.

- Gửi lại ký tự vừa nhận ra USART (echo).

- Thêm \r\n để xuống dòng trên terminal.

```c
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        uint16_t data = USART_ReceiveData(USART1);

       
        USART1_SendChar((char)data);  
        USART1_SendChar('\r');        
        USART1_SendChar('\n');
    }
}
```
### 4. Gửi chuỗi "ON" thì bật đèn, "OFF" thì tắt đèn

_Chức năng chính:_
Nhận chuỗi từ máy tính qua USART, so sánh nội dung.

- Nếu nhận "ON" → bật LED (GPIO_ResetBits).

- Nếu nhận "OFF" → tắt LED (GPIO_SetBits).

_Nguyên lý hoạt động:_

- Khi có dữ liệu đến, cờ RXNE bật → vào hàm ngắt.

- Đọc ký tự nhận được và lưu vào rx_buffer.

- Thêm ký tự '\0' để chuỗi luôn kết thúc đúng chuẩn C.

- So sánh nội dung buffer với "ON" hoặc "OFF".

- Nếu khớp → điều khiển LED và reset lại rx_index để chuẩn bị nhận chuỗi mới.

- Xóa cờ ngắt RXNE.



```c
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
```

