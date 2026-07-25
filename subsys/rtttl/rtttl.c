
#include <stdlib.h>
#include <zephyr/drivers/buzzer.h>

#include <zephyr/rtttl/rtttl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rtttl, CONFIG_RTTTL_LOG_LEVEL);

#define MIN_OCTAVE 5
#define MAX_OCTAVE 8

#define VALID_DURATION(val)                                                                        \
	((val) == 1 || (val) == 2 || (val) == 4 || (val) == 8 || (val) == 16 || (val) == 32)
#define VALID_OCTAVE(val) IN_RANGE((val), MIN_OCTAVE, MAX_OCTAVE)
#define VALID_TEMPO(val)  IN_RANGE((val), 5, 900)

enum freq {
	FREQ_C5 = 523,
	FREQ_C5_SHARP = 554,
	FREQ_D5 = 587,
	FREQ_D5_SHARP = 622,
	FREQ_E5 = 659,
	FREQ_F5 = 698,
	FREQ_F5_SHARP = 740,
	FREQ_G5 = 784,
	FREQ_G5_SHARP = 831,
	FREQ_A5 = 880,
	FREQ_A5_SHARP = 932,
	FREQ_B5 = 988,
};

static const char section_delim = ':';
static const char note_delim = ',';

static struct rtttl_settings g_settings = {0};

int parse_settings(const char *data, char **end, size_t data_len, struct rtttl_settings *settings)
{
	enum {
		NONE,
		DURATION,
		OCTAVE,
		TEMPO,
	} parsing;

	char *ptr = (char *)data;
	while (*ptr != section_delim) {
		if (*ptr == 0 || data_len == 0) {
			LOG_ERR("invalid ring tone settings");
			return -EINVAL;
		}
		switch (*ptr) {
		case 'd':
			parsing = DURATION;
			break;
		case 'o':
			parsing = OCTAVE;
			break;
		case 'b':
			parsing = TEMPO;
			break;
		case '=': {
			ptr++;
			long value = strtol(ptr, &ptr, 10);
			if (value == 0) {
				LOG_ERR("invalid ring tone setting for %d", parsing);
				return -EINVAL;
			}
			switch (parsing) {
			case DURATION:
				if (!VALID_DURATION(value)) {
					LOG_ERR("invalid ring tone setting value for d: %ld",
						value);
					return -EINVAL;
				}
				settings->duration = value;
				break;
			case OCTAVE:
				if (!VALID_OCTAVE(value)) {
					LOG_ERR("invalid ring tone setting value for o: %ld",
						value);
					return -EINVAL;
				}
				settings->octave = value;
				break;
			case TEMPO:
				if (!VALID_TEMPO(value)) {
					LOG_ERR("invalid ring tone setting value for b: %ld",
						value);
					return -EINVAL;
				}
				settings->tempo = value;
				break;
			case NONE:
				return -EINVAL;
			}
			break;
		}
		}
		ptr++;
		data_len--;
	}
	if (end) {
		*end = ptr;
	}
	return ptr - data;
}

uint32_t note_to_freq(struct rtttl_note note)
{
	uint32_t freq;
	switch (note.pitch) {
	case RTTTL_PITCH_A:
		freq = note.is_sharp ? FREQ_A5_SHARP : FREQ_A5;
	case RTTTL_PITCH_B:
		freq = FREQ_B5;
	case RTTTL_PITCH_C:
		freq = note.is_sharp ? FREQ_C5_SHARP : FREQ_C5;
	case RTTTL_PITCH_D:
		freq = note.is_sharp ? FREQ_D5_SHARP : FREQ_D5;
	case RTTTL_PITCH_E:
		freq = FREQ_E5;
	case RTTTL_PITCH_F:
		freq = note.is_sharp ? FREQ_F5_SHARP : FREQ_F5;
	case RTTTL_PITCH_G:
		freq = note.is_sharp ? FREQ_G5_SHARP : FREQ_G5;
	case RTTTL_PITCH_PAUSE:
		return 0;
	}

	freq *= ((note.octave - MIN_OCTAVE) * 2);
}

uint32_t note_to_ms(struct rtttl_note note)
{
	uint32_t tempo = g_settings.tempo;
	uint32_t beat_per_ms = 60000;
	switch (note.duration) {
	case RTTTL_DURATION_WHOLE:
		beat_per_ms *= 4;
		break;
	case RTTTL_DURATION_HALF:
		beat_per_ms *= 2;
		break;
	case RTTTL_DURATION_QUARTER:
		break;
	case RTTTL_DURATION_EIGHTH:
		tempo *= 2;
		break;
	case RTTTL_DURATION_SIXTEENTH:
		tempo *= 4;
		break;
	case RTTTL_DURATION_THIRTY_SECOND:
		tempo *= 8;
		break;
	}
	return beat_per_ms / tempo;
}

int play_note(const struct device *buzzer, struct rtttl_note note)
{

	uint32_t freq = note_to_freq(note);
	uint32_t ms = note_to_ms(note);
	if (freq != 0) {
		buzzer_tone(buzzer, freq, ms);
	}
	k_sleep(K_MSEC(ms));
	return 0;
}

int rtttl_play(const struct device *buzzer, const char *data, size_t data_len)
{
	__ASSERT(data, "rtttl started with NULL data");
	if (!device_is_ready(buzzer)) {
		LOG_ERR("Buzzer device %s not ready\n", buzzer->name);
		return 0;
	}

	char *ptr = (char *)data;
	for (; *ptr != section_delim; ptr++, data_len--) {
		if (*ptr == 0 || data_len == 0) {
			LOG_ERR("invalid ring tone name");
			return -EINVAL;
		}
	}
	LOG_HEXDUMP_INF(data, ptr - data, "playing:");
	ptr++; /* skip delim */
	data_len--;

	int ret = parse_settings(ptr, &ptr, data_len, &g_settings);
	if (ret < 0) {
		return ret;
	}
	data_len += ret;

	ptr++; /* skip delim */
	data_len--;

	// play notes
	for (; *ptr != section_delim; ptr++, data_len--) {
		if (*ptr == 0 || data_len == 0) {
			return 0;
		}

		if (*ptr == 0x20) {
			continue;
		}

		struct rtttl_note note = {0};

		note.duration = strtol(ptr, &ptr, 10);
		if (!VALID_DURATION(note.duration)) {
			note.duration = g_settings.duration;
		}

		note.pitch = *ptr++;
		data_len--;
		if (*ptr == 0 || data_len == 0 || *ptr == note_delim) {
			play_note(buzzer, note);
			continue;
		}

		if (*ptr == '#') {
			note.is_sharp = true;
			ptr++;
			data_len--;
		}
		if (*ptr == 0 || data_len == 0 || *ptr == note_delim) {
			play_note(buzzer, note);
			continue;
		}

		uint8_t octave = strtol(ptr, &ptr, 10);
		if (!VALID_OCTAVE(octave)) {
			octave = g_settings.octave;
		}
		if (*ptr == 0 || data_len == 0 || *ptr == note_delim) {
			play_note(buzzer, note);
			continue;
		}

		if (*ptr == '.') {
			note.is_dotted = true;
			ptr++;
			data_len--;
		}

		play_note(buzzer, note);
	}
	return 0;
}
