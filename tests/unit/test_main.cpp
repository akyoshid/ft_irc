// Test main with required global variables
#include <signal.h>

// Define global variable required by Server.cpp
volatile sig_atomic_t g_shutdown = 0;

// Google Test will provide its own main() function via -lgtest_main
