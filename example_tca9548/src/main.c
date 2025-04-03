#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor/veml7700.h> //required for enums

int main(void)
{
	// Section for TCA9548a
	const struct device *veml_0 = DEVICE_DT_GET(DT_NODELABEL(veml7700_0));
	const struct device *veml_1 = DEVICE_DT_GET(DT_NODELABEL(veml7700_1));

	static struct sensor_value lux_0;
	static struct sensor_value lux_1;

	// while loop for data logging
	while (1){
		printk("Start of while loop: \n");
		
		sensor_sample_fetch_chan(veml_0, SENSOR_CHAN_LIGHT);
        sensor_channel_get(veml_0, SENSOR_CHAN_LIGHT, &lux_0);
        printk("lux_0: %d.%03d\n",lux_0.val1, lux_0.val2);

		sensor_sample_fetch_chan(veml_1, SENSOR_CHAN_LIGHT);
        sensor_channel_get(veml_1, SENSOR_CHAN_LIGHT, &lux_1);
        printk("lux_1: %d.%03d\n",lux_1.val1, lux_1.val2);

		k_msleep(2000);
	}

	return 0;
}