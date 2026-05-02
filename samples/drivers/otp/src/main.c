
#include <zephyr/device.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#ifndef BUILDTIME
#define BUILDTIME ""
#endif /* BUILDTIME */
#ifndef BUILDDATE
#define BUILDDATE ""
#endif /* BUILDDATE */


static char otp_data[] = "written at " BUILDTIME " on " BUILDDATE;

int main()
{
	const struct device *otp_dev = DEVICE_DT_GET(DT_ALIAS(otp));
	int ret;

	if (!device_is_ready(otp_dev)) {
		LOG_ERR("otp device not ready");
		return -ENODEV;
	}

	uint8_t buf[32] = {};
	ret = otp_read(otp_dev, 0, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("failed to read otp: %s", strerror(-ret));
		return ret;
	}
	LOG_HEXDUMP_INF(buf, sizeof(buf), "otp: ");

	LOG_HEXDUMP_INF(otp_data, sizeof(otp_data), "programming otp: ");
	ret = otp_program(otp_dev, 0, otp_data, sizeof(otp_data));
	if (ret < 0) {
		LOG_ERR("failed to program otp: %s", strerror(-ret));
		return ret;
	}

	ret = otp_read(otp_dev, 0, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("failed to read otp: %s", strerror(-ret));
		return ret;
	}
	LOG_HEXDUMP_INF(buf, sizeof(buf), "otp: ");

	return 0;
}
