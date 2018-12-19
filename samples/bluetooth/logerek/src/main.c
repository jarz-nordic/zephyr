/* main.c - Bluetooth Cycling Speed and Cadence app main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <gatt/bas.h>

s16_t temp_in = 0xBABA;
u16_t humidity_in = 0xBACA;
s16_t temp_out;

static ssize_t read_temperature(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				void *buf, u16_t len, u16_t offset);

static ssize_t read_humidity(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf, u16_t len, u16_t offset);

static ssize_t write_temperature(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 const void *buf, u16_t len, u16_t offset,
				 u8_t flags);

static struct bt_gatt_attr logger_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_ESS),

	/* Internal temperature */
	BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
			       read_temperature, NULL, NULL),
	BT_GATT_CUD("Temperatura", BT_GATT_PERM_READ),

	/* Internal humidity */
	BT_GATT_CHARACTERISTIC(BT_UUID_HUMIDITY,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
			       read_humidity, NULL, NULL),
	BT_GATT_CUD("Wilgotność", BT_GATT_PERM_READ),

	/* External temperature */
	BT_GATT_CHARACTERISTIC(BT_UUID_HEAT_INDEX,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_WRITE,
			       NULL, write_temperature, NULL),
	BT_GATT_CUD("Temperatura zadana", BT_GATT_PERM_WRITE),
};
static struct bt_gatt_service logger_svc = BT_GATT_SERVICE(logger_attrs);


static ssize_t read_temperature(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				void *buf, u16_t len, u16_t offset)
{
	u16_t value = temp_in;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(value));
}

static ssize_t read_humidity(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf, u16_t len, u16_t offset)
{
	s16_t value = humidity_in;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(value));
}

static ssize_t write_temperature(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 const void *buf, u16_t len, u16_t offset,
				 u8_t flags)
{
	const s16_t *val = buf;

	if (len != 2) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	temp_out = *val;
	printk("otrzymana temperatura = %d\n", temp_out);

	bt_gatt_notify(conn, &logger_attrs[4], buf, sizeof(*val));

	return len;
}


static void connected(struct bt_conn *conn, u8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
	} else {
		printk("Connected\n");
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x1A, 0x18, 0x0f, 0x18),
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	bas_init();
	bt_gatt_service_register(&logger_svc);

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

void main(void)
{
	int err;

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);

	while (1) {
		k_sleep(MSEC_PER_SEC);

		/* Battery level simulation */
		bas_notify();
	}
}
