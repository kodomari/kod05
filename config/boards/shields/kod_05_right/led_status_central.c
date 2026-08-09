#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/ble.h>

static const struct gpio_dt_spec led_r = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led_g = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec led_b = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

static int  s_profile   = 0;
static bool s_connected = false;
static bool s_pairing   = false;
static int  s_step      = 0;

static struct k_work_delayable blink_work;

static void leds_off(void) {
    gpio_pin_set_dt(&led_r, 0);
    gpio_pin_set_dt(&led_g, 0);
    gpio_pin_set_dt(&led_b, 0);
}

static void blink_handler(struct k_work *work) {
    if (s_pairing) {
        // 黄（赤+緑）200ms点滅
        if (s_step % 2 == 0) {
            gpio_pin_set_dt(&led_r, 1);
            gpio_pin_set_dt(&led_g, 1);
            gpio_pin_set_dt(&led_b, 0);
        } else {
            leds_off();
        }
        s_step++;
        k_work_schedule(&blink_work, K_MSEC(200));
        return;
    }

    if (!s_connected) {
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
        s_step++;
        return;
    }

    // 青 N回点滅（N = プロファイル番号+1）
    int n = s_profile + 1;
    int blink_steps = n * 2;

    if (s_step < blink_steps) {
        if (s_step % 2 == 0) {
            gpio_pin_set_dt(&led_b, 1);
        } else {
            gpio_pin_set_dt(&led_b, 0);
        }
        s_step++;
        k_work_schedule(&blink_work, K_MSEC(150));
    } else {
        leds_off();
        s_step = 0;
        k_work_schedule(&blink_work, K_MSEC(1000));
    }
}

static void refresh(void) {
    s_step = 0;
    k_work_cancel_delayable(&blink_work);
    leds_off();
    k_work_schedule(&blink_work, K_MSEC(50));
}

static int ble_listener(const zmk_event_t *eh) {
    s_profile   = zmk_ble_active_profile_index();
    s_connected = zmk_ble_active_profile_is_connected();
    s_pairing   = !zmk_ble_active_profile_is_connected()
                  && zmk_ble_active_profile_is_open();
    refresh();
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(led_status, ble_listener);
ZMK_SUBSCRIPTION(led_status, zmk_ble_active_profile_changed);

static int led_status_init(void) {
    gpio_pin_configure_dt(&led_r, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_g, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_b, GPIO_OUTPUT_INACTIVE);
    k_work_init_delayable(&blink_work, blink_handler);

    s_profile   = zmk_ble_active_profile_index();
    s_connected = zmk_ble_active_profile_is_connected();
    s_pairing   = !zmk_ble_active_profile_is_connected()
                  && zmk_ble_active_profile_is_open();
    refresh();
    return 0;
}

SYS_INIT(led_status_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
