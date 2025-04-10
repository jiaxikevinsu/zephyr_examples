#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/rtc/maxim_ds3231.h>
#include <zephyr/drivers/i2c.h>

/* Format times as: YYYY-MM-DD HH:MM:SS DOW DOY */
static char *format_time(struct tm *time){ 
	static char buf[64]; //buffer to store formatted time
	char *bp = buf; //pointer to the current write position
	char *const bpe = bp + sizeof(buf); //pointer to the end of the buffer
	//struct tm *tp = time;
	bp += strftime(bp, bpe - bp, "%Y-%m-%d %H:%M:%S", time); //format date and time as a string, adjusts the write location of bp by N*sizeof(char)
	bp += strftime(bp, bpe - bp, " %a", time);
	return buf;
}

void set_rtc_time_interrupt(const struct device *ds3231, time_t time){
	// establish a syncpoint
	uint32_t syncclock = maxim_ds3231_read_syncclock(ds3231); //this is using k_uptime_get_32() to get uptime from boot in ms
	printk("The time has been set as %ld\n", (long)time);

	struct maxim_ds3231_syncpoint sp = {
		.rtc = {
			.tv_sec = time,
			.tv_nsec = 0, //to not ignore sub second, use (uint64_t)NSEC_PER_SEC * syncclock / syncclock_Hz,
		},
		.syncclock = syncclock,
	};

	// Set the RTC time with syncpoint
	struct sys_notify notify;
	int rc = maxim_ds3231_set(ds3231, &sp, &notify); //won't work without some sort of interrupt setup
	k_msleep(200);
	// Check for success or failure
	if (rc == 0) {
	    printk("RTC time set successfully using syncpoint!\n");
	} else {
	    printk("Error setting RTC time: %d\n", rc);
	}
}

void set_rtc_time(const struct device *i2c_dev, struct tm time){
	uint8_t buf_sec = 0, buf_min = 0, buf_hour = 0, buf_day = 0, buf_date = 0, buf_month = 0, buf_year = 0;
	printk("The values passed in to set are: sec: %d, min: %d, hour: %d, wday: %d, mday: %d, mon: %d, year: %d\n", time.tm_sec, time.tm_min, time.tm_hour, time.tm_wday, time.tm_mday, time.tm_mon, time.tm_year);
	buf_sec = ((time.tm_sec / 10) << 4) | (time.tm_sec % 10);
	buf_min = ((time.tm_min / 10) << 4) | (time.tm_min % 10);
	buf_hour = ((time.tm_hour / 10) << 4) | (time.tm_hour % 10);
	buf_day = time.tm_wday;
	buf_date = ((time.tm_mday / 10) << 4) | (time.tm_mday % 10);
	buf_month = ((time.tm_mon / 10) << 4) | (time.tm_mon % 10);
	buf_year = ((time.tm_year / 10) << 4) | (time.tm_year % 10);

	int ret = i2c_reg_write_byte(i2c_dev, 0x68, 0x00, buf_sec);
	ret = i2c_reg_write_byte(i2c_dev, 0x68, 0x01, buf_min);
	ret = i2c_reg_write_byte(i2c_dev, 0x68, 0x02, buf_hour);
	ret = i2c_reg_write_byte(i2c_dev, 0x68, 0x03, buf_day);
	ret = i2c_reg_write_byte(i2c_dev, 0x68, 0x04, buf_date);
	ret = i2c_reg_write_byte(i2c_dev, 0x68, 0x05, buf_month);
	ret = i2c_reg_write_byte(i2c_dev, 0x68, 0x06, buf_year);

	printk("The following time has been set: sec: %hhu, min: %hhu, hour: %hhu, wday: %hhu, mday: %hhu, mon: %hhu, year: %hhu\n", buf_sec, buf_min, buf_hour, buf_day, buf_date, buf_month, buf_year);

}

struct tm get_rtc_time(const struct device *i2c_dev){
	// Directly reads and interprets the date time registers of the DS3231, outputs a tm struct
	uint8_t buf_sec = 0, buf_min = 0, buf_hour = 0, buf_day = 0, buf_date = 0, buf_month = 0, buf_year = 0;
	struct tm current_time = {0};
	int ret = i2c_reg_read_byte(i2c_dev, 0x68, 0x00, &buf_sec);
	ret = i2c_reg_read_byte(i2c_dev, 0x68, 0x01, &buf_min);
	ret = i2c_reg_read_byte(i2c_dev, 0x68, 0x02, &buf_hour);
	ret = i2c_reg_read_byte(i2c_dev, 0x68, 0x03, &buf_day);
	ret = i2c_reg_read_byte(i2c_dev, 0x68, 0x04, &buf_date);
	ret = i2c_reg_read_byte(i2c_dev, 0x68, 0x05, &buf_month);
	ret = i2c_reg_read_byte(i2c_dev, 0x68, 0x06, &buf_year);
	// if (ret != 0){
	// 	printk("There was an error reading from the register");
	// }
	current_time.tm_sec = (((buf_sec & 112) >> 4) * 10) + (buf_sec & 15);//((buf_sec / 10) << 4) | (buf_sec % 10);
	current_time.tm_min = (((buf_min & 112) >> 4) * 10) + (buf_min & 15);

	int hour_calc = (((buf_hour & 16) >> 4) * 10) + (buf_hour & 15);
	if (((buf_hour & 64) >> 6) == 1){ //this means 12 hour format
		if (((buf_hour & 32) >> 5) == 1){ //this means PM
			//handle edge cases:
			if (hour_calc == 12){
				current_time.tm_hour = 12; //convert to 24 hour time
			}
		} else { //this means AM time
			if (hour_calc == 12){
				current_time.tm_hour = 0; //convert to 24 hour time
			}
		}
	} else {
		current_time.tm_hour = hour_calc;  //24 hour time
	}
	current_time.tm_wday = buf_day;
	current_time.tm_mday = (((buf_date & 48) >> 4) * 10) + (buf_date & 15);
	current_time.tm_mon = (((buf_month & 16) >> 4) * 10) + (buf_month & 15); //ignore bit 7
	current_time.tm_year = (((buf_year & 240) >> 4) * 10) + (buf_year & 15);
	printk("The following buffers contains: sec: %hhu, min: %hhu, hour: %hhu, wday: %hhu, mday: %hhu, mon: %hhu, year: %hhu\n", buf_sec, buf_min, buf_hour, buf_day, buf_date, buf_month, buf_year);
	printk("The time formatted is: sec: %d, min: %d, hour: %d, wday: %d, mday: %d, mon: %d, year: %d\n", current_time.tm_sec, current_time.tm_min, current_time.tm_hour, current_time.tm_wday, current_time.tm_mday, current_time.tm_mon, current_time.tm_year);
	return current_time;
}

int main(void){
	const struct device *const ds3231 = DEVICE_DT_GET_ONE(maxim_ds3231);
	const struct device *const i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

	struct tm tm_set_time = {0};
	tm_set_time.tm_sec = 0;		//seconds,  range 0 to 59
	tm_set_time.tm_min = 40;		//minutes, range 0 to 59
	tm_set_time.tm_hour = 17;		//hours, range 0 to 23

	tm_set_time.tm_mday = 10;		//day of the month, range 1 to 31
	tm_set_time.tm_mon = 3;		//month, range 0 to 11
	tm_set_time.tm_year = 2025 - 1900;	//the number of years since 1900

	tm_set_time.tm_wday = 4;		//day of the week, range 0 to 6
	tm_set_time.tm_yday = 100;		//day in the year, range 0 to 365
	// tm_set_time.tm_isdst = 1;		//daylight saving time - positive when in effect, 0 when not, negative when unknown

	//time_t time_s = mktime(&tm_set_time); 
	set_rtc_time(i2c_dev, tm_set_time);

	while (1){
		struct tm tm_curr_time = get_rtc_time(i2c_dev);
		printk("The current time is: %s\n", format_time(&tm_curr_time));
		
		k_msleep(2000);
		printk("after sleep...\n");
	}
	return 0;
}