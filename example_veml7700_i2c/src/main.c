#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/veml7700.h> //required for enums
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/logging/log.h>

// Defines for Lux sensor
#define I2C0_NODE DT_NODELABEL(veml7700)

static const struct device *get_veml7700_device(void)
{  
    /* You can use either of these to get the device */
    const struct device *dev = DEVICE_DT_GET(DT_INST(0,vishay_veml7700));
    //const struct device *const dev = DEVICE_DT_GET_ANY(st_lsm6dsl);

    //veml7700_init(dev);
    if (dev == NULL) {
        /* No such node, or the node does not have status "okay". */
        printk("\nError: no device found.\n");
        return NULL;
    }

    if (!device_is_ready(dev)) {
        printk("\nError: Device \"%s\" is not ready; "
               "check the driver initialization logs for errors.\n",
               dev->name);
        return NULL;
    }

    printk("Found device \"%s\", getting sensor data\n", dev->name);
    return dev;
}

int main(void)
{
	//retrieve the API-specific device structure
	const struct device *dev = get_veml7700_device();

	// define sensor_value struct
	static struct sensor_value lux;    
	static struct sensor_value gain;
	static struct sensor_value it;
	//static struct sensor_value gain2;
	//static struct sensor_value it2;
	gain.val1 = VEML7700_ALS_GAIN_1_8; //gain of 1/8
	it.val1 = VEML7700_ALS_IT_25; //25 ms integration time

	//configure the sensor registers
	sensor_attr_set(dev, SENSOR_CHAN_LIGHT, SENSOR_ATTR_VEML7700_GAIN, &gain);
	printk("The gain attribute has been set to %d.\n", gain.val1);
	//sensor_attr_get(dev, SENSOR_CHAN_LIGHT, SENSOR_ATTR_VEML7700_GAIN, &gain2);
	//printk("The gain value returned is %d\n", gain2.val1);
	sensor_attr_set(dev, SENSOR_CHAN_LIGHT, SENSOR_ATTR_VEML7700_ITIME, &it);
	printk("The integration time has been set to %d\n", it.val1);
	//sensor_attr_get(dev, SENSOR_CHAN_LIGHT, SENSOR_ATTR_VEML7700_ITIME, &it2);
	//printk("The integration time returned is %d\n", it2.val1);

	// while loop for data logging
	while (1){
		sensor_sample_fetch_chan(dev, SENSOR_CHAN_LIGHT);
        sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &lux);
        printk("lux: %d.%03d\n",lux.val1, lux.val2);
		k_msleep(2000);
	}

	return 0;
}