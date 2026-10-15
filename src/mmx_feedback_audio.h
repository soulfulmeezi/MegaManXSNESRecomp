#pragma once
#include <stdbool.h>
#include <stdint.h>

/* Host presentation only: no new guest/save/rollback fields. */
typedef struct MmxFeedbackActor {
  uint16_t slash;
  int16_t x, y;
  uint8_t combo, active, grounded;
} MmxFeedbackActor;
void MmxFeedbackAudioFrame(const MmxFeedbackActor actors[2],
                           uint32_t frame, unsigned stage, bool enabled);
void MmxFeedbackAudioReset(void);
