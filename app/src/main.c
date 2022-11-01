#include <stdio.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>

#include <app_event_manager.h>

#define MODULE main
#include <caf/events/module_state_event.h>
#include <caf/events/button_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);


#define LED0_NODE DT_ALIAS(myled0alias)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);


#ifdef CONFIG_ISM330DHCX_TRIGGER
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
#endif

static void ism330dhcx_config(const struct device *ism330dhcx)
{
	struct sensor_value odr_attr, fs_attr;

	/* set ISM330DHCX sampling frequency to 416 Hz */
	odr_attr.val1 = 416;
	odr_attr.val2 = 0;

	if (sensor_attr_set(ism330dhcx, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		LOG_ERR("Cannot set sampling frequency for ISM330DHCX accel\n");
		return;
	}

	sensor_g_to_ms2(16, &fs_attr);

	if (sensor_attr_set(ism330dhcx, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		LOG_ERR("Cannot set sampling frequency for ISM330DHCX accel\n");
		return;
	}

	/* set ISM330DHCX gyro sampling frequency to 208 Hz */
	odr_attr.val1 = 208;
	odr_attr.val2 = 0;

	if (sensor_attr_set(ism330dhcx, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		LOG_ERR("Cannot set sampling frequency for ISM330DHCX gyro\n");
		return;
	}

	sensor_degrees_to_rad(250, &fs_attr);

	if (sensor_attr_set(ism330dhcx, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		LOG_ERR("Cannot set fs for ISM330DHCX gyro\n");
		return;
	}

#ifdef CONFIG_ISM330DHCX_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(ism330dhcx, &trig, ism330dhcx_acc_trigger_handler);

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_GYRO_XYZ;
	sensor_trigger_set(ism330dhcx, &trig, ism330dhcx_gyr_trigger_handler);
#endif
}


void main(void)
{
	int ret;
	
	const struct device *const cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	const struct device *const ism330dhcx = DEVICE_DT_GET_ONE(st_ism330dhcx);

	struct sensor_value accel1[3];
	struct sensor_value gyro[3];
	int cnt;

	if (app_event_manager_init()) {
		LOG_ERR("Event manager not initialized");
	} else {
		module_set_state(MODULE_STATE_READY);
	}

	if (!device_is_ready(led.port)) {
		return;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return;
	}

	if (!device_is_ready(ism330dhcx)) {
		printk("%s: device not ready.\n", ism330dhcx->name);
		return;
	}

	ism330dhcx_config(ism330dhcx);

	while (1) {
		sensor_channel_get(ism330dhcx, SENSOR_CHAN_ACCEL_XYZ, accel1);
		sensor_channel_get(ism330dhcx, SENSOR_CHAN_GYRO_XYZ, gyro);

		/* Erase previous */
		printf("\0033\014");

		printf("ISM330DHCX: Accel (m.s-2): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&accel1[0]),
			sensor_value_to_double(&accel1[1]),
			sensor_value_to_double(&accel1[2]));

		printf("ISM330DHCX: GYro (dps): x: %.3f, y: %.3f, z: %.3f\n",
			sensor_value_to_double(&gyro[0]),
			sensor_value_to_double(&gyro[1]),
			sensor_value_to_double(&gyro[2]));

#ifdef CONFIG_ISM330DHCX_TRIGGER
		printk("%d:: ism330dhcx acc trig %d\n", cnt, ism330dhcx_acc_trig_cnt);
		printk("%d:: ism330dhcx gyr trig %d\n", cnt, ism330dhcx_gyr_trig_cnt);
#endif

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
	int ret;
	const struct button_event *evt;

	if (is_button_event(eh)) {
		evt = cast_button_event(eh);

		if (evt->pressed) {
			LOG_INF("Pin Toggle");
			ret = gpio_pin_toggle_dt(&led);
		}
	}

	return true;
}

APP_EVENT_LISTENER(MODULE, event_handler);
APP_EVENT_SUBSCRIBE(MODULE, button_event);
