
#include <zephyr.h>
#include <hal/nrf_timer.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_gpiote.h>
#include <hal/nrf_dppi.h>
#include <hal/nrf_clock.h>
#include <compiler_abstraction.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <drivers/clock_control.h>
#include <prism_dispatcher.h>
#include <event_manager.h>
#include <status_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MAIN, LOG_LEVEL_INF);

#define CONSOLE_LABEL DT_LABEL(DT_CHOSEN(zephyr_console))

#define AUX_PIN			12
#define DPPI_PIN		9
#define SYSCTL_TIMER 	NRF_TIMER1
#define MY_DPPI_EVENT	NRF_RTC_EVENT_TICK
#define WAIT_TICKS		(32768 * 4)
#define GPIOTE			NRF_GPIOTE

void TIMER1_IRQHandler(void *arg)
{
    //nrf_gpio_pin_clear(AUX_PIN);
    if (nrf_timer_event_check(SYSCTL_TIMER, NRF_TIMER_EVENT_COMPARE1)) {
        nrf_timer_event_clear(SYSCTL_TIMER, NRF_TIMER_EVENT_COMPARE1);
        nrf_gpiote_task_enable(GPIOTE, 0);
        //printk("XXXXXXX");
    }
    //nrf_gpio_pin_set(AUX_PIN);
}

static void backdoor_cb(uint64_t *value)
{}

void main(void)
{
    //nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLKSTARTED);
    //nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_HFCLKSTART);
	if (event_manager_init()) {
		LOG_ERR("Event Manager not initialized");
	} else {
		struct status_event *event = new_status_event();
		event->status = STATUS_INIT;
		EVENT_SUBMIT(event);
	}
	prism_dispatcher_backdoor_register(backdoor_cb);

	//nrf_802154_clock_hfclk_start();
	nrf_gpio_cfg_output(AUX_PIN);
	nrf_gpio_pin_clear(AUX_PIN);
	nrf_gpio_cfg_output(DPPI_PIN);
	nrf_gpio_pin_clear(DPPI_PIN);
	//nrf_gpio_cfg_output(11);
	//nrf_gpio_pin_clear(11);
	//nrf_gpio_cfg_output(8);
	//nrf_gpio_pin_clear(8);

	nrf_timer_frequency_set(SYSCTL_TIMER, NRF_TIMER_FREQ_1MHz);
	nrf_timer_cc_set(SYSCTL_TIMER, NRF_TIMER_CC_CHANNEL1, 100);
	nrf_timer_bit_width_set(SYSCTL_TIMER, NRF_TIMER_BIT_WIDTH_32);
	nrf_timer_int_enable(SYSCTL_TIMER, TIMER_INTENSET_COMPARE1_Msk);
	//nrf_timer_shorts_enable(SYSCTL_TIMER, 0x0200);
	nrf_timer_shorts_enable(SYSCTL_TIMER, NRF_TIMER_SHORT_COMPARE1_CLEAR_MASK);
	IRQ_CONNECT(TIMER1_IRQn, 1, TIMER1_IRQHandler, 0, 0);
	irq_enable(TIMER1_IRQn);
	nrf_timer_publish_set(SYSCTL_TIMER, NRF_TIMER_EVENT_COMPARE1, 0);
	nrf_timer_task_trigger(SYSCTL_TIMER, NRF_TIMER_TASK_START);

	nrf_gpiote_task_disable(GPIOTE, 0);
	nrf_gpiote_subscribe_set(GPIOTE, NRF_GPIOTE_TASK_SET_0, 0);
	nrf_gpiote_task_configure(GPIOTE, 0, DPPI_PIN, GPIOTE_CONFIG_POLARITY_LoToHi, NRF_GPIOTE_INITIAL_VALUE_LOW);
	nrf_gpiote_task_enable(GPIOTE, 0);

	nrf_dppi_channels_enable(NRF_DPPIC, 1);
	nrf_gpio_pin_mcu_select(11, NRF_GPIO_PIN_MCUSEL_NETWORK);
	nrf_gpio_pin_mcu_select(23, NRF_GPIO_PIN_MCUSEL_NETWORK);

	//nrf_rtc_task_trigger(RTC, NRF_RTC_TASK_START);

	while(1)
	{
		nrf_timer_task_trigger(SYSCTL_TIMER, NRF_TIMER_TASK_CAPTURE0);
		printk("CT:%u %x\n", (uint32_t)SYSCTL_TIMER->CC[0], NRF_CLOCK->HFCLKSTAT);
		k_sleep(K_MSEC(1000));
		//nrf_gpiote_task_trigger(GPIOTE, NRF_GPIOTE_TASK_SET_0);
	}
}
