/*
 * Copyright (c) 2016-2020 Nordic Semiconductor ASA
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <drivers/timer/system_timer.h>
#include <sys_clock.h>
#include <spinlock.h>

#include <hal/nrf_clock.h>
#if (IS_ENABLED(CONFIG_NRF_TIME_SOURCE_TIMER0))
#include <hal/nrf_timer.h>
#elif (IS_ENABLED(CONFIG_NRF_TIME_SOURCE_RTC1))
#include <hal/nrf_rtc.h>
#endif

#if IS_ENABLED(CONFIG_NRF_TIME_SOURCE_TIMER0)

#define SYSTEM_TIMER NRF_TIMER0
#define SYSTEM_TIMER_IRQn NRFX_IRQ_NUMBER_GET(SYSTEM_TIMER)
#define SYSTEM_TIMER_LABEL timer1

#define DEFAULT_FREQUENCY_SETTING NRF_TIMER_FREQ_16MHz

#define COUNTER_MAX (0xFFFFFFFF)
#define COUNTER_HALF_SPAN (BIT(31))
#elif IS_ENABLED(CONFIG_NRF_TIME_SOURCE_RTC1)
#define SYSTEM_TIMER NRF_RTC1
#define SYSTEM_TIMER_IRQn NRFX_IRQ_NUMBER_GET(SYSTEM_TIMER)
#define SYSTEM_TIMER_LABEL rtc1
#define COUNTER_SPAN BIT(24)
#define COUNTER_HALF_SPAN (COUNTER_SPAN / 2U)
#define COUNTER_MAX (COUNTER_SPAN - 1U)
#endif // IS_ENABLED(CONFIG_NRF_TIME_SOURCE_TIMER0)

#define CYC_PER_TICK (sys_clock_hw_cycles_per_sec()	\
		      / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_TICKS ((COUNTER_HALF_SPAN - CYC_PER_TICK) / CYC_PER_TICK)
#define MAX_CYCLES (MAX_TICKS * CYC_PER_TICK)

static struct k_spinlock lock;

static uint32_t last_count;

#if IS_ENABLED(CONFIG_NRF_TIME_SOURCE_TIMER0)
static nrf_timer_frequency_t convert_frequency_value(uint32_t frequency_hz)
{
	switch (frequency_hz) {
		case 16000000:
			return NRF_TIMER_FREQ_16MHz;
		case 8000000:
			return NRF_TIMER_FREQ_8MHz;
		case 4000000:
			return NRF_TIMER_FREQ_4MHz;
		case 2000000:
			return NRF_TIMER_FREQ_2MHz;
		case 1000000:
			return NRF_TIMER_FREQ_1MHz;
		case 500000:
			return NRF_TIMER_FREQ_500kHz;
		case 250000:
			return NRF_TIMER_FREQ_250kHz;
		case 125000:
			return NRF_TIMER_FREQ_125kHz;
		case 62500:
			return NRF_TIMER_FREQ_62500Hz;
		case 31250:
			return NRF_TIMER_FREQ_31250Hz;
		default:
			__ASSERT_NO_MSG(false);
			return DEFAULT_FREQUENCY_SETTING;
	}
}

static int HFCLK_init(struct device *dev)
{
	ARG_UNUSED(dev);
	nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLKSTARTED);
	nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_HFCLKSTART);
	while (!nrf_clock_hf_is_running(NRF_CLOCK, NRF_CLOCK_HFCLK_HIGH_ACCURACY)) {};
	return 0;
}
SYS_INIT(HFCLK_init, PRE_KERNEL_1, 0);
#endif

static uint32_t counter_sub(uint32_t a, uint32_t b)
{
	return (a - b) & COUNTER_MAX;
}

static void set_comparator(uint32_t cyc)
{
#if IS_ENABLED(CONFIG_NRF_TIME_SOURCE_TIMER0)
	nrf_timer_cc_set(SYSTEM_TIMER, 0, cyc & COUNTER_MAX);
#elif IS_ENABLED(CONFIG_NRF_TIME_SOURCE_RTC1)
	nrf_rtc_cc_set(SYSTEM_TIMER, 0, cyc & COUNTER_MAX);
#endif
}

static uint32_t get_comparator(void)
{
#if IS_ENABLED(CONFIG_NRF_TIME_SOURCE_TIMER0)
	return nrf_timer_cc_get(SYSTEM_TIMER, 0);
#elif IS_ENABLED(CONFIG_NRF_TIME_SOURCE_RTC1)
	return nrf_rtc_cc_get(SYSTEM_TIMER, 0);
#endif
}

static void event_clear(void)
{
#if IS_ENABLED(CONFIG_NRF_TIME_SOURCE_TIMER0)
	nrf_timer_event_clear(SYSTEM_TIMER, NRF_TIMER_EVENT_COMPARE0);
#elif IS_ENABLED(CONFIG_NRF_TIME_SOURCE_RTC1)
	nrf_rtc_event_clear(SYSTEM_TIMER, NRF_RTC_EVENT_COMPARE_0);
#endif
}

static void event_enable(void)
{
#if IS_ENABLED(CONFIG_NRF_TIME_SOURCE_TIMER0)
#elif IS_ENABLED(CONFIG_NRF_TIME_SOURCE_RTC1)
	nrf_rtc_event_enable(SYSTEM_TIMER, NRF_RTC_INT_COMPARE0_MASK);
#endif
}

static void int_disable(void)
{
#if IS_ENABLED(CONFIG_NRF_TIME_SOURCE_TIMER0)
	nrf_timer_int_disable(SYSTEM_TIMER, NRF_TIMER_INT_COMPARE0_MASK);
#elif IS_ENABLED(CONFIG_NRF_TIME_SOURCE_RTC1)
	nrf_rtc_int_disable(SYSTEM_TIMER, NRF_RTC_INT_COMPARE0_MASK);
#endif
}

static void int_enable(void)
{
#if IS_ENABLED(CONFIG_NRF_TIME_SOURCE_TIMER0)
	nrf_timer_int_enable(SYSTEM_TIMER, NRF_TIMER_INT_COMPARE0_MASK);
#elif IS_ENABLED(CONFIG_NRF_TIME_SOURCE_RTC1)
	nrf_rtc_int_enable(SYSTEM_TIMER, NRF_RTC_INT_COMPARE0_MASK);
#endif
}

static void task_clear(void)
{
#if IS_ENABLED(CONFIG_NRF_TIME_SOURCE_TIMER0)
	nrf_timer_task_trigger(SYSTEM_TIMER, NRF_TIMER_TASK_CLEAR);
#elif IS_ENABLED(CONFIG_NRF_TIME_SOURCE_RTC1)
	nrf_rtc_task_trigger(SYSTEM_TIMER, NRF_RTC_TASK_CLEAR);
#endif
}
static void task_start(void)
{
#if IS_ENABLED(CONFIG_NRF_TIME_SOURCE_TIMER0)
	nrf_timer_task_trigger(SYSTEM_TIMER, NRF_TIMER_TASK_START);
#elif IS_ENABLED(CONFIG_NRF_TIME_SOURCE_RTC1)
	nrf_rtc_task_trigger(SYSTEM_TIMER, NRF_RTC_TASK_START);
#endif
}




static uint32_t counter(void)
{
#if IS_ENABLED(CONFIG_NRF_TIME_SOURCE_TIMER0)
	nrf_timer_task_trigger(SYSTEM_TIMER, NRF_TIMER_TASK_CAPTURE1);
	return nrf_timer_cc_get(SYSTEM_TIMER, NRF_TIMER_CC_CHANNEL1);
#elif IS_ENABLED(CONFIG_NRF_TIME_SOURCE_RTC1)
	return nrf_rtc_counter_get(SYSTEM_TIMER);
#endif
}

/* Function ensures that previous CC value will not set event */
static void prevent_false_prev_evt(void)
{
	uint32_t now = counter();
	uint32_t prev_val;

	/* First take care of a risk of an event coming from CC being set to the
	 * next cycle.
	 * Reconfigure CC to the future. If CC was set to next cycle we need to
	 * wait for up to 15 us (half of 32 kHz interval) and clean a potential
	 * event. After that there is no risk of unwanted event.
	 */
	prev_val = get_comparator();
	event_clear();
	set_comparator(now);
	event_enable();

	if (counter_sub(prev_val, now) == 1) {
#if IS_ENABLED(CONFIG_NRF_TIME_SOURCE_RTC1)
		k_busy_wait(15);
#endif
		event_clear();
	}

	/* Clear interrupt that may have fired as we were setting the
	 * comparator.
	 */
	NRFX_IRQ_PENDING_CLEAR(SYSTEM_TIMER_IRQn);
}

/* If alarm is next SYSTEM_TIMER cycle from now, function attempts to adjust. If
 * counter progresses during that time it means that 1 cycle elapsed and
 * interrupt is set pending.
 */
static void handle_next_cycle_case(uint32_t t)
{
	set_comparator(t + 2);
	while (t != counter()) {
		/* Already expired, time elapsed but event might not be
		 * generated. Trigger interrupt.
		 */
		t = counter();
		set_comparator(t + 2);
	}
}

/* Function safely sets absolute alarm. It assumes that provided value is
 * less than MAX_CYCLES from now. It detects late setting and also handles
 * +1 cycle case.
 */
static void set_absolute_alarm(uint32_t abs_val)
{
	uint32_t diff;
	uint32_t t = counter();

	diff = counter_sub(abs_val, t);
	if (diff == 1 && IS_ENABLED(CONFIG_NRF_TIME_SOURCE_RTC1)) {
		handle_next_cycle_case(t);
		return;
	}

	set_comparator(abs_val);
	t = counter();
	/* A little trick, subtract 2 to force now and now + 1 case fall into
	 * negative (> MAX_CYCLES). Diff 0 means two cycles from now.
	 */
	diff = counter_sub(abs_val - 2, t);
	if (diff > MAX_CYCLES) {
		/* Already expired, set for subsequent cycle. */
		/* It is possible that setting CC was interrupted and CC might
		 * be set to COUNTER+1 value which will not generate an event.
		 * In that case, special handling is performed (attempt to set
		 * CC to COUNTER+2). In case of using NRF_TIMERx peripheral
		 * some ticks must be added dependently on frequency. At the
		 * moment we're assuming, that 1us is enough.
		 */
		if(IS_ENABLED(CONFIG_NRF_TIME_SOURCE_RTC1)) {
			handle_next_cycle_case(t);
		}
		else if (IS_ENABLED(CONFIG_NRF_TIME_SOURCE_TIMER0)) {
			uint32_t offs = CONFIG_NRF_SYSTEM_TIMER_FREQUENCY_HZ / 1000000;
			set_comparator(t + offs);
		}
	}
}

/* Sets relative alarm from any context. Function is lockless. It only
 * blocks SYSTEM_TIMER interrupt.
 */
static void set_protected_absolute_alarm(uint32_t cycles)
{
	int_disable();

	prevent_false_prev_evt();

	set_absolute_alarm(cycles);

	int_enable();
}

/* Note: this function has public linkage, and MUST have this
 * particular name.  The platform architecture itself doesn't care,
 * but there is a test (tests/arch/arm_irq_vector_table) that needs
 * to find it to it can set it in a custom vector table.  Should
 * probably better abstract that at some point (e.g. query and reset
 * it by pointer at runtime, maybe?) so we don't have this leaky
 * symbol.
 */
void rtc_nrf_isr(const void *arg)
{
	ARG_UNUSED(arg);
	event_clear();

	uint32_t t = get_comparator();
	uint32_t dticks = counter_sub(t, last_count) / CYC_PER_TICK;

	last_count += dticks * CYC_PER_TICK;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* protection is not needed because we are in the SYSTEM_TIMER interrupt
		 * so it won't get preempted by the interrupt.
		 */
		set_absolute_alarm(last_count + CYC_PER_TICK);
	}

	z_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? dticks : (dticks > 0));
}

int z_clock_driver_init(const struct device *device)
{
	ARG_UNUSED(device);
	static const enum nrf_lfclk_start_mode mode =
		IS_ENABLED(CONFIG_SYSTEM_CLOCK_NO_WAIT) ?
			CLOCK_CONTROL_NRF_LF_START_NOWAIT :
			(IS_ENABLED(CONFIG_SYSTEM_CLOCK_WAIT_FOR_AVAILABILITY) ?
			CLOCK_CONTROL_NRF_LF_START_AVAILABLE :
			CLOCK_CONTROL_NRF_LF_START_STABLE);

	/* TODO: replace with counter driver to access SYSTEM_TIMER */
#if (IS_ENABLED(CONFIG_NRF_TIME_SOURCE_TIMER0))
	nrf_timer_bit_width_set(SYSTEM_TIMER, NRF_TIMER_BIT_WIDTH_32);
	nrf_timer_frequency_set(SYSTEM_TIMER,
		convert_frequency_value(CONFIG_NRF_SYSTEM_TIMER_FREQUENCY_HZ));

#elif (IS_ENABLED(CONFIG_NRF_TIME_SOURCE_RTC1))
	nrf_rtc_prescaler_set(SYSTEM_TIMER, 0);
#endif
	event_clear();
	NRFX_IRQ_PENDING_CLEAR(SYSTEM_TIMER_IRQn);
	int_enable();

	IRQ_CONNECT(SYSTEM_TIMER_IRQn,
		    DT_IRQ(DT_NODELABEL(SYSTEM_TIMER_LABEL),priority),
		    rtc_nrf_isr, 0, 0);
	irq_enable(SYSTEM_TIMER_IRQn);

	task_clear();
	task_start();

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		set_comparator(counter() + CYC_PER_TICK);
	}


#if (IS_ENABLED(CONFIG_NRF_TIME_SOURCE_RTC1))
	z_nrf_clock_control_lf_on(mode);
#endif
	return 0;
}

void z_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);
	uint32_t cyc;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks - 1, 0, (int32_t)MAX_TICKS);

	uint32_t unannounced = counter_sub(counter(), last_count);

	/* If we haven't announced for more than half the 24-bit wrap
	 * duration, then force an announce to avoid loss of a wrap
	 * event.  This can happen if new timeouts keep being set
	 * before the existing one triggers the interrupt.
	 */
	if (unannounced >= COUNTER_HALF_SPAN) {
		ticks = 0;
	}

	/* Get the cycles from last_count to the tick boundary after
	 * the requested ticks have passed starting now.
	 */
	cyc = ticks * CYC_PER_TICK + 1 + unannounced;
	cyc += (CYC_PER_TICK - 1);
	cyc = (cyc / CYC_PER_TICK) * CYC_PER_TICK;

	/* Due to elapsed time the calculation above might produce a
	 * duration that laps the counter.  Don't let it.
	 */
	if (cyc > MAX_CYCLES) {
		cyc = MAX_CYCLES;
	}

	cyc += last_count;
	set_protected_absolute_alarm(cyc);
}

uint32_t z_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = counter_sub(counter(), last_count) / CYC_PER_TICK;

	k_spin_unlock(&lock, key);
	return ret;
}

uint32_t z_timer_cycle_get_32(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = counter_sub(counter(), last_count) + last_count;

	k_spin_unlock(&lock, key);
	return ret;
}
