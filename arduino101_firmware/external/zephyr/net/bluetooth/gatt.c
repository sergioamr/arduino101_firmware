/*
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/* gatt.c - Generic Attribute Profile handling */


#include <nanokernel.h>
#include <toolchain.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <misc/byteorder.h>
#include <misc/util.h>

#include <bluetooth/log.h>
#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/buf.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "hci_core.h"
#include "conn_internal.h"
#include "keys.h"
#include "l2cap.h"
#include "att.h"

#if !defined(CONFIG_BLUETOOTH_DEBUG_GATT)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

static const struct bt_gatt_attr *db = NULL;
static size_t attr_count = 0;

void bt_gatt_register(const struct bt_gatt_attr *attrs, size_t count)
{
	db = attrs;
	attr_count = count;
}

int bt_gatt_attr_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		      void *buf, uint8_t buf_len, uint16_t offset,
		      const void *value, uint8_t value_len)
{
	uint8_t len;

	if (offset > value_len) {
		return -EINVAL;
	}

	len = min(buf_len, value_len - offset);

	BT_DBG("handle 0x%04x offset %u length %u\n", attr->handle, offset,
	       len);

	memcpy(buf, value + offset, len);

	return len;
}

int bt_gatt_attr_read_service(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      void *buf, uint8_t len, uint16_t offset)
{
	struct bt_uuid *uuid = attr->user_data;

	if (uuid->type == BT_UUID_16) {
		uint16_t uuid16 = sys_cpu_to_le16(uuid->u16);

		return bt_gatt_attr_read(conn, attr, buf, len, offset, &uuid16,
					 sizeof(uuid16));
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, uuid->u128,
				 sizeof(uuid->u128));
}

struct gatt_incl {
	uint16_t start_handle;
	uint16_t end_handle;
	union {
		uint16_t uuid16;
		uint8_t  uuid[16];
	};
} __packed;

int bt_gatt_attr_read_include(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      void *buf, uint8_t len, uint16_t offset)
{
	struct bt_gatt_include *incl = attr->user_data;
	struct gatt_incl pdu;
	uint8_t value_len;

	pdu.start_handle = sys_cpu_to_le16(incl->start_handle);
	pdu.end_handle = sys_cpu_to_le16(incl->end_handle);
	value_len = sizeof(pdu.start_handle) + sizeof(pdu.end_handle);

	if (incl->uuid->type == BT_UUID_16) {
		pdu.uuid16 = sys_cpu_to_le16(incl->uuid->u16);
		value_len += sizeof(pdu.uuid16);
	} else {
		memcpy(pdu.uuid, incl->uuid->u128, sizeof(incl->uuid->u128));
		value_len += sizeof(incl->uuid->u128);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &pdu, value_len);
}

struct gatt_chrc {
	uint8_t properties;
	uint16_t value_handle;
	union {
		uint16_t uuid16;
		uint8_t  uuid[16];
	};
} __packed;

int bt_gatt_attr_read_chrc(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint8_t len, uint16_t offset)
{
	struct bt_gatt_chrc *chrc = attr->user_data;
	struct gatt_chrc pdu;
	uint8_t value_len;

	pdu.properties = chrc->properties;
	pdu.value_handle = sys_cpu_to_le16(chrc->value_handle);
	value_len = sizeof(pdu.properties) + sizeof(pdu.value_handle);

	if (chrc->uuid->type == BT_UUID_16) {
		pdu.uuid16 = sys_cpu_to_le16(chrc->uuid->u16);
		value_len += sizeof(pdu.uuid16);
	} else {
		memcpy(pdu.uuid, chrc->uuid->u128, sizeof(chrc->uuid->u128));
		value_len += sizeof(chrc->uuid->u128);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &pdu, value_len);
}

void bt_gatt_foreach_attr(uint16_t start_handle, uint16_t end_handle,
			  bt_gatt_attr_func_t func, void *user_data)
{
	size_t i;

	for (i = 0; i < attr_count; i++) {
		const struct bt_gatt_attr *attr = &db[i];

		/* Check if attribute handle is within range */
		if (attr->handle < start_handle || attr->handle > end_handle)
			continue;

		if (func(attr, user_data) == BT_GATT_ITER_STOP)
			break;
	}
}

int bt_gatt_attr_read_ccc(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  uint8_t len, uint16_t offset)
{
	struct _bt_gatt_ccc *ccc = attr->user_data;
	uint16_t value;
	size_t i;

	for (i = 0; i < ccc->cfg_len; i++) {
		if (bt_addr_le_cmp(&ccc->cfg[i].peer, &conn->dst)) {
			continue;
		}

		value = sys_cpu_to_le16(ccc->cfg[i].value);
		break;
	}

	/* Default to disable if there is no cfg for the peer */
	if (i == ccc->cfg_len) {
		value = 0x0000;
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(value));
}

static void gatt_ccc_changed(struct _bt_gatt_ccc *ccc)
{
	int i;
	uint16_t value = 0x0000;

	for (i = 0; i < ccc->cfg_len; i++) {
		if (ccc->cfg[i].value > value) {
			value = ccc->cfg[i].value;
		}
	}

	BT_DBG("ccc %p value 0x%04x\n", ccc, value);

	if (value != ccc->value) {
		ccc->value = value;
		ccc->cfg_changed(value);
	}
}

int bt_gatt_attr_write_ccc(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, const void *buf,
			   uint8_t len, uint16_t offset)
{
	struct _bt_gatt_ccc *ccc = attr->user_data;
	const uint16_t *data = buf;
	bool bonded;
	size_t i;

	if (len != sizeof(*data) || offset) {
		return -EINVAL;
	}

	if (bt_keys_get_addr(&conn->dst))
		bonded = true;
	else
		bonded = false;

	for (i = 0; i < ccc->cfg_len; i++) {
		/* Check for existing configuration */
		if (!bt_addr_le_cmp(&ccc->cfg[i].peer, &conn->dst)) {
			break;
		}
	}

	if (i == ccc->cfg_len) {
		for (i = 0; i < ccc->cfg_len; i++) {
			/* Check for unused configuration */
			if (!ccc->cfg[i].valid) {
				bt_addr_le_copy(&ccc->cfg[i].peer, &conn->dst);
				/* Only set valid if bonded */
				ccc->cfg[i].valid = bonded;
				break;
			}
		}

		if (i == ccc->cfg_len) {
			BT_WARN("No space to store CCC cfg\n");
			return -ENOMEM;
		}
	}

	ccc->cfg[i].value = sys_le16_to_cpu(*data);

	BT_DBG("handle 0x%04x value %u\n", attr->handle, ccc->cfg[i].value);

	/* Update cfg if don't match */
	if (ccc->cfg[i].value != ccc->value) {
		gatt_ccc_changed(ccc);
	}

	return len;
}

int bt_gatt_attr_read_cep(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  uint8_t len, uint16_t offset)
{
	struct bt_gatt_cep *value = attr->user_data;
	uint16_t props = sys_cpu_to_le16(value->properties);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &props,
				 sizeof(props));
}

struct notify_data {
	const void *data;
	size_t len;
	uint8_t handle;
};

static uint8_t notify_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct notify_data *data = user_data;
	struct bt_uuid uuid = { .type = BT_UUID_16, .u16 = BT_UUID_GATT_CCC };
	struct bt_uuid chrc = { .type = BT_UUID_16, .u16 = BT_UUID_GATT_CHRC };
	struct _bt_gatt_ccc *ccc;
	size_t i;

	if (bt_uuid_cmp(attr->uuid, &uuid)) {
		/* Stop if we reach the next characteristic */
		if (!bt_uuid_cmp(attr->uuid, &chrc)) {
			return BT_GATT_ITER_STOP;
		}
		return BT_GATT_ITER_CONTINUE;
	}

	/* Check attribute user_data must be of type struct _bt_gatt_ccc */
	if (attr->write != bt_gatt_attr_write_ccc) {
		return BT_GATT_ITER_CONTINUE;
	}

	ccc = attr->user_data;

	/* Notify all peers configured */
	for (i = 0; i < ccc->cfg_len; i++) {
		struct bt_conn *conn;
		struct bt_buf *buf;
		struct bt_att_notify *nfy;

		/* TODO: Handle indications */
		if (ccc->value != BT_GATT_CCC_NOTIFY) {
			continue;
		}

		conn = bt_conn_lookup_addr_le(&ccc->cfg[i].peer);
		if (!conn || conn->state != BT_CONN_CONNECTED) {
			continue;
		}

		buf = bt_att_create_pdu(conn, BT_ATT_OP_NOTIFY,
					sizeof(*nfy) + data->len);
		if (!buf) {
			BT_WARN("No buffer available to send notification");
			bt_conn_put(conn);
			return BT_GATT_ITER_STOP;
		}

		BT_DBG("conn %p handle 0x%04x\n", conn, data->handle);

		nfy = bt_buf_add(buf, sizeof(*nfy));
		nfy->handle = sys_cpu_to_le16(data->handle);

		bt_buf_add(buf, data->len);
		memcpy(nfy->value, data->data, data->len);

		bt_l2cap_send(conn, BT_L2CAP_CID_ATT, buf);
		bt_conn_put(conn);
	}

	return BT_GATT_ITER_CONTINUE;
}

void bt_gatt_notify(uint16_t handle, const void *data, size_t len)
{
	struct notify_data nfy;

	nfy.handle = handle;
	nfy.data = data;
	nfy.len = len;

	bt_gatt_foreach_attr(handle, 0xffff, notify_cb, &nfy);
}

static uint8_t connected_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct bt_conn *conn = user_data;
	struct _bt_gatt_ccc *ccc;
	size_t i;

	/* Check attribute user_data must be of type struct _bt_gatt_ccc */
	if (attr->write != bt_gatt_attr_write_ccc) {
		return BT_GATT_ITER_CONTINUE;
	}

	ccc = attr->user_data;

	/* If already enabled skip */
	if (ccc->value) {
		return BT_GATT_ITER_CONTINUE;
	}

	for (i = 0; i < ccc->cfg_len; i++) {
		/* Ignore configuration for different peer */
		if (bt_addr_le_cmp(&conn->dst, &ccc->cfg[i].peer)) {
			continue;
		}

		if (ccc->cfg[i].value) {
			gatt_ccc_changed(ccc);
			return BT_GATT_ITER_CONTINUE;
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

void bt_gatt_connected(struct bt_conn *conn)
{
	BT_DBG("conn %p\n", conn);
	bt_gatt_foreach_attr(0x0001, 0xffff, connected_cb, conn);
}

static uint8_t disconnected_cb(const struct bt_gatt_attr *attr, void *user_data)
{
	struct bt_conn *conn = user_data;
	struct _bt_gatt_ccc *ccc;
	size_t i;

	/* Check attribute user_data must be of type struct _bt_gatt_ccc */
	if (attr->write != bt_gatt_attr_write_ccc) {
		return BT_GATT_ITER_CONTINUE;
	}

	ccc = attr->user_data;

	/* If already disabled skip */
	if (!ccc->value) {
		return BT_GATT_ITER_CONTINUE;
	}

	for (i = 0; i < ccc->cfg_len; i++) {
		/* Ignore configurations with disabled value */
		if (!ccc->cfg[i].value) {
			continue;
		}

		if (bt_addr_le_cmp(&conn->dst, &ccc->cfg[i].peer)) {
			struct bt_conn *tmp;

			/* Skip if there is another peer connected */
			tmp = bt_conn_lookup_addr_le(&ccc->cfg[i].peer);
			if (tmp && tmp->state == BT_CONN_CONNECTED) {
				bt_conn_put(tmp);
				return BT_GATT_ITER_CONTINUE;
			}
		}
	}

	/* Reset value while disconnected */
	memset(&ccc->value, 0, sizeof(ccc->value));
	ccc->cfg_changed(ccc->value);

	BT_DBG("ccc %p reseted\n", ccc);

	return BT_GATT_ITER_CONTINUE;
}

void bt_gatt_disconnected(struct bt_conn *conn)
{
	BT_DBG("conn %p\n", conn);
	bt_gatt_foreach_attr(0x0001, 0xffff, disconnected_cb, conn);
}

static void gatt_mtu_rsp(struct bt_conn *conn, uint8_t err, const void *pdu,
			 uint16_t length, void *user_data)
{
	bt_gatt_rsp_func_t func = user_data;

	func(conn, err);
}

static int gatt_send(struct bt_conn *conn, struct bt_buf *buf,
		     bt_att_func_t func, void *user_data,
		     bt_att_destroy_t destroy)
{
	int err;

	err = bt_att_send(conn, buf, func, user_data, destroy);
	if (err) {
		BT_ERR("Error sending ATT PDU: %d\n", err);
		bt_buf_put(buf);
	}

	return err;
}

int bt_gatt_exchange_mtu(struct bt_conn *conn, bt_gatt_rsp_func_t func)
{
	struct bt_att_exchange_mtu_req *req;
	struct bt_buf *buf;
	uint16_t mtu;

	if (!conn || !func) {
		return -EINVAL;
	}

	buf = bt_att_create_pdu(conn, BT_ATT_OP_MTU_REQ, sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	/* Select MTU based on the amount of room we have in bt_buf including
	 * one extra byte for ATT header.
	 */
	mtu = bt_buf_tailroom(buf) + 1;

	BT_DBG("Client MTU %u\n", mtu);

	req = bt_buf_add(buf, sizeof(*req));
	req->mtu = sys_cpu_to_le16(mtu);

	return gatt_send(conn, buf, gatt_mtu_rsp, func, NULL);
}

static void att_find_type_rsp(struct bt_conn *conn, uint8_t err,
			      const void *pdu, uint16_t length,
			      void *user_data)
{
	const struct bt_att_find_type_rsp *rsp = pdu;
	struct bt_gatt_discover_params *params = user_data;
	uint8_t i;
	uint16_t end_handle = 0, start_handle;

	BT_DBG("err 0x%02x\n", err);

	if (err) {
		goto done;
	}

	/* Parse attributes found */
	for (i = 0; length >= sizeof(rsp->list[i]);
	     i++, length -=  sizeof(rsp->list[i])) {
		const struct bt_gatt_attr *attr;

		start_handle = sys_le16_to_cpu(rsp->list[i].start_handle);
		end_handle = sys_le16_to_cpu(rsp->list[i].end_handle);

		BT_DBG("start_handle 0x%04x end_handle 0x%04x\n", start_handle,
		       end_handle);

		attr = (&(struct bt_gatt_attr)
			BT_GATT_PRIMARY_SERVICE(start_handle, params->uuid));

		if (params->func(attr, params) == BT_GATT_ITER_STOP) {
			goto done;
		}
	}

	/* Stop if could not parse the whole PDU */
	if (length > 0) {
		goto done;
	}

	/* Stop if over the range or the requests */
	if (end_handle >= params->end_handle) {
		goto done;
	}

	/* Continue for the last found handle */
	params->start_handle = end_handle;
	if (!bt_gatt_discover(conn, params)) {
		return;
	}

done:
	if (params->destroy) {
		params->destroy(params);
	}
}

int bt_gatt_discover(struct bt_conn *conn,
		     struct bt_gatt_discover_params *params)
{
	struct bt_buf *buf;
	struct bt_att_find_type_req *req;
	uint16_t *value;

	if (!conn || !params->uuid || !params->func || !params->start_handle ||
	    !params->end_handle) {
		return -EINVAL;
	}

	buf = bt_att_create_pdu(conn, BT_ATT_OP_FIND_TYPE_REQ, sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	req = bt_buf_add(buf, sizeof(*req));
	req->start_handle = sys_cpu_to_le16(params->start_handle);
	req->end_handle = sys_cpu_to_le16(params->end_handle);
	req->type = sys_cpu_to_le16(BT_UUID_GATT_PRIMARY);

	BT_DBG("uuid 0x%04x start_handle 0x%04x end_handle 0x%04x\n",
	       params->uuid->u16, params->start_handle, params->end_handle);

	switch (params->uuid->type) {
	case BT_UUID_16:
		value = bt_buf_add(buf, sizeof(*value));
		*value = sys_cpu_to_le16(params->uuid->u16);
		break;
	case BT_UUID_128:
		bt_buf_add(buf, sizeof(params->uuid->u128));
		memcpy(req->value, params->uuid->u128,
		       sizeof(params->uuid->u128));
		break;
	default:
		BT_ERR("Unkown UUID type %u\n", params->uuid->type);
		bt_buf_put(buf);
		return -EINVAL;
	}

	return gatt_send(conn, buf, att_find_type_rsp, params, NULL);
}

static void att_read_type_rsp(struct bt_conn *conn, uint8_t err,
			      const void *pdu, uint16_t length,
			      void *user_data)
{
	const struct bt_att_read_type_rsp *rsp = pdu;
	struct bt_gatt_discover_params *params = user_data;
	struct bt_uuid uuid;
	uint16_t handle = 0;
	struct bt_gatt_chrc value;

	BT_DBG("err 0x%02x\n", err);

	if (err) {
		goto done;
	}

	/* Data can be either in UUID16 or UUID128 */
	switch (rsp->len) {
	case 7: /* UUID16 */
		uuid.type = BT_UUID_16;
		break;
	case 21: /* UUID128 */
		uuid.type = BT_UUID_128;
		break;
	default:
		BT_ERR("Invalid data len %u\n", rsp->len);
		goto done;
	}

	/* Parse characteristics found */
	for (length--, pdu = rsp->data; length >= rsp->len;
	     length -= rsp->len, pdu += rsp->len) {
		const struct bt_gatt_attr *attr;
		const struct bt_att_data *data = pdu;
		struct gatt_chrc *chrc = (void *)data->value;

		handle = sys_le16_to_cpu(data->handle);
		/* Handle 0 is invalid */
		if (!handle) {
			goto done;
		}

		/* Convert characteristic data, bt_gatt_chrc and gatt_chrc
		 * have different formats so the convertion have to be done
		 * field by field.
		 */
		value.properties = chrc->properties;
		value.value_handle = sys_le16_to_cpu(chrc->value_handle);
		value.uuid = &uuid;

		switch(uuid.type) {
		case BT_UUID_16:
			uuid.u16 = sys_le16_to_cpu(chrc->uuid16);
			break;
		case BT_UUID_128:
			memcpy(uuid.u128, chrc->uuid, sizeof(chrc->uuid));
			break;
		}

		BT_DBG("handle 0x%04x properties 0x%02x value_handle 0x%04x\n",
		       handle, value.properties, value.value_handle);

		/* Skip if UUID is set but doesn't match */
		if (params->uuid && bt_uuid_cmp(&uuid, params->uuid)) {
			continue;
		}

		attr = (&(struct bt_gatt_attr)
			BT_GATT_CHARACTERISTIC(handle, &value));

		if (params->func(attr, params) == BT_GATT_ITER_STOP) {
			goto done;
		}
	}

	/* Stop if could not parse the whole PDU */
	if (length > 0) {
		goto done;
	}

	/* Stop if over the requested range */
	if (params->start_handle >= params->end_handle) {
		goto done;
	}

	/* Continue to the next range */
	if (!bt_gatt_discover_characteristic(conn, params)) {
		return;
	}

done:
	if (params->destroy) {
		params->destroy(params);
	}
}

int bt_gatt_discover_characteristic(struct bt_conn *conn,
				    struct bt_gatt_discover_params *params)
{
	struct bt_buf *buf;
	struct bt_att_read_type_req *req;
	uint16_t *value;

	if (!conn || !params->func || !params->start_handle ||
	    !params->end_handle) {
		return -EINVAL;
	}

	buf = bt_att_create_pdu(conn, BT_ATT_OP_READ_TYPE_REQ, sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	req = bt_buf_add(buf, sizeof(*req));
	req->start_handle = sys_cpu_to_le16(params->start_handle);
	req->end_handle = sys_cpu_to_le16(params->end_handle);

	value = bt_buf_add(buf, sizeof(*value));
	*value = sys_cpu_to_le16(BT_UUID_GATT_CHRC);

	BT_DBG("start_handle 0x%04x end_handle 0x%04x\n", params->start_handle,
	       params->end_handle);

	return gatt_send(conn, buf, att_read_type_rsp, params, NULL);
}

static void att_find_info_rsp(struct bt_conn *conn, uint8_t err,
			      const void *pdu, uint16_t length,
			      void *user_data)
{
	const struct bt_att_find_info_rsp *rsp = pdu;
	struct bt_gatt_discover_params *params = user_data;
	struct bt_uuid uuid;
	uint16_t handle = 0;
	uint8_t len;
	union {
		const struct bt_att_info_16 *i16;
		const struct bt_att_info_128 *i128;
	} info;

	BT_DBG("err 0x%02x\n", err);

	if (err) {
		goto done;
	}

	/* Data can be either in UUID16 or UUID128 */
	switch (rsp->format) {
	case BT_ATT_INFO_16:
		uuid.type = BT_UUID_16;
		len = sizeof(info.i16);
		break;
	case BT_ATT_INFO_128:
		uuid.type = BT_UUID_128;
		len = sizeof(info.i128);
		break;
	default:
		BT_ERR("Invalid format %u\n", rsp->format);
		goto done;
	}

	/* Parse descriptors found */
	for (length--, pdu = rsp->info; length >= len;
	     length -= len, pdu += len) {
		const struct bt_gatt_attr *attr;

		info.i16 = pdu;
		handle = sys_le16_to_cpu(info.i16->handle);

		switch (uuid.type) {
		case BT_UUID_16:
			uuid.u16 = sys_le16_to_cpu(info.i16->uuid);
			break;
		case BT_UUID_128:
			memcpy(uuid.u128, info.i128->uuid, sizeof(uuid.u128));
			break;
		}

		BT_DBG("handle 0x%04x\n", handle);

		/* Skip if UUID is set but doesn't match */
		if (params->uuid && bt_uuid_cmp(&uuid, params->uuid)) {
			continue;
		}

		attr = (&(struct bt_gatt_attr)
			BT_GATT_DESCRIPTOR(handle, &uuid, 0, NULL, NULL, NULL));

		if (params->func(attr, params) == BT_GATT_ITER_STOP) {
			goto done;
		}
	}

	/* Stop if could not parse the whole PDU */
	if (length > 0) {
		goto done;
	}

	/* Next characteristic shall be after current value handle */
	params->start_handle = handle;
	if (params->start_handle < UINT16_MAX) {
		params->start_handle++;
	}

	/* Stop if over the requested range */
	if (params->start_handle >= params->end_handle) {
		goto done;
	}

	/* Continue to the next range */
	if (!bt_gatt_discover_descriptor(conn, params)) {
		return;
	}

done:
	if (params->destroy) {
		params->destroy(params);
	}
}

int bt_gatt_discover_descriptor(struct bt_conn *conn,
				struct bt_gatt_discover_params *params)
{
	struct bt_buf *buf;
	struct bt_att_find_info_req *req;

	if (!conn || !params->func || !params->start_handle ||
	    !params->end_handle) {
		return -EINVAL;
	}

	buf = bt_att_create_pdu(conn, BT_ATT_OP_FIND_INFO_REQ, sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	req = bt_buf_add(buf, sizeof(*req));
	req->start_handle = sys_cpu_to_le16(params->start_handle);
	req->end_handle = sys_cpu_to_le16(params->end_handle);

	BT_DBG("start_handle 0x%04x end_handle 0x%04x\n", params->start_handle,
	       params->end_handle);

	return gatt_send(conn, buf, att_find_info_rsp, params, NULL);
}

static void att_read_rsp(struct bt_conn *conn, uint8_t err, const void *pdu,
			 uint16_t length, void *user_data)
{
	bt_gatt_read_func_t func = user_data;

	BT_DBG("err 0x%02x\n", err);

	if (err) {
		func(conn, err, NULL, 0);
		return;
	}

	func(conn, 0, pdu, length);
}

static int gatt_read_blob(struct bt_conn *conn, uint16_t handle,
			  uint16_t offset, bt_gatt_read_func_t func)
{
	struct bt_buf *buf;
	struct bt_att_read_blob_req *req;

	buf = bt_att_create_pdu(conn, BT_ATT_OP_READ_BLOB_REQ, sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	req = bt_buf_add(buf, sizeof(*req));
	req->handle = sys_cpu_to_le16(handle);
	req->offset = sys_cpu_to_le16(offset);

	BT_DBG("handle 0x%04x offset 0x%04x\n", handle, offset);

	return gatt_send(conn, buf, att_read_rsp, func, NULL);
}

int bt_gatt_read(struct bt_conn *conn, uint16_t handle, uint16_t offset,
		 bt_gatt_read_func_t func)
{
	struct bt_buf *buf;
	struct bt_att_read_req *req;

	if (!conn || !handle || !func) {
		return -EINVAL;
	}

	if (offset) {
		return gatt_read_blob(conn, handle, offset, func);
	}

	buf = bt_att_create_pdu(conn, BT_ATT_OP_READ_REQ, sizeof(*req));
	if (!buf) {
		return -ENOMEM;
	}

	req = bt_buf_add(buf, sizeof(*req));
	req->handle = sys_cpu_to_le16(handle);

	BT_DBG("handle 0x%04x\n", handle);

	return gatt_send(conn, buf, att_read_rsp, func, NULL);
}

static void att_write_rsp(struct bt_conn *conn, uint8_t err, const void *pdu,
			  uint16_t length, void *user_data)
{
	bt_gatt_rsp_func_t func = user_data;

	BT_DBG("err 0x%02x\n", err);

	func(conn, err);
}

static int gatt_write_cmd(struct bt_conn *conn, uint16_t handle,
			  const void *data, uint16_t length)
{
	struct bt_buf *buf;
	struct bt_att_write_cmd *cmd;

	buf = bt_att_create_pdu(conn, BT_ATT_OP_WRITE_CMD,
				sizeof(*cmd) + length);
	if (!buf) {
		return -ENOMEM;
	}

	cmd = bt_buf_add(buf, sizeof(*cmd));
	cmd->handle = sys_cpu_to_le16(handle);
	memcpy(cmd->value, data, length);
	bt_buf_add(buf, length);

	BT_DBG("handle 0x%04x length %u\n", handle, length);

	return gatt_send(conn, buf, NULL, NULL, NULL);
}

int bt_gatt_write(struct bt_conn *conn, uint16_t handle, const void *data,
		  uint16_t length, bt_gatt_rsp_func_t func)
{
	struct bt_buf *buf;
	struct bt_att_write_req *req;

	if (!conn || !handle) {
		return -EINVAL;
	}

	if (!func) {
		return gatt_write_cmd(conn, handle, data, length);
	}

	buf = bt_att_create_pdu(conn, BT_ATT_OP_WRITE_REQ,
				sizeof(*req) + length);
	if (!buf) {
		return -ENOMEM;
	}

	req = bt_buf_add(buf, sizeof(*req));
	req->handle = sys_cpu_to_le16(handle);
	memcpy(req->value, data, length);
	bt_buf_add(buf, length);

	BT_DBG("handle 0x%04x length %u\n", handle, length);

	return gatt_send(conn, buf, att_write_rsp, func, NULL);
}

void bt_gatt_cancel(struct bt_conn *conn)
{
	bt_att_cancel(conn);
}

int bt_gatt_read_multiple(struct bt_conn *conn, const uint16_t *handles,
			  size_t count, bt_gatt_read_func_t func)
{
	struct bt_buf *buf;
	uint8_t i;

	if (!conn && conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	if (!handles || count < 2 || !func) {
		return -EINVAL;
	}

	buf = bt_att_create_pdu(conn, BT_ATT_OP_READ_MULT_REQ,
				count * sizeof(*handles));
	if (!buf) {
		return -ENOMEM;
	}

	for (i = 0; i < count; i++) {
		bt_buf_add_le16(buf, handles[i]);
	}

	return gatt_send(conn, buf, att_read_rsp, func, NULL);
}
