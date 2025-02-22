/*
 * Copyright (c) 2015-2017 The Linux Foundation. All rights reserved.
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

/**
 * DOC: wlan_hdd_subnet_detect.c
 *
 * WLAN Host Device Driver subnet detect API implementation
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <net/cfg80211.h>
#include <ani_global.h>
#include "sme_api.h"
#include "wlan_hdd_main.h"
#include "wlan_hdd_subnet_detect.h"

/*
 * define short names for the global vendor params
 * used by __wlan_hdd_cfg80211_set_gateway_params()
 */
#define PARAM_MAC_ADDR QCA_WLAN_VENDOR_ATTR_GW_PARAM_CONFIG_GW_MAC_ADDR
#define PARAM_IPV4_ADDR QCA_WLAN_VENDOR_ATTR_GW_PARAM_CONFIG_IPV4_ADDR
#define PARAM_IPV6_ADDR QCA_WLAN_VENDOR_ATTR_GW_PARAM_CONFIG_IPV6_ADDR

static const struct nla_policy
	policy[QCA_WLAN_VENDOR_ATTR_GW_PARAM_CONFIG_MAX + 1] = {
		[PARAM_MAC_ADDR] = {
				.type = NLA_BINARY,
				.len = QDF_MAC_ADDR_SIZE
		},
		[PARAM_IPV4_ADDR] = {
				.type = NLA_BINARY,
				.len = QDF_IPV4_ADDR_SIZE
		},
		[PARAM_IPV6_ADDR] = {
				.type = NLA_BINARY,
				.len = QDF_IPV6_ADDR_SIZE
		}
};

/**
 * __wlan_hdd_cfg80211_set_gateway_params() - set gateway params
 * @wiphy: Pointer to wireless phy
 * @wdev: Pointer to wireless device
 * @data: Pointer to data
 * @data_len: Data length
 *
 * Return: 0 on success, negative errno on failure
 */
static int __wlan_hdd_cfg80211_set_gateway_params(struct wiphy *wiphy,
		struct wireless_dev *wdev,
		const void *data,
		int data_len)
{
	struct net_device *dev = wdev->netdev;
	hdd_adapter_t *adapter = WLAN_HDD_GET_PRIV_PTR(dev);
	hdd_context_t *hdd_ctx = wiphy_priv(wiphy);
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_GW_PARAM_CONFIG_MAX + 1];
	struct gateway_param_update_req req = { 0 };
	int ret;
	QDF_STATUS status;

	ENTER_DEV(dev);

	ret = wlan_hdd_validate_context(hdd_ctx);
	if (0 != ret)
		return ret;

	/* user may have disabled the feature in INI */
	if (!hdd_ctx->config->enable_lfr_subnet_detection) {
		hdd_info("LFR Subnet Detection disabled in INI");
		return -ENOTSUPP;
	}

	/* The gateway parameters are only valid in the STA persona
	 * and only in the connected state.
	 */
	if (QDF_STA_MODE != adapter->device_mode) {
		hdd_err("Received GW param update for non-STA mode adapter");
		return -ENOTSUPP;
	}

	if (!hdd_conn_is_connected(WLAN_HDD_GET_STATION_CTX_PTR(adapter))) {
		hdd_err("Received GW param update in disconnected state!");
		return -ENOTSUPP;
	}

	/* Extract NL parameters
	 * mac_addr:  6 bytes
	 * ipv4 addr: 4 bytes
	 * ipv6 addr: 16 bytes
	 */
	if (hdd_nla_parse(tb, QCA_WLAN_VENDOR_ATTR_GW_PARAM_CONFIG_MAX, data,
			  data_len, policy)) {
		hdd_err("Invalid ATTR list");
		return -EINVAL;
	}

	if (!tb[PARAM_MAC_ADDR]) {
		hdd_err("request mac addr failed");
		return -EINVAL;
	}
	nla_memcpy(req.gw_mac_addr.bytes, tb[PARAM_MAC_ADDR],
			QDF_MAC_ADDR_SIZE);

	/* req ipv4_addr_type and ipv6_addr_type are initially false due
	 * to zeroing the struct
	 */
	if (tb[PARAM_IPV4_ADDR]) {
		nla_memcpy(req.ipv4_addr, tb[PARAM_IPV4_ADDR],
			QDF_IPV4_ADDR_SIZE);
		req.ipv4_addr_type = true;
	}

	if (tb[PARAM_IPV6_ADDR]) {
		nla_memcpy(&req.ipv6_addr, tb[PARAM_IPV6_ADDR],
			QDF_IPV6_ADDR_SIZE);
		req.ipv6_addr_type = true;
	}

	if (!req.ipv4_addr_type && !req.ipv6_addr_type) {
		hdd_err("invalid ipv4 or ipv6 gateway address");
		return -EINVAL;
	}

	req.max_retries = 3;
	req.timeout = 100;   /* in milliseconds */
	req.session_id = adapter->sessionId;

	hdd_info("**** Gateway Parameters: ****");
	hdd_info("session id: %d", req.session_id);
	hdd_info("ipv4 addr type: %d", req.ipv4_addr_type);
	hdd_info("ipv6 addr type: %d", req.ipv6_addr_type);
#ifdef CONFIG_HUAWEI_WIFI
	hdd_info("gw mac addr: %02x:%02x:**:**:%02x:%02x",
                req.gw_mac_addr.bytes[0],req.gw_mac_addr.bytes[1],
                req.gw_mac_addr.bytes[4],req.gw_mac_addr.bytes[5]);
#else
	hdd_info("gw mac addr: %pM", req.gw_mac_addr.bytes);
#endif
	hdd_info("ipv4 addr: %pI4", req.ipv4_addr);
	hdd_info("ipv6 addr: %pI6c", req.ipv6_addr);

	status = sme_gateway_param_update(hdd_ctx->hHal, &req);
	if (!QDF_IS_STATUS_SUCCESS(status)) {
		hdd_err("sme_gateway_param_update failed(err=%d)", status);
		ret = -EINVAL;
	}

	EXIT();
	return ret;
}

/**
 * wlan_hdd_cfg80211_set_gateway_params() - set gateway parameters
 * @wiphy:    wiphy structure pointer
 * @wdev:     Wireless device structure pointer
 * @data:     Pointer to the data received
 * @data_len: Length of @data
 *
 * The API is invoked by the user space to set the gateway parameters
 * such as mac address and the IP address which is used for detecting
 * the IP subnet change
 *
 * Return: 0 on success; errno on failure
 */
int wlan_hdd_cfg80211_set_gateway_params(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int data_len)
{
	int ret;

	cds_ssr_protect(__func__);

	ret = __wlan_hdd_cfg80211_set_gateway_params(
				wiphy, wdev, data, data_len);
	cds_ssr_unprotect(__func__);
	return ret;
}
#undef PARAM_MAC_ADDR
#undef PARAM_IPV4_ADDR
#undef PARAM_IPV6_ADDR
