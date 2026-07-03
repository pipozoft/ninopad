#pragma once
#include <stdint.h>
#include <stdbool.h>

#define SD_CS    5
#define SD_SCK   18
#define SD_MOSI  23
#define SD_MISO  19

bool nino_sd_mount(void);
bool nino_sd_unmount(void);
bool nino_sd_is_mounted(void);
