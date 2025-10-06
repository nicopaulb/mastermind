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

#include "ble.hpp"

#define LOG_LEVEL 4

#define STATE_CONNECTED 1U
#define STATE_DISCONNECTED 2U

#define BT_UUID_MSTR_SRV_VAL BT_UUID_128_ENCODE(0x00001523, 0x2929, 0xefde, 0x1523, 0x785feabcd123)
#define BT_UUID_MSTR_STATUS_CHAR_VAL BT_UUID_128_ENCODE(0x00001524, 0x2929, 0xefde, 0x1523, 0x785feabcd123)
#define BT_UUID_MSTR_RESET_CHAR_VAL BT_UUID_128_ENCODE(0x00001525, 0x2929, 0xefde, 0x1523, 0x785feabcd123)

#define BT_UUID_MSTR_SRV BT_UUID_DECLARE_128(BT_UUID_MSTR_SRV_VAL)
#define BT_UUID_MSTR_STATUS_CHAR BT_UUID_DECLARE_128(BT_UUID_MSTR_STATUS_CHAR_VAL)
#define BT_UUID_MSTR_RESET_CHAR BT_UUID_DECLARE_128(BT_UUID_MSTR_RESET_CHAR_VAL)

LOG_MODULE_REGISTER(ble);

static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static ssize_t read_status(struct bt_conn *conn,
				const struct bt_gatt_attr *attr, void *buf,
				uint16_t len, uint16_t offset);

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL,
                  BT_UUID_16_ENCODE(BT_UUID_HRS_VAL),
                  BT_UUID_16_ENCODE(BT_UUID_BAS_VAL),
                  BT_UUID_16_ENCODE(BT_UUID_DIS_VAL)),
};

static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

BT_GATT_SERVICE_DEFINE(mstr_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_MSTR_SRV),
	BT_GATT_CHARACTERISTIC(BT_UUID_MSTR_STATUS_CHAR, BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_status, NULL, NULL),
    BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	// BT_GATT_CHARACTERISTIC(BT_UUID_MSTR_RESET_CHAR,
	// 		       BT_GATT_CHRC_WRITE,
	// 		       BT_GATT_PERM_WRITE, NULL, write_ctrl_point,
	// 		       &sensor_location),
);

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err)
    {
        LOG_ERR("Connection failed, err 0x%02x %s", err, bt_hci_err_to_str(err));
    }
    else
    {
        LOG_INF("Connected\n");
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected, reason 0x%02x %s", reason, bt_hci_err_to_str(reason));
}

static ssize_t read_status(struct bt_conn *conn,
				const struct bt_gatt_attr *attr, void *buf,
				uint16_t len, uint16_t offset)
{
	uint8_t buf2[3] = {0x12, 0x13, 0x14};

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &buf2, sizeof(buf2));
}

void ble_init(void)
{
    int err;

    err = bt_enable(NULL);
    if (err)
    {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return;
    }

    LOG_INF("Bluetooth initialized");
    LOG_INF("Starting Legacy Advertising (connectable and scannable)");
    err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err)
    {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return;
    }

    LOG_INF("Advertising successfully started");
}

void ble_notify(void)
{
    uint8_t buf[3] = {0x12, 0x13, 0x14};
    bt_gatt_notify(NULL, &mstr_svc.attrs[1], buf, sizeof(buf));
}