#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zmk/event_manager.h>
#include <zmk/events/split_peripheral_status_changed.h>

static const struct gpio_dt_spec led_r = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led_g = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec led_b = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

static bool s_connected = false;
static int  s_step      = 0;

static struct k_work_delayable blink_work;

static void leds_off(void) {
    gpio_pin_set_dt(&led_r, 0);
    gpio_pin_set_dt(&led_g, 0);
    gpio_pin_set_dt(&led_b, 0);
}

static void blink_handler(struct k_work *work) {
    if (s_connected) {
        // 青 ゆっくり点滅（500ms on / 1500ms off）
        if (s_step % 2 == 0) {
            gpio_pin_set_dt(&led_r, 0);
            gpio_pin_set_dt(&led_g, 0);
            gpio_pin_set_dt(&led_b, 1);
            k_work_schedule(&blink_work, K_MSEC(500));
        } else {
            leds_off();
            k_work_schedule(&blink_work, K_MSEC(1500));
        }
    } else {
        // 赤 ゆっくり点滅（500ms on / 1500ms off）
        if (s_step % 2 == 0) {
            gpio_pin_set_dt(&led_r, 1);
            gpio_pin_set_dt(&led_g, 0);
            gpio_pin_set_dt(&led_b, 0);
            k_work_schedule(&blink_work, K_MSEC(500));
        } else {
            leds_off();
            k_work_schedule(&blink_work, K_MSEC(1500));
        }
    }
    s_step++;
}

static void refresh(void) {
    s_step = 0;
    k_work_cancel_delayable(&blink_work);
    leds_off();
    k_work_schedule(&blink_work, K_MSEC(50));
}

static int split_listener(const zmk_event_t *eh) {
    const struct zmk_split_peripheral_status_changed *ev =
        as_zmk_split_peripheral_status_changed(eh);
    s_connected = ev->connected;
    refresh();
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(led_status_peripheral, split_listener);
ZMK_SUBSCRIPTION(led_status_peripheral, zmk_split_peripheral_status_changed);

static int led_status_init(void) {
    gpio_pin_configure_dt(&led_r, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_g, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_b, GPIO_OUTPUT_INACTIVE);
    k_work_init_delayable(&blink_work, blink_handler);
    // 起動時は未接続扱いで赤点滅スタート
    s_connected = false;
    refresh();
    return 0;
}

SYS_INIT(led_status_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
