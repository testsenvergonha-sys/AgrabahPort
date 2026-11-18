#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Redirect stdout/stderr into an in-memory live ring buffer so the launcher
 * can display real-time logs without spawning a Windows console window. */
void aladdin_live_log_init(void);

/* Copy the buffered log text (NUL-terminated) into dst.
 * Returns the number of bytes written (excluding NUL). */
size_t recomp_live_log_copy(char *dst, size_t cap, uint64_t *out_seq, size_t *out_lines);

/* Clear the buffered log. */
void recomp_live_log_clear(void);

/* Total number of lines currently buffered. */
size_t recomp_live_log_line_count(void);

#ifdef __cplusplus
}
#endif
