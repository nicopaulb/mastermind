#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/services/hrs.h>

#include "etl/bitset.h"
#include "etl/array.h"
#include "ble.hpp"
#include "combination.hpp"
#include "app_cfg.hpp"

#define BT_UUID_MSTR_SRV_VAL BT_UUID_128_ENCODE(0x00001523, 0x2929, 0xefde, 0x1523, 0x785feabcd123)
#define BT_UUID_MSTR_STATUS_CHAR_VAL BT_UUID_128_ENCODE(0x00001524, 0x2929, 0xefde, 0x1523, 0x785feabcd123)
#define BT_UUID_MSTR_CMD_CHAR_VAL BT_UUID_128_ENCODE(0x00001525, 0x2929, 0xefde, 0x1523, 0x785feabcd123)

#define BT_UUID_MSTR_SRV BT_UUID_DECLARE_128(BT_UUID_MSTR_SRV_VAL)
#define BT_UUID_MSTR_STATUS_CHAR BT_UUID_DECLARE_128(BT_UUID_MSTR_STATUS_CHAR_VAL)
#define BT_UUID_MSTR_CMD_CHAR BT_UUID_DECLARE_128(BT_UUID_MSTR_CMD_CHAR_VAL)

#define LOG_LEVEL 4

LOG_MODULE_REGISTER(ble);

static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static void recycled(void);
static void exchange_mtu(struct bt_conn *conn, uint8_t att_err,
                         struct bt_gatt_exchange_params *params);
static ssize_t read_status(struct bt_conn *conn,
                           const struct bt_gatt_attr *attr, void *buf,
                           uint16_t len, uint16_t offset);
static ssize_t write_command(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr,
                             const void *buf,
                             uint16_t len, uint16_t offset, uint8_t flags);

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL,
                  BT_UUID_16_ENCODE(BT_UUID_DIS_VAL)),
};

static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static struct bt_conn *connection;
static uint8_t status_buf[BLE_STATUS_BUF_SIZE] = {0};
static uint8_t status_buf_len = 0;
static etl::bitset<BT_COMMAND_COUNT> command_flags = 0;
static etl::array<uint8_t, BT_COMMAND_BUF_SIZE> command_buf = {0};

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
    .recycled = recycled,
};

BT_GATT_SERVICE_DEFINE(mstr_svc,
                       BT_GATT_PRIMARY_SERVICE(BT_UUID_MSTR_SRV),
                       BT_GATT_CHARACTERISTIC(BT_UUID_MSTR_STATUS_CHAR, BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_READ,
                                              BT_GATT_PERM_READ, read_status, NULL, NULL),
                       BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
                       BT_GATT_CHARACTERISTIC(BT_UUID_MSTR_CMD_CHAR,
                                              BT_GATT_CHRC_WRITE,
                                              BT_GATT_PERM_WRITE, NULL, write_command,
                                              NULL), );

static void connected(struct bt_conn *conn, uint8_t err)
{
    static struct bt_gatt_exchange_params params;

    if (err)
    {
        LOG_ERR("Connection failed, err 0x%02x %s", err, bt_hci_err_to_str(err));
    }
    else
    {
        LOG_INF("Connected, updating MTU");
        connection = conn;
        params.func = exchange_mtu;
        err = bt_gatt_exchange_mtu(connection, &params);
        if (err)
        {
            LOG_ERR("bt_gatt_exchange_mtu failed (err %d)", err);
        }
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected, reason 0x%02x %s", reason, bt_hci_err_to_str(reason));
    connection = NULL;
}

static void recycled(void)
{
    LOG_INF("Connection recycled. Restarting advertisements");
    int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err)
    {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return;
    }
}

static void exchange_mtu(struct bt_conn *conn, uint8_t att_err,
                         struct bt_gatt_exchange_params *params)
{
    if (att_err)
    {
        LOG_ERR("MTU exchange failed");
    }
    else
    {
        LOG_INF("MTU exchange successful");
    }
}

static ssize_t read_status(struct bt_conn *conn,
                           const struct bt_gatt_attr *attr, void *buf,
                           uint16_t len, uint16_t offset)
{
    LOG_INF("Received request to read game status");
    return bt_gatt_attr_read(conn, attr, buf, len, offset,
                             &status_buf, status_buf_len);
}

static ssize_t write_command(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr,
                             const void *buf,
                             uint16_t len, uint16_t offset, uint8_t flags)
{
    uint8_t cmd = *((uint8_t *)buf);

    LOG_INF("Received write command");

    if (len < 1 || len > BT_COMMAND_BUF_SIZE + 1)
    {
        LOG_ERR("Incorrect data length");
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    if (offset != 0)
    {
        LOG_ERR("Incorrect data offset");
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    switch (cmd)
    {
    case BT_COMMAND_RESET:
    case BT_COMMAND_OFF:
    case BT_COMMAND_CODE:
        LOG_INF("Valid command received");
        command_flags.set(cmd, true);
        break;
    default:
        LOG_ERR("Unknown command");
        return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
    }

    memcpy(command_buf.data(), ((uint8_t *)buf) + 1, len - 1);

    return len;
}

/**
 * @brief Initialise the Bluetooth Low Energy module.
 *
 * @return true if the initialization was successful, false otherwise.
 */
bool ble_init(void)
{
    int32_t err = bt_enable(NULL);
    if (err)
    {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return false;
    }

    LOG_INF("Starting advertising");
    err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err)
    {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return false;
    }

    return true;
}

/**
 * @brief Updates the game status buffer, which is then sent to connected devices.
 *
 * The game status is made up of the following elements:
 * - The correct combination (code) object, serialized.
 * - The number of tries that have been made so far.
 * - The combinations that have been tried before, serialized.
 *
 * @param tentatives The tentative combinations that have been tried.
 * @param code The correct combination.
 * @param try_nb The number of tries that have been made so far.
 */
void ble_update_status(etl::array<combination, MAX_TRY> &tentatives, combination &code, uint8_t try_nb)
{
    uint8_t *buf_ptr = status_buf;

    buf_ptr = code.serialize(buf_ptr);
    memcpy(buf_ptr, &try_nb, sizeof(try_nb));
    buf_ptr += sizeof(try_nb);
    for (uint8_t i = 0; i < try_nb; i++)
    {
        buf_ptr = tentatives[i].serialize(buf_ptr);
    }

    status_buf_len = buf_ptr - &status_buf[0];
    ble_status_notify();
}

/**
 * @brief Notify the connected device about the game status.
 */
void ble_status_notify(void)
{
    LOG_HEXDUMP_INF(status_buf, status_buf_len, "Status buffer");
    const struct bt_gatt_attr *attr = &mstr_svc.attrs[1];
    if (connection && bt_gatt_is_subscribed(connection, attr, BT_GATT_CCC_NOTIFY))
    {
        LOG_INF("Sending notification to update game status");
        int err = bt_gatt_notify(connection, attr, status_buf, status_buf_len);
        if (err)
        {
            LOG_ERR("Failed to send notification (err %d)", err);
        }
    }
}

/**
 * @brief Returns a reference to the command flags bitset.
 *
 * @return A reference to the command flags bitset.
 */
etl::bitset<BT_COMMAND_COUNT> &ble_get_commands(void)
{
    return command_flags;
}

/**
 * @brief Returns a reference to the command buffer array.
 *
 * @return A reference to the command buffer array.
 */
etl::array<uint8_t, BT_COMMAND_BUF_SIZE> &ble_get_command_buf(void)
{
    return command_buf;
}