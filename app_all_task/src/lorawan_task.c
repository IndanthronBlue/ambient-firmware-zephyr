/*
 * LoRaWAN Uplink
 * Periodically joins the network and sends telemetry frames
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/byteorder.h>
#include <errno.h>
#include <string.h>

LOG_MODULE_REGISTER(lorawan_task, LOG_LEVEL_INF);

#include "tasks.h"

/* OTAA credentials (MSB first) */
static uint8_t join_eui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t dev_eui[]  = { 0xB2, 0x21, 0x56, 0xAD, 0xAD, 0x8A, 0x4F, 0xF2 };
//603f3636a03926abe7521abe84049be2
static uint8_t app_key[]  = { 0x60, 0x3F, 0x36, 0x36, 0xA0, 0x39, 0x26, 0xAB, 0xE7, 0x52, 0x1A, 0xBE, 0x84, 0x04, 0x9B, 0xE2 };

#define LORAWAN_RETRY_1MIN_MS		(1 * 60 * 1000)
#define LORAWAN_RETRY_5MIN_MS		(5 * 60 * 1000)
#define LORAWAN_RETRY_10MIN_MS		(10 * 60 * 1000)
#define LORAWAN_RETRY_30MIN_MS		(30 * 60 * 1000)
#define LORAWAN_APP_PORT		2U
#define TX_PAYLOAD_LENGTH               29U
#define LORAWAN_CONFIRMED_EVERY_N       500U
#define LORAWAN_CONFIRMED_FAIL_LIMIT    2U

static bool lorawan_stack_started;
static bool lorawan_session_joined;
static bool dev_eui_initialized;
static int64_t next_join_deadline;
static uint32_t init_fail_count;
static uint8_t confirmed_fail_streak;
static uint16_t last_dev_nonce;
static uint32_t devnonce_prng_state;

static struct lorawan_downlink_cb downlink_cb;

static bool lorawan_dev_eui_is_valid(const uint8_t eui[8])
{
	bool all_zero = true;
	bool all_ff = true;

	for (size_t i = 0; i < 8U; i++) {
		if (eui[i] != 0U) {
			all_zero = false;
		}
		if (eui[i] != 0xFFU) {
			all_ff = false;
		}
	}

	return !all_zero && !all_ff;
}

static void lorawan_log_dev_eui(const char *tag, const uint8_t eui[8])
{
	LOG_INF("%s DevEUI: %02X %02X %02X %02X %02X %02X %02X %02X",
		tag,
		eui[0], eui[1], eui[2], eui[3],
		eui[4], eui[5], eui[6], eui[7]);
}

static uint64_t lorawan_crc64_ecma(const uint8_t *data, size_t len)
{
	uint64_t crc = 0ULL;
	const uint64_t poly = 0x42F0E1EBA9EA3693ULL;

	for (size_t i = 0; i < len; i++) {
		crc ^= ((uint64_t)data[i] << 56);
		for (int bit = 0; bit < 8; bit++) {
			if (crc & 0x8000000000000000ULL) {
				crc = (crc << 1) ^ poly;
			} else {
				crc <<= 1;
			}
		}
	}

	return crc;
}

static int lorawan_generate_dev_eui_from_hwid(uint8_t out_eui[8])
{
	uint8_t hwid[32];
	ssize_t hwid_len = hwinfo_get_device_id(hwid, sizeof(hwid));
	if (hwid_len <= 0) {
		LOG_WRN("hwinfo_get_device_id failed (%d)", (int)hwid_len);
		return -ENODATA;
	}

	uint64_t derived = lorawan_crc64_ecma(hwid, (size_t)hwid_len);
	memcpy(out_eui, &derived, sizeof(derived));

	/* Locally administered bit set; keep deterministic per-device. */
	out_eui[0] |= 0x02U;

	if (!lorawan_dev_eui_is_valid(out_eui)) {
		return -EINVAL;
	}

	return 0;
}

static void lorawan_init_dev_eui_once(void)
{
	if (dev_eui_initialized) {
		(void)task_storage_persist_dev_eui(dev_eui);
		return;
	}

	if (task_storage_load_dev_eui(dev_eui) && lorawan_dev_eui_is_valid(dev_eui)) {
		lorawan_log_dev_eui("Loaded", dev_eui);
		dev_eui_initialized = true;
		return;
	}

	if (lorawan_generate_dev_eui_from_hwid(dev_eui) == 0) {
		lorawan_log_dev_eui("Generated", dev_eui);
		dev_eui_initialized = true;
		(void)task_storage_persist_dev_eui(dev_eui);
		return;
	}

	if (!lorawan_dev_eui_is_valid(dev_eui)) {
		/* Last-resort fallback keeps old hardcoded value if generation failed. */
		dev_eui[0] = 0xB2;
		dev_eui[1] = 0x21;
		dev_eui[2] = 0x56;
		dev_eui[3] = 0xAD;
		dev_eui[4] = 0xAD;
		dev_eui[5] = 0x8A;
		dev_eui[6] = 0x4F;
		dev_eui[7] = 0xF2;
	}
	lorawan_log_dev_eui("Fallback", dev_eui);
	dev_eui_initialized = true;
	(void)task_storage_persist_dev_eui(dev_eui);
}

static uint16_t lorawan_random_devnonce(void)
{
	if (devnonce_prng_state == 0U) {
		devnonce_prng_state = k_cycle_get_32() ^ (uint32_t)k_uptime_get_32() ^ 0xA5A5C3D2U;
	}

	devnonce_prng_state ^= devnonce_prng_state << 13;
	devnonce_prng_state ^= devnonce_prng_state >> 17;
	devnonce_prng_state ^= devnonce_prng_state << 5;

	return (uint16_t)devnonce_prng_state;
}

static void lorawan_invalidate_session(const char *reason)
{
	lorawan_session_joined = false;
	app_state.lorawan_joined = false;
	confirmed_fail_streak = 0U;
	next_join_deadline = k_uptime_get();
	LOG_WRN("LoRaWAN session invalidated: %s; will rejoin on next uplink event",
		reason);
}

static int64_t lorawan_backoff_ms(uint32_t fail_count)
{
	if (fail_count <= 5U) {
		return LORAWAN_RETRY_1MIN_MS;
	}
	if (fail_count == 10U) {
		return LORAWAN_RETRY_5MIN_MS;
	}
	return LORAWAN_RETRY_10MIN_MS;
}

static void lorawan_on_downlink(uint8_t port, uint8_t flags, int16_t rssi,
				       int8_t snr, uint8_t len,
				       const uint8_t *payload)
{
	ARG_UNUSED(flags);

	LOG_INF("Downlink on port %u, RSSI %d dBm, SNR %d dB, len %u",
		port, rssi, snr, len);
	if (payload && len > 0U) {
		LOG_HEXDUMP_INF(payload, len, "DL:");
	}
}

static int lorawan_select_region(void)
{
#if defined(CONFIG_LORAMAC_REGION_AS923)
	return lorawan_set_region(LORAWAN_REGION_AS923);
#elif defined(CONFIG_LORAMAC_REGION_AU915)
	return lorawan_set_region(LORAWAN_REGION_AU915);
#elif defined(CONFIG_LORAMAC_REGION_CN470)
	return lorawan_set_region(LORAWAN_REGION_CN470);
#elif defined(CONFIG_LORAMAC_REGION_CN779)
	return lorawan_set_region(LORAWAN_REGION_CN779);
#elif defined(CONFIG_LORAMAC_REGION_EU433)
	return lorawan_set_region(LORAWAN_REGION_EU433);
#elif defined(CONFIG_LORAMAC_REGION_EU868)
	return lorawan_set_region(LORAWAN_REGION_EU868);
#elif defined(CONFIG_LORAMAC_REGION_KR920)
	return lorawan_set_region(LORAWAN_REGION_KR920);
#elif defined(CONFIG_LORAMAC_REGION_IN865)
	return lorawan_set_region(LORAWAN_REGION_IN865);
#elif defined(CONFIG_LORAMAC_REGION_US915)
	return lorawan_set_region(LORAWAN_REGION_US915);
#elif defined(CONFIG_LORAMAC_REGION_RU864)
	return lorawan_set_region(LORAWAN_REGION_RU864);
#else
	return 0;
#endif
}

static int lorawan_start_stack(void)
{
	const struct device *lora_dev = DEVICE_DT_GET(DT_NODELABEL(lora));
	int ret;

	lorawan_init_dev_eui_once();

	if (!device_is_ready(lora_dev)) {
		LOG_ERR("SX126x radio not ready");
		return -ENODEV;
	}

	ret = lorawan_select_region();
	if (ret < 0) {
		LOG_ERR("Failed to set region (%d)", ret);
		return ret;
	}
	LOG_INF("LoRaWAN region selected");

	ret = lorawan_start();
	if (ret < 0) {
		LOG_ERR("lorawan_start failed (%d)", ret);
		return ret;
	}
	LOG_INF("LoRaWAN stack started");

	downlink_cb.port = LW_RECV_PORT_ANY;
	downlink_cb.cb = lorawan_on_downlink;
	lorawan_register_downlink_callback(&downlink_cb);
	lorawan_enable_adr(true);

	ret = lorawan_set_conf_msg_tries(1U);
	if (ret < 0) {
		LOG_WRN("Failed to set confirm retries (%d)", ret);
	}
	LOG_INF("LoRaWAN ADR enabled, confirm retries=%u", 1U);

	return 0;
}

static int lorawan_attempt_join(void)
{
	struct lorawan_join_config join_cfg = { 0 };
	int ret;
	uint16_t dev_nonce;

	join_cfg.mode = LORAWAN_ACT_OTAA;
	join_cfg.dev_eui = dev_eui;
	join_cfg.otaa.join_eui = join_eui;
	join_cfg.otaa.app_key = app_key;
	join_cfg.otaa.nwk_key = app_key;

	// DevNonce must be unique for each OTAA join attempt with the same JoinEUI; use random value with monotonic increment fallback.
	// TODO: store DevNonce in non-volatile memory for true monotonicity across resets, and handle rollover (0..65535).
	dev_nonce = lorawan_random_devnonce();
	if (dev_nonce == last_dev_nonce) {
		dev_nonce++;
	}
	last_dev_nonce = dev_nonce;
	join_cfg.otaa.dev_nonce = dev_nonce;

	app_state.lorawan_join_attempts++;

	LOG_INF("LoRaWAN join attempt #%u (dev_nonce=%u)",
		app_state.lorawan_join_attempts, (unsigned int)dev_nonce);

	ret = lorawan_join(&join_cfg);
	if (ret < 0) {
		LOG_ERR("lorawan_join failed (%d)", ret);
		if (ret == -ETIMEDOUT) {
			LOG_INF("Join timeout: no JoinAccept received in RX windows");
		}
		app_state.lorawan_last_error = ret;
		return ret;
	}

	LOG_INF("LoRaWAN join successful");
	app_state.lorawan_last_error = 0;
	lorawan_session_joined = true;
	app_state.lorawan_joined = true;
	confirmed_fail_streak = 0U;
	LOG_INF("LoRaWAN session established (OTAA); session version is managed by stack/network");

	return 0;
}

static size_t lorawan_build_payload(uint8_t *buffer, size_t buffer_len)
{
	if (buffer_len < TX_PAYLOAD_LENGTH) {
		return 0U;
	}

	/* 0..9: uplink snapshot of 10 inference labels (-2/-1/0..10) */
	for (size_t i = 0; i < 10U; i++) {
		buffer[i] = (uint8_t)app_state.infer_labels_uplink[i];
	}

	/* 10..13: battery/solar voltage from INA3221 in mV */
	int16_t batt_mv = (int16_t)(app_state.ina_ch2_voltage_v * 1000.0f);
	int16_t solar_mv = (int16_t)(app_state.ina_ch1_voltage_v * 1000.0f);
	buffer[10] = (uint8_t)((batt_mv >> 8) & 0xff);
	buffer[11] = (uint8_t)((batt_mv >> 0) & 0xff);
	buffer[12] = (uint8_t)((solar_mv >> 8) & 0xff);
	buffer[13] = (uint8_t)((solar_mv >> 0) & 0xff);

	/* 14: temperature (ambient-compatible single-byte truncation) */
	buffer[14] = (uint8_t)((int)app_state.last_temp_celsius & 0xff);

	/* 15..16: total inferences low 16 bits */
	uint16_t total_inf = (uint16_t)(app_state.infer_total_count_uplink & 0xffffU);
	buffer[15] = (uint8_t)((total_inf >> 8) & 0xff);
	buffer[16] = (uint8_t)((total_inf >> 0) & 0xff);

	/* 17..20: battery/solar current in mA from INA3221 */
	int16_t batt_ma = (int16_t)(app_state.ina_ch2_current_ma);
	int16_t solar_ma = (int16_t)(app_state.ina_ch1_current_ma);
	buffer[17] = (uint8_t)((batt_ma >> 8) & 0xff);
	buffer[18] = (uint8_t)((batt_ma >> 0) & 0xff);
	buffer[19] = (uint8_t)((solar_ma >> 8) & 0xff);
	buffer[20] = (uint8_t)((solar_ma >> 0) & 0xff);

	/* 21: SD write success flag */
	buffer[21] = app_state.sdon;

	/* 22..25: compressed latitude/longitude (ambient-compatible split) */
	buffer[22] = (uint8_t)((int)(app_state.gps_latitude) & 0xff);
	buffer[23] = (uint8_t)((int)(app_state.gps_latitude * 256.0f) & 0xff);
	buffer[24] = (uint8_t)((int)(app_state.gps_longitude) & 0xff);
	buffer[25] = (uint8_t)((int)(app_state.gps_longitude * 256.0f) & 0xff);

	/* 26..27: average RMS (arms) compressed like ambient */
	buffer[26] = (uint8_t)((int)(app_state.infer_avg_rms) & 0xff);
	buffer[27] = (uint8_t)((int)(app_state.infer_avg_rms * 256.0f) & 0xff);

	/* 28: SD usage percent */
	buffer[28] = app_state.sd_usage_percent;

	return TX_PAYLOAD_LENGTH;
}

static int lorawan_send_uplink(void)
{
	uint8_t payload[TX_PAYLOAD_LENGTH];
	size_t len = lorawan_build_payload(payload, sizeof(payload));
	int ret;
	uint32_t next_success_index = app_state.lorawan_uplink_count + 1U;
	// For demonstration, use unconfirmed messages for all uplinks to avoid hitting confirmed uplink failure threshold during testing. 
	// In production, consider using confirmed messages for better reliability, especially if downlink ACKs or commands are expected.
	// enum lorawan_message_type msg_type =
	// 	((next_success_index % LORAWAN_CONFIRMED_EVERY_N) == 0U) ?
	// 	LORAWAN_MSG_CONFIRMED : LORAWAN_MSG_UNCONFIRMED;
	enum lorawan_message_type msg_type = LORAWAN_MSG_UNCONFIRMED;
	const char *msg_type_name =
		(msg_type == LORAWAN_MSG_CONFIRMED) ? "confirmed" : "unconfirmed";

	if (len == 0U) {
		LOG_WRN("Payload buffer too small");
		return -EMSGSIZE;
	}

	ret = lorawan_send(LORAWAN_APP_PORT, payload, len, msg_type);
	if (ret == -EAGAIN) {
		LOG_WRN("lorawan_send busy, will retry (%s uplink)", msg_type_name);
		return ret;
	} else if (ret < 0) {
		LOG_ERR("lorawan_send failed (%d) [%s uplink]", ret, msg_type_name);
		if (msg_type == LORAWAN_MSG_CONFIRMED) {
			confirmed_fail_streak++;
			LOG_WRN("Confirmed uplink failure streak: %u/%u",
				(unsigned int)confirmed_fail_streak,
				(unsigned int)LORAWAN_CONFIRMED_FAIL_LIMIT);
			if (confirmed_fail_streak >= LORAWAN_CONFIRMED_FAIL_LIMIT) {
				lorawan_invalidate_session("confirmed uplink failure threshold reached");
				return -ENOTCONN;
			}
		}
		return ret;
	}

	if (msg_type == LORAWAN_MSG_CONFIRMED) {
		confirmed_fail_streak = 0U;
	}

	LOG_INF("LoRaWAN %s uplink #%u sent (len=%u)",
		msg_type_name,
		(unsigned int)next_success_index,
		(unsigned int)len);
	return 0;
}

int task_lorawan_send_event_uplink(void)
{
	int64_t now = k_uptime_get();
	int ret;

	/* Event-driven only: called when inference reaches 10 samples. */
	if (!lorawan_stack_started) {
		if (now < next_join_deadline) {
			int64_t remain_ms = next_join_deadline - now;
			if (remain_ms < 0) {
				remain_ms = 0;
			}
			LOG_WRN("LoRaWAN init backoff active, skip tx event (remain=%lld ms)", remain_ms);
			return -EAGAIN;
		}

		ret = lorawan_start_stack();
		if (ret < 0) {
			app_state.lorawan_last_error = ret;
			init_fail_count++;
			int64_t retry_ms = lorawan_backoff_ms(init_fail_count);
			next_join_deadline = now + retry_ms;
			LOG_WRN("LoRaWAN stack init failed (err=%d), retry in %lld s",
				ret, retry_ms / 1000);
			return -EIO;
		}

		lorawan_stack_started = true;
		next_join_deadline = now;
		LOG_INF("LoRaWAN stack ready");
	}

	if (!lorawan_session_joined) {
		if (now < next_join_deadline) {
			int64_t remain_ms = next_join_deadline - now;
			if (remain_ms < 0) {
				remain_ms = 0;
			}
			LOG_WRN("LoRaWAN join backoff active, skip tx event (remain=%lld ms)", remain_ms);
			return -EAGAIN;
		}

		// ret = lorawan_attempt_join();
		// if (ret < 0) {
		// 	init_fail_count++;
		// 	int64_t retry_ms = lorawan_backoff_ms(init_fail_count);
		// 	next_join_deadline = now + retry_ms;
		// 	LOG_WRN("LoRaWAN join failed (err=%d), retry in %lld s",
		// 		ret, retry_ms / 1000);
		// 	return;
		// }
		// init_fail_count = 0U;

		// try joining in a loop with short delay, since join can take multiple attempts and we want to avoid long backoff during testing
		bool joined = false;
		for (int i = 0; i < 5; i++) {
			ret = lorawan_attempt_join();
			if (ret == 0) {
				joined = true;
				init_fail_count = 0U;
				break;
			}
			LOG_WRN("LoRaWAN join retry %d/5 failed (err=%d); global attempts=%u, retrying...",
				i + 1, ret,
				(unsigned int)app_state.lorawan_join_attempts);
			k_msleep(15000);
		}
		if(!joined) {
			init_fail_count++;
			int64_t retry_ms = lorawan_backoff_ms(init_fail_count);
			next_join_deadline = now + retry_ms;
			LOG_WRN("LoRaWAN join failed after 5 attempts, retry in %lld s",
				retry_ms / 1000);
			return -ENOTCONN;
		}

	}

	ret = lorawan_send_uplink();
	if (ret < 0) {
		app_state.lorawan_last_error = ret;
		LOG_WRN("LoRaWAN send failed (err=%d)", ret);
		return ret;
	}
	LOG_INF("LoRaWAN uplink event sent successfully");

	app_state.lorawan_last_error = 0;
	app_state.lorawan_uplink_count++;
	app_state.sdon = 0U;
	task_inference_reset_periodic_stats();
	return 0;
}

int task_lorawan_connect(void)
{
	int ret;

	/*
	 * Called from main() startup loop before any event-driven uplink.
	 * Must ensure the LoRaMAC stack is started (lorawan_start() called)
	 * before issuing a join request, otherwise LoRaMacMlmeRequest()
	 * returns LORAMAC_STATUS_BUSY immediately.
	 */
	if (!lorawan_stack_started) {
		ret = lorawan_start_stack();
		if (ret < 0) {
			LOG_ERR("lorawan_start_stack failed (%d)", ret);
			return ret;
		}
		lorawan_stack_started = true;
	}

	return lorawan_attempt_join();
}

void task_lorawan_prepare_sleep(void)
{
	/* The SX126x sits on the v_periph power domain.
	 * Once suspend powers v_periph off, the radio loses its runtime state
	 * and any previously joined LoRaWAN session is no longer valid from the
	 * application's point of view. If we keep lorawan_stack_started /
	 * lorawan_session_joined set, the next uplink path will try to transmit
	 * using a stale stack+radio state and can hit McpsRequest Tx timeout.
	 *
	 * Force a clean stack restart and OTAA rejoin after every power-cycle of
	 * the radio domain.
	 */
	if (lorawan_stack_started || lorawan_session_joined) {
		LOG_INF("LoRaWAN prepare sleep: invalidate stack/session after v_periph power-off");
	}

	lorawan_stack_started = false;
	lorawan_session_joined = false;
	app_state.lorawan_joined = false;
	confirmed_fail_streak = 0U;
	next_join_deadline = k_uptime_get();
}

