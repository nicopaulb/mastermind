#ifndef BLE_H
#define BLE_H

#include "etl/bitset.h"
#include "etl/array.h"

#include "combination.hpp"
#include "app_cfg.hpp"

#define BLE_STATUS_BUF_SIZE 128

#define BT_COMMAND_RESET 0
#define BT_COMMAND_OFF 1
#define BT_COMMAND_CODE 2
#define BT_COMMAND_COUNT 3
#define BT_COMMAND_BUF_SIZE 8

bool ble_init(void);
void ble_update_status(etl::array<combination, MAX_TRY> &tentatives, combination &code, uint8_t try_nb);
void ble_status_notify();
etl::bitset<BT_COMMAND_COUNT> &ble_get_commands(void);
etl::array<uint8_t, BT_COMMAND_BUF_SIZE> &ble_get_command_buf(void);

#endif
