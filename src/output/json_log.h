#ifndef CANMOD_JSON_LOG_H
#define CANMOD_JSON_LOG_H

#include <stdint.h>

/**
 * Emit a single NDJSON line for the decoded CAN frame.
 *
 * Called by the main loop immediately after car_process_frame() when the
 * frame ID is one the car module recognises.
 *
 * Output format:
 *   {"ts":12345,"id":"0x3B5","signals":{"fan_speed":3,"ac_on":1,...}}\n
 *
 * @param ts   Timestamp in ms since boot (from hw_tick_get()).
 * @param id   11-bit CAN frame identifier.
 */
void json_log_emit(uint32_t ts, uint32_t id);

#endif /* CANMOD_JSON_LOG_H */
