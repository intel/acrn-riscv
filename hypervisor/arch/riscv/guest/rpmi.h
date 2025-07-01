/*
 * Copyright (C) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RISCV_RPMI_H__
#define __RISCV_RPMI_H__

#include <lib/types.h>

enum rpmi_mm_type {
	RPMI_MM_GET_ATTR = 0,
	RPMI_MM_SET_ATTR ,
	RPMI_MM_SEND_WITH_RESP,
	RPMI_MM_SEND_WITHOUT_RESP,
	RPMI_MM_NOTIF_EVENT,
	RPMI_MM_MAX_TYPE,
};

enum rpmi_mm_attr_id {
	RPMI_MM_ATTR_SPEC_VERSION = 0,
	RPMI_MM_ATTR_MAX_MSG_DATA_SIZE,
	RPMI_MM_ATTR_SERVICE_ID,
	RPMI_MM_ATTR_SERVICE_VERSION,
	RPMI_MM_ATTR_MAX_ID,
};

struct rpmi_mm_request {
	enum rpmi_mm_type type;
	union {
		struct {
			uint16_t notif_datalen;
			uint8_t notif_id;
			uint8_t notif_data[];
		};

		struct {
			enum rpmi_mm_attr_id attr_id;
			uint32_t attr_val;
		};

		struct {
			uint32_t comm_serv_id;
			void *comm_req;
			uint64_t comm_req_len;
			void *comm_resp;
			uint64_t comm_max_resp_len;
			uint64_t comm_out_resp_len;
		};
	};
	int32_t error;
} __aligned(4096);

enum rpmi_reqfwd_type {
	RPMI_REQFWD_NOTIFI_EVENT = 0x01,
	RPMI_REQFWD_RETRI_MESG = 0x02,
	RPMI_REQFWD_COMPL_MESG = 0x03,
	RPMI_REQFWD_MAX_TYPE,
};

struct rpmi_reqfwd_request {
	union {
		struct {
			union {
				uint32_t notif_id;
				int32_t notif_status;
			};
			uint32_t notif_state;
		};

		struct {
			union {
				uint32_t retri_index;
				struct {
					int32_t retri_status;
					uint32_t retri_remain;
					uint32_t retri_returned;
					uint8_t retri_mesg;
				};
			};
		};

		struct {
			union {
				uint8_t *compl_data;
				struct {
					int32_t compl_status;
					uint32_t compl_num;
				};
			};
		};
	};
} __aligned(4096);

#endif /* __RISCV_RPMI_H__ */
