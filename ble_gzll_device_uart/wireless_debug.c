/* Copyright (c) 2013 Nordic Semiconductor. All Rights Reserved.
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
 
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nrf_assert.h"
#include "nrf_soc.h"
#include "nrf_gzll.h"
#include "app_error.h"
#include "wireless_debug.h"
#include "app_util_platform.h"
 
//void notification_cb(nrf_impl_notification_t notification);
/*lint -e526 "Symbol RADIO_IRQHandler not defined" */
void RADIO_IRQHandler(void);

#define PIPE_NUMBER          0
#define TX_PAYLOAD_LENGTH   32
#define ACK_PAYLOAD_LENGTH  32

static nrf_radio_request_t  m_timeslot_request;
static uint32_t             m_slot_length;
static volatile bool        m_cmd_received = false;
static volatile bool        m_gzll_initialized = false;

static nrf_radio_signal_callback_return_param_t signal_callback_return_param;
static uint8_t ack_payload[ACK_PAYLOAD_LENGTH];

void HardFault_Handler(uint32_t program_counter, uint32_t link_register)
{
}

void m_configure_next_event(void)
{
    m_slot_length                                  = 25000;
    m_timeslot_request.request_type                = NRF_RADIO_REQ_TYPE_EARLIEST;
    m_timeslot_request.params.earliest.hfclk       = NRF_RADIO_HFCLK_CFG_DEFAULT;
    m_timeslot_request.params.earliest.priority    = NRF_RADIO_PRIORITY_NORMAL;
    m_timeslot_request.params.earliest.length_us   = m_slot_length;
    m_timeslot_request.params.earliest.timeout_us  = 1000000;
}

void sys_evt_dispatch(uint32_t evt_id)
{
    uint32_t err_code;
    
    switch (evt_id)
    {
        case NRF_EVT_RADIO_SIGNAL_CALLBACK_INVALID_RETURN:
            ASSERT(false);
            break;
        
        case NRF_EVT_RADIO_SESSION_IDLE:
            ASSERT(false);
            break;

        case NRF_EVT_RADIO_SESSION_CLOSED:
            ASSERT(false);
            break;

        case NRF_EVT_RADIO_BLOCKED:
            m_configure_next_event();
            err_code = sd_radio_request(&m_timeslot_request);
            APP_ERROR_CHECK(err_code);
            break;

        case NRF_EVT_RADIO_CANCELED:
            m_configure_next_event();
            err_code = sd_radio_request(&m_timeslot_request);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            break;
    }
}

static void m_on_start(void)
{
    bool res;
    signal_callback_return_param.params.request.p_next = NULL;
    signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;
    
    if (!m_gzll_initialized)
    {
        res = nrf_gzll_init(NRF_GZLL_MODE_DEVICE);
        ASSERT(res);
        res = nrf_gzll_set_device_channel_selection_policy(NRF_GZLL_DEVICE_CHANNEL_SELECTION_POLICY_USE_CURRENT);
        ASSERT(res);
        res = nrf_gzll_set_xosc_ctl(NRF_GZLL_XOSC_CTL_MANUAL);
        ASSERT(res);
        res = nrf_gzll_set_max_tx_attempts(0);
        ASSERT(res);
        res = nrf_gzll_set_base_address_0(0xE7E7E7E7);
        ASSERT(res);
        res = nrf_gzll_enable();
        ASSERT(res);
        m_gzll_initialized = true;
    }
    else
    {
        res = nrf_gzll_set_mode(NRF_GZLL_MODE_DEVICE);
        ASSERT(res);
    }
    NRF_TIMER0->INTENSET = TIMER_INTENSET_COMPARE0_Msk;
    NRF_TIMER0->CC[0] = m_slot_length - 4000; // TODO: Use define instead of magic number
    NVIC_EnableIRQ(TIMER0_IRQn);    
}

static void m_on_multitimer(void)
{
    NRF_TIMER0->EVENTS_COMPARE[0] = 0;
    if (nrf_gzll_get_mode() != NRF_GZLL_MODE_SUSPEND)
    {
        signal_callback_return_param.params.request.p_next = NULL;
        signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;
        (void)nrf_gzll_set_mode(NRF_GZLL_MODE_SUSPEND);
        NRF_TIMER0->INTENSET = TIMER_INTENSET_COMPARE0_Msk;
        NRF_TIMER0->CC[0] = m_slot_length - 1000;
    }
    else
    {
        ASSERT(nrf_gzll_get_mode() == NRF_GZLL_MODE_SUSPEND);
        m_configure_next_event();
        signal_callback_return_param.params.request.p_next = &m_timeslot_request;
        signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_REQUEST_AND_END;
    }
}

nrf_radio_signal_callback_return_param_t * m_radio_callback(uint8_t signal_type)
{   
    switch(signal_type)
    {
        case NRF_RADIO_CALLBACK_SIGNAL_TYPE_START:
            m_on_start();
            break;

        case NRF_RADIO_CALLBACK_SIGNAL_TYPE_RADIO:
            signal_callback_return_param.params.request.p_next = NULL;
            signal_callback_return_param.callback_action = NRF_RADIO_SIGNAL_CALLBACK_ACTION_NONE;
            RADIO_IRQHandler();
            break;

        case NRF_RADIO_CALLBACK_SIGNAL_TYPE_TIMER0:
            m_on_multitimer();
            break;
    }
    return (&signal_callback_return_param);
}

uint32_t gazell_sd_radio_init(void)
{
    uint32_t err_code;

    err_code = sd_radio_session_open(m_radio_callback);
    if (err_code != NRF_SUCCESS)
        return err_code;
    m_configure_next_event();
    err_code = sd_radio_request(&m_timeslot_request);
    if (err_code != NRF_SUCCESS)
    {
        (void)sd_radio_session_close();
        return err_code;
    }
    return NRF_SUCCESS;
}


void nrf_gzll_device_tx_success(uint32_t pipe, nrf_gzll_device_tx_info_t tx_info)
{
    uint32_t ack_payload_length = ACK_PAYLOAD_LENGTH; 
    if (tx_info.payload_received_in_ack)
    {      
        if (nrf_gzll_fetch_packet_from_rx_fifo(pipe, ack_payload, &ack_payload_length))
        {
            ASSERT(ack_payload_length == 1);
            m_cmd_received = true;
        }
    }
}

void nrf_gzll_device_tx_failed(uint32_t pipe, nrf_gzll_device_tx_info_t tx_info)
{
}

void nrf_gzll_host_rx_data_ready(uint32_t pipe, nrf_gzll_host_rx_info_t rx_info)
{
}

void nrf_gzll_disabled(void)
{
}

bool debug_cmd_available(void)
{
    return m_cmd_received;
}

char get_debug_cmd(void)
{
    char cmd = ack_payload[0];
    m_cmd_received = false;
    return cmd;
}
