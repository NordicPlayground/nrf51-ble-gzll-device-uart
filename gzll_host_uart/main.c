/* Copyright (c) 2015 Nordic Semiconductor. All Rights Reserved.
*
* The information contained herein is property of Nordic Semiconductor ASA.
* Terms and conditions of usage are described in detail in NORDIC
* SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
*
* Licensees are granted free, non-transferable use of the information. NO
* WARRANTY of ANY KIND is provided. This heading must NOT be removed from
* the file.
*
*/

#include <stdio.h>
#include <string.h>
#include "nrf_gzll.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "boards.h"
#include "app_uart.h"
#include "app_util_platform.h"

// Data and acknowledgement payloads
static uint8_t data_payload[NRF_GZLL_CONST_MAX_PAYLOAD_LENGTH];  ///< Placeholder for data payload received from host. 

// Debug helper variables
extern nrf_gzll_error_code_t nrf_gzll_error_code;   ///< Error code

void uart_event_handler(app_uart_evt_t *p_app_uart_event){}

static uint32_t uart_init(void)
{
    uint32_t err_code;
    
    app_uart_comm_params_t uart_params = {.rx_pin_no = RX_PIN_NUMBER,
                                          .tx_pin_no = TX_PIN_NUMBER,
                                          .rts_pin_no = RTS_PIN_NUMBER,
                                          .cts_pin_no = CTS_PIN_NUMBER,
                                          .flow_control = (HWFC ? APP_UART_FLOW_CONTROL_ENABLED : APP_UART_FLOW_CONTROL_DISABLED),
                                          .use_parity = false,
                                          .baud_rate = UART_BAUDRATE_BAUDRATE_Baud38400};
    
    APP_UART_FIFO_INIT(&uart_params, 1, 64, uart_event_handler, APP_IRQ_PRIORITY_LOW, err_code);    

    return err_code;                                          
}

int main()
{
    // Initialize Gazell
    nrf_gzll_init(NRF_GZLL_MODE_HOST);  
    
    nrf_gzll_set_base_address_0(0xE7E7E7E7);
    
    uart_init();
    
    printf("Gazell UART relay\r\n");

    // Enable Gazell to start sending over the air
    nrf_gzll_enable();

    while(1)
    {
        // Send the CPU to sleep while waiting for a callback.
        __WFE();
    }
}

void nrf_gzll_host_rx_data_ready(uint32_t pipe, nrf_gzll_host_rx_info_t rx_info)
{   
    static uint8_t uart_buf[21];
    
    uint32_t data_payload_length = NRF_GZLL_CONST_MAX_PAYLOAD_LENGTH;

    // Pop packet and write first byte of the payload to the GPIO port.
    nrf_gzll_fetch_packet_from_rx_fifo(pipe, data_payload, &data_payload_length);
    
    if (data_payload_length > 0 && data_payload_length <= 20)
    {
        // Copy the gazell packet to a buffer, and make sure the string is null terminated
        memcpy(uart_buf, data_payload, data_payload_length);
        uart_buf[data_payload_length] = 0;
        // Output the data over the UART
        printf("RX: %s\r\n", uart_buf);
    }
}

// Callbacks not needed in this example.
void nrf_gzll_device_tx_success(uint32_t pipe, nrf_gzll_device_tx_info_t tx_info) {}
void nrf_gzll_device_tx_failed(uint32_t pipe, nrf_gzll_device_tx_info_t tx_info) {}
void nrf_gzll_disabled() {}
