#pragma once

enum loglevel_e {
	log_info,
	log_error
};

void log_init(void);
void log_shutdown(void);
void log_printf(enum loglevel_e level, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
