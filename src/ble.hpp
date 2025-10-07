#ifndef BLE_H
#define BLE_H

#include "combination.hpp"
#include "app_cfg.hpp"

#define BLE_STATUS_BUF_SIZE 128

bool ble_init(void);
void ble_update_status(etl::array<combination, MAX_TRY> &tentatives, combination &code, uint8_t try_nb);
void ble_status_notify();

#endif
