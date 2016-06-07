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
/* l2cap.h - L2CAP handling */


#define BT_L2CAP_CID_ATT		0x0004
#define BT_L2CAP_CID_LE_SIG		0x0005
#define BT_L2CAP_CID_SMP		0x0006

struct bt_l2cap_hdr {
	uint16_t len;
	uint16_t cid;
} __packed;

struct bt_l2cap_sig_hdr {
	uint8_t  code;
	uint8_t  ident;
	uint16_t len;
} __packed;

#define BT_L2CAP_REJ_NOT_UNDERSTOOD	0x0000
#define BT_L2CAP_REJ_MTU_EXCEEDED	0x0001
#define BT_L2CAP_REJ_INVALID_CID	0x0002

#define BT_L2CAP_CMD_REJECT		0x01
struct bt_l2cap_cmd_reject {
	uint16_t reason;
	uint8_t  data[0];
} __packed;

#define BT_L2CAP_CONN_PARAM_REQ		0x12
struct bt_l2cap_conn_param_req {
	uint16_t min_interval;
	uint16_t max_interval;
	uint16_t latency;
	uint16_t timeout;
} __packed;

#define BT_L2CAP_CONN_PARAM_RSP		0x13
struct bt_l2cap_conn_param_rsp {
	uint16_t result;
} __packed;

struct bt_l2cap_chan {
	uint16_t		cid;

	void			(*connected)(struct bt_conn *conn);
	void			(*disconnected)(struct bt_conn *conn);
	void			(*encrypt_change)(struct bt_conn *conn);

	void			(*recv)(struct bt_conn *conn,
					struct bt_buf *buf);

	struct bt_l2cap_chan	*_next;
};

/* Register a fixed L2CAP channel for L2CAP */
void bt_l2cap_chan_register(struct bt_l2cap_chan *chan);

/* Notify L2CAP channels of a new connection */
void bt_l2cap_connected(struct bt_conn *conn);

/* Notify L2CAP channels of a disconnect event */
void bt_l2cap_disconnected(struct bt_conn *conn);

/* Notify L2CAP channels of a change in encryption state */
void bt_l2cap_encrypt_change(struct bt_conn *conn);

/* Prepare an L2CAP PDU to be sent over a connection */
struct bt_buf *bt_l2cap_create_pdu(struct bt_conn *conn);

/* Send L2CAP PDU over a connection */
void bt_l2cap_send(struct bt_conn *conn, uint16_t cid, struct bt_buf *buf);

/* Receive a new L2CAP PDU from a connection */
void bt_l2cap_recv(struct bt_conn *conn, struct bt_buf *buf);

/* Perform connection parameter update request */
void bt_l2cap_update_conn_param(struct bt_conn *conn);

/* Initialize L2CAP and supported channels */
int bt_l2cap_init(void);
