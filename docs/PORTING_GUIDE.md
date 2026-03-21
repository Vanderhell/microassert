# Porting Guide

Two files: `massert.h` + `massert.c`. C99 + vsnprintf. Provide clock and reset callbacks.

```cmake
add_library(microassert STATIC lib/microassert/src/massert.c)
target_include_directories(microassert PUBLIC lib/microassert/include)
```

Reset callback examples: `NVIC_SystemReset()` (STM32), `esp_restart()` (ESP32), `exit(1)` (Linux).
