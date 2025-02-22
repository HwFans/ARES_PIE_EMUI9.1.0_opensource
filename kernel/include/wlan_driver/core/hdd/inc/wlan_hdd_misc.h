/*
 * Copyright (c) 2012-2014,2016-2017 The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef WLAN_HDD_MISC_H
#define WLAN_HDD_MISC_H
/*
 * If MULTI_IF_NAME is defined, then prepend MULTI_IF_NAME to the filename
 * to prevent name conflicts when loading multiple instances of the driver.
 */
#ifdef MULTI_IF_NAME
#define PREFIX MULTI_IF_NAME "/"
#else
#define PREFIX ""
#endif

#ifdef MSM_PLATFORM
#ifdef CONFIG_HUAWEI_WIFI
#define WLAN_INI_FILE              "../../vendor/etc/wifi/WCNSS_qcom_cfg.ini"
#else
#define WLAN_INI_FILE              "wlan/qca_cld/" PREFIX "WCNSS_qcom_cfg.ini"
#endif /* CONFIG_HUAWEI_WIFI */
#define WLAN_MAC_FILE              "wlan/qca_cld/" PREFIX "wlan_mac.bin"
#else
#define WLAN_INI_FILE              "wlan/" PREFIX "qcom_cfg.ini"
#define WLAN_MAC_FILE              "wlan/" PREFIX "wlan_mac.bin"
#endif /* MSM_PLATFORM */

#endif /* WLAN_HDD_MISC_H */
