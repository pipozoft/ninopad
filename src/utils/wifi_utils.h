#pragma once
#include <stdbool.h>

bool nino_wifi_connect_from_sd(void);
bool nino_wifi_is_connected(void);
void nino_wifi_disconnect(void);

bool nino_ntp_sync(int timezone_offset);
bool nino_ntp_has_time(void);
