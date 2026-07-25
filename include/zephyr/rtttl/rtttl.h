
/**
 * @file
 * @brief Public API for RTTTL API
 */

#ifndef ZEPHYR_INCLUDE_RTTTL_RTTTL_H_
#define ZEPHYR_INCLUDE_RTTTL_RTTTL_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Ring Tone Text Transfer Language API
 * @defgroup retention_api RTTTL API
 * @since 4.4
 * @version 0.1.0
 * @ingroup subsys
 * @{
 */

enum rtttl_duration {
	RTTTL_DURATION_WHOLE = 1,
	RTTTL_DURATION_HALF = 2,
	RTTTL_DURATION_QUARTER = 4,
	RTTTL_DURATION_EIGHTH = 8,
	RTTTL_DURATION_SIXTEENTH = 16,
	RTTTL_DURATION_THIRTY_SECOND = 32,
};

enum rtttl_pitch {
	RTTTL_PITCH_PAUSE = 0x70,
	RTTTL_PITCH_A = 0x61,
	RTTTL_PITCH_B = 0x62,
	RTTTL_PITCH_C = 0x63,
	RTTTL_PITCH_D = 0x64,
	RTTTL_PITCH_E = 0x65,
	RTTTL_PITCH_F = 0x66,
	RTTTL_PITCH_G = 0x67,
};

struct rtttl_note {
	enum rtttl_duration duration;
	enum rtttl_pitch pitch;
	bool is_sharp;
	bool is_dotted;
	uint8_t octave;
};

struct rtttl_settings {
	enum rtttl_duration duration;
	uint8_t octave;
	uint16_t tempo;
};

struct rtttl_data {
	char *name;
	struct rtttl_note *notes;
};

int rtttl_play(const struct device *buzzer, const char *data, size_t data_len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RTTTL_RTTTL_H_ */
