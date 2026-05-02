

#include "syscalls/device.h"
#define DT_DRV_COMPAT micronix_mx25_otp

#include "zephyr/drivers/flash.h"
#include "zephyr/drivers/spi.h"
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mx25_otp, CONFIG_OTP_LOG_LEVEL);

struct otp_mx25_config {
	struct spi_dt_spec spi;
	const struct device *parent_dev;
};

struct otp_mx25_data {
};

#define MX25_SPI_CONFIG (SPI_WORD_SET(8))

#define MX25_ENSO   (0xB1U) /* Enter Secure OTP */
#define MX25_EXSO   (0xC1U) /* Exit Secure OTP */
#define MX25_RDSCUR (0x2BU) /* Read Secure Register */
#define MX25_WRSCUR (0x2FU) /* Write Secure Register */
#define MX25_WREN   (0x06U) /* Write Enable */

#define MX25_SECTOR_SIZE (0x1000U)

static int mx25_send_command(const struct spi_dt_spec *spi, uint8_t command)
{
	int ret;

	uint8_t tx_buf[] = {
		command,
	};
	const struct spi_buf tx_bufs[] = {{
		.buf = tx_buf,
		.len = sizeof(tx_buf),
	}};
	struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs),
	};

	uint8_t rx_buf[32] = {};
	const struct spi_buf rx_bufs[] = {
		{
			.buf = NULL,
			.len = 1,
		},
		{
			.buf = rx_buf,
			.len = sizeof(rx_buf),
		},
	};
	struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs),
	};
	ret = spi_transceive_dt(spi, &tx, &rx);
	if (ret < 0) {
		return ret;
	}
	LOG_HEXDUMP_INF(rx_buf, sizeof(rx_buf), "rx: ");

	return 0;
}

static int mx25_enter_otp(const struct spi_dt_spec *spi)
{
	return mx25_send_command(spi, MX25_ENSO);
}

static int mx25_exit_otp(const struct spi_dt_spec *spi)
{
	return mx25_send_command(spi, MX25_EXSO);
}

static int mx25_lock_otp(const struct spi_dt_spec *spi)
{
	int ret = mx25_send_command(spi, MX25_WREN);
	if (ret < 0) {
		return ret;
	}

	return mx25_send_command(spi, MX25_WRSCUR);
}

static int otp_mx25_init(const struct device *dev)
{
	const struct otp_mx25_config *config = dev->config;
	if (!device_is_ready(config->parent_dev)) {
		return -ENODEV;
	}
	return 0;
}

static int otp_mx25_program(const struct device *dev, off_t offset, const void *data, size_t len)
{
	const struct otp_mx25_config *config = dev->config;
	int ret;

	mx25_send_command(&config->spi, MX25_RDSCUR);

	ret = mx25_enter_otp(&config->spi);
	if (ret < 0) {
		return ret;
	}

	ret = flash_erase(config->parent_dev, offset,
			  MX25_SECTOR_SIZE * DIV_ROUND_UP(len, MX25_SECTOR_SIZE));
	if (ret < 0) {
		goto exit;
	}

	ret = flash_write(config->parent_dev, offset, data, len);

exit:
	mx25_exit_otp(&config->spi);
	if (ret < 0) {
		return ret;
	}

	return mx25_lock_otp(&config->spi);
}

static int otp_mx25_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct otp_mx25_config *config = dev->config;
	int ret;

	ret = mx25_enter_otp(&config->spi);
	if (ret < 0) {
		return ret;
	}

	ret = flash_read(config->parent_dev, offset, data, len);
	mx25_exit_otp(&config->spi);
	return ret;
}

static DEVICE_API(otp, otp_mx25_api) = {
#if defined(CONFIG_OTP_PROGRAM)
	.program = otp_mx25_program,
#endif
	.read = otp_mx25_read,
};

#define OTP_MX25_INIT(inst)                                                                        \
	static const struct otp_mx25_config otp_mx25_config_##inst = {                             \
		.parent_dev = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),                         \
		.spi = SPI_DT_SPEC_GET(DT_PARENT(DT_DRV_INST(inst)), MX25_SPI_CONFIG),             \
	};                                                                                         \
	static struct otp_mx25_data otp_mx25_data_##inst = {};                                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, otp_mx25_init, NULL, &otp_mx25_data_##inst,                    \
			      &otp_mx25_config_##inst, POST_KERNEL, CONFIG_OTP_INIT_PRIORITY,      \
			      &otp_mx25_api);

DT_INST_FOREACH_STATUS_OKAY(OTP_MX25_INIT)
