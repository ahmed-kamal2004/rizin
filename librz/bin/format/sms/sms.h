// SPDX-FileCopyrightText: 2025 ahmed-kamal <ahmedkamal200427@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-only

#ifndef SMS_H
#define SMS_H
#include <rz_util.h>

typedef struct sms_bin_context_t {
    ut32 cb;
} SmsBinContext;

RZ_IPI bool sms_bin_init(void **plugin_data); ///< initialize a context of type SmsBinContext
RZ_IPI void sms_bin_fini(void *plugin_data); ///< free SmsBinContext*

#endif