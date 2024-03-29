#include <stdio.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>

#include <app_event_manager.h>

#define MODULE main
#include <caf/events/module_state_event.h>
#include <caf/events/button_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);


#define STRIP_NUM_LEDS 20

#define DELAY_TIME K_MSEC(40)

static const struct led_rgb colors[] = {
	{ .r = 0xff, .g = 0x00, .b = 0x00, }, /* red */
	{ .r = 0x00, .g = 0xff, .b = 0x00, }, /* green */
	{ .r = 0x00, .g = 0x00, .b = 0xff, }, /* blue */
};

static const struct led_rgb black = {
	.r = 0x00,
	.g = 0x00,
	.b = 0x00,
};

struct led_rgb strip0_colors[STRIP_NUM_LEDS];
struct led_rgb strip1_colors[STRIP_NUM_LEDS];
struct led_rgb strip2_colors[STRIP_NUM_LEDS];

static int ism330dhcx_acc_trig_cnt;
static int ism330dhcx_gyr_trig_cnt;

static void ism330dhcx_acc_trigger_handler(const struct device *dev,
					   const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	ism330dhcx_acc_trig_cnt++;
}

static void ism330dhcx_gyr_trigger_handler(const struct device *dev,
					   const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_GYRO_XYZ);
	ism330dhcx_gyr_trig_cnt++;
}

static void ism330dhcx_config(const struct device *ism330dhcx)
{
	struct sensor_value odr_attr, fs_attr;

	/* set ISM330DHCX sampling frequency to 416 Hz */
	odr_attr.val1 = 416;
	odr_attr.val2 = 0;

	if (sensor_attr_set(ism330dhcx, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		LOG_ERR("Cannot set sampling frequency for ISM330DHCX accel");
		return;
	}

	sensor_g_to_ms2(16, &fs_attr);

	if (sensor_attr_set(ism330dhcx, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		LOG_ERR("Cannot set sampling frequency for ISM330DHCX accel");
		return;
	}

	/* set ISM330DHCX gyro sampling frequency to 208 Hz */
	odr_attr.val1 = 208;
	odr_attr.val2 = 0;

	if (sensor_attr_set(ism330dhcx, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		LOG_ERR("Cannot set sampling frequency for ISM330DHCX gyro");
		return;
	}

	sensor_degrees_to_rad(250, &fs_attr);

	if (sensor_attr_set(ism330dhcx, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		LOG_ERR("Cannot set fs for ISM330DHCX gyro");
		return;
	}


	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(ism330dhcx, &trig, ism330dhcx_acc_trigger_handler);

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_GYRO_XYZ;
	sensor_trigger_set(ism330dhcx, &trig, ism330dhcx_gyr_trigger_handler);
}


void main(void)
{
	const struct device *const cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	const struct device *const ism330dhcx = DEVICE_DT_GET_ONE(st_ism330dhcx);

	const struct device *strip0 = DEVICE_DT_GET(DT_ALIAS(led_strip_0));
	const struct device *strip1 = DEVICE_DT_GET(DT_NODELABEL(led_strip_1));
	const struct device *strip2 = DEVICE_DT_GET(DT_NODELABEL(led_strip_2));

	struct sensor_value accel1[3];
	struct sensor_value gyro[3];
	int cnt = 0;

	if (app_event_manager_init()) {
		LOG_ERR("Event manager not initialized");
	} else {
		module_set_state(MODULE_STATE_READY);
	}

	if (!device_is_ready(ism330dhcx)) {
		LOG_ERR("%s: device not ready.", ism330dhcx->name);
		return;
	}

	ism330dhcx_config(ism330dhcx);

	if (!strip0) {
		LOG_ERR("LED strip device not found");
		return;
	} else if (!device_is_ready(strip0)) {
		LOG_ERR("LED strip device %s is not ready", strip0->name);
		return;
	} else {
		LOG_INF("Found LED strip device %s", strip0->name);
	}

	if (!strip1) {
		LOG_ERR("LED strip device not found");
		return;
	} else if (!device_is_ready(strip1)) {
		LOG_ERR("LED strip device %s is not ready", strip1->name);
		return;
	} else {
		LOG_INF("Found LED strip device %s", strip1->name);
	}

	if (!strip2) {
		LOG_ERR("LED strip device not found");
		return;
	} else if (!device_is_ready(strip2)) {
		LOG_ERR("LED strip device %s is not ready", strip2->name);
		return;
	} else {
		LOG_INF("Found LED strip device %s", strip2->name);
	}


	LOG_INF("Displaying pattern on strip");
	while (1) {
		memcpy(&strip0_colors[0], &colors[0], sizeof(strip0_colors[0]));
		memcpy(&strip1_colors[1], &colors[0], sizeof(strip1_colors[1]));
		memcpy(&strip2_colors[2], &colors[0], sizeof(strip2_colors[2]));

		led_strip_update_rgb(strip0, strip0_colors, STRIP_NUM_LEDS);
		led_strip_update_rgb(strip1, strip1_colors, STRIP_NUM_LEDS);
		led_strip_update_rgb(strip2, strip2_colors, STRIP_NUM_LEDS);
		// k_sleep(DELAY_TIME);

		sensor_channel_get(ism330dhcx, SENSOR_CHAN_ACCEL_XYZ, accel1);
		sensor_channel_get(ism330dhcx, SENSOR_CHAN_GYRO_XYZ, gyro);

		/* Erase previous */
		// LOG_INF("\0033\014");

		LOG_INF("ISM330DHCX: Accel (m.s-2): x: %.3f, y: %.3f, z: %.3f",
			sensor_value_to_double(&accel1[0]),
			sensor_value_to_double(&accel1[1]),
			sensor_value_to_double(&accel1[2]));

		LOG_INF("ISM330DHCX: GYro (dps): x: %.3f, y: %.3f, z: %.3f",
			sensor_value_to_double(&gyro[0]),
			sensor_value_to_double(&gyro[1]),
			sensor_value_to_double(&gyro[2]));

		LOG_INF("%d:: ism330dhcx acc trig %d", cnt, ism330dhcx_acc_trig_cnt);
		LOG_INF("%d:: ism330dhcx gyr trig %d", cnt, ism330dhcx_gyr_trig_cnt);

		cnt++;
		k_sleep(K_MSEC(2000));
	}

	LOG_INF("****************************************");
	LOG_INF("MAIN DONE");
	LOG_INF("****************************************");

	k_sleep(K_SECONDS(3));
	pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);

	LOG_INF("PM_DEVICE_ACTION_SUSPEND");
}

static bool event_handler(const struct app_event_header *eh)
{
	const struct button_event *evt;

	if (is_button_event(eh)) {
		evt = cast_button_event(eh);

		if (evt->pressed) {
			LOG_INF("PRESSED");
		}
	}

	return true;
}

APP_EVENT_LISTENER(MODULE, event_handler);
APP_EVENT_SUBSCRIBE(MODULE, button_event);
