#ifndef _TERMINAL_COLOR_H
#define _TERMINAL_COLOR_H


#ifdef _WIN32
char *CLR = "\033[0m";
char *WHITEB = "\033[1;97m"; /* White + bold */
char *ITALICIZE = "\033[3m";
char *GREEN  = "\033[0;92m";
char *GREENB = "\033[1;92m"; /* Green + bold */
char *YELLOW  = "\033[0;93m";
char *YELLOWB = "\033[1;93m"; /* Yellow + bold */
char *RED  = "\033[0;91m";
char *REDB = "\033[1;91m"; /* Red + bold */

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

#define ENABLE_CONSOLE_COLOR() \
  /* Attempt to enable console colors.  This succeeds in Windows 10.  For other OSes \
   * color is disabled. */						\
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);			\
  DWORD consoleMode = 0;						\
  if ((hConsole == INVALID_HANDLE_VALUE) || (!GetConsoleMode(hConsole, &consoleMode)) || (!SetConsoleMode(hConsole, consoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))) { \
    CLR = "";								\
    WHITEB = "";							\
    ITALICIZE = "";							\
    GREEN = "";								\
    GREENB = "";							\
    YELLOW = "";							\
    YELLOWB = "";							\
    RED = "";								\
    REDB = "";								\
  }

#else  /* On Linux... */
char *CLR = "\033[0m";
char *WHITEB = "\033[1;97m"; /* White + bold */
char *ITALICIZE = "\033[3m";
char *GREEN  = "\033[0;32m";
char *GREENB = "\033[1;32m"; /* Green + bold */
char *YELLOW  = "\033[0;33m";
char *YELLOWB = "\033[1;33m"; /* Yellow + bold */
char *RED  = "\033[0;31m";
char *REDB = "\033[1;31m"; /* Red + bold */

/* Do nothing: Linux consoles have color enabled by default. */
#define ENABLE_CONSOLE_COLOR()

#endif


#endif
