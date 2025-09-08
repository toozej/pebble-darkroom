#ifndef UNITY_CONFIG_H_INCLUDED
#define UNITY_CONFIG_H_INCLUDED

/* Pebble-specific Unity configuration for embedded environment */

/* Disable floating point support - Pebble doesn't need it for these tests */
#define UNITY_EXCLUDE_FLOAT
#define UNITY_EXCLUDE_DOUBLE

/* Disable file I/O - use simple output for Pebble */
#define UNITY_OUTPUT_CHAR(a) putchar((a))
#define UNITY_OUTPUT_FLUSH() fflush(stdout)

/* Use hex display style for pointers and numbers */
#define UNITY_DISPLAY_STYLE_HEX
#define UNITY_DISPLAY_STYLE_POINTER

/* Small buffer sizes for embedded environment */
#define UNITY_OUTPUT_LINE_LENGTH 80
#define UNITY_MAX_MESSAGE_LENGTH 100

/* Disable verbose output for cleaner test results */
#define UNITY_WITHOUT_PREPROCESSOR
#define UNITY_WITHOUT_CLEANUP

/* Use simple assertions suitable for embedded */
#define UNITY_INCLUDE_DOUBLE
#define UNITY_INCLUDE_FLOAT

/* Set up and tear down functions will be provided by test files */
extern void setUp(void);
extern void tearDown(void);

#endif /* UNITY_CONFIG_H_INCLUDED */