// SPDX-FileCopyrightText: 2017 ahmed-kamal <ahmedkamal200427@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-only

#include "sms.h"

RZ_IPI bool sms_bin_init(void **plugin_data) {
	SmsBinContext *sms_bin_ctx = RZ_NEW0(SmsBinContext);
	if (!sms_bin_ctx) {
		return false;
	}
    sms_bin_ctx->cb = 0;
	*plugin_data = sms_bin_ctx;
	return true;
}

RZ_IPI void sms_bin_fini(void *plugin_data) {
    if(!plugin_data){
        return;
    }
	SmsBinContext *sms_bin_ctx = plugin_data;
	free(sms_bin_ctx);
}