#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/rtc/maxim_ds3231.h>

static const struct device *get_rtc_device(void)
{  
    const struct device *dev = DEVICE_DT_GET(DT_INST(0,maxim_ds3231));

    if (dev == NULL) {
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
	const struct device *rtc_dev = get_rtc_device();
	//uint32_t syncclock_Hz = maxim_ds3231_syncclock_frequency(rtc_dev);
	printk("The device has initialized: \"%s\"\n", rtc_dev->name);

	while (1){
		printk("In while loop...\n");
		k_msleep(2000);
	}

	return 0;
}