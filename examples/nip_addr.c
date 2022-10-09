// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include "nip_uapi.h"
#include "nip_lib.h"

/* get ifindex based on the device name
 * struct ifreq ifr;
 * struct nip_ifreq ifrn;
 * ioctl(fd, SIOGIFINDEX, &ifr);
 * ifr.ifr_ifindex; ===> ifindex
 */
int32_t nip_add_addr(int32_t ifindex, struct nip_addr *addr, int opt)
{
	int fd, ret;
	struct nip_ifreq ifrn;

	fd = socket(AF_NINET, SOCK_DGRAM, 0);
	if (fd < 0)
		return -1;

	memset(&ifrn, 0, sizeof(ifrn));
	ifrn.ifrn_addr = *addr;
	ifrn.ifrn_ifindex = ifindex;

	ret = ioctl(fd, opt, &ifrn);
	if (ret < 0 && errno != EEXIST) { // ignore File Exists error
		printf("cfg newip addr fail, ifindex=%d, opt=%u, ret=%d.\n", ifindex, opt, ret);
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

void cmd_help(void)
{
	/* nip_addr wlan0 add 01 (在wlan0上配置地址01) */
	/* nip_addr wlan0 del 01 (在wlan0上删除地址01) */
	printf("[cmd example] nip_addr <netcard-name> { add | del } <addr>\n");
}

int main(int argc, char **argv_input)
{
	int ret;
	int opt;
	int ifindex = 0;
	char **argv = argv_input;
	char cmd[ARRAY_LEN];
	char dev[ARRAY_LEN];
	struct nip_addr addr = {0};

	if (argc != DEMO_INPUT_3) {
		printf("unsupport addr cfg input, argc=%u.\n", argc);
		cmd_help();
		return -1;
	}

	/* 配置参数1解析: <netcard-name> */
	argv++;
	memset(dev, 0, ARRAY_LEN);
	ret = sscanf(*argv, "%s", dev);
	if (strncmp(dev, "wlan", NAME_WLAN_LEN) && strncmp(dev, "eth", NAME_ETH_LEN)) {
		printf("unsupport addr cfg cmd-1, cmd=%s.\n", dev);
		cmd_help();
		return -1;
	}
	ret = nip_get_ifindex(dev, &ifindex);
	if (ret != 0)
		return -1;

	/* 配置参数2解析: { add | del } */
	argv++;
	memset(cmd, 0, ARRAY_LEN);
	ret = sscanf(*argv, "%s", cmd);
	if (!strncmp(cmd, "add", NAME_WLAN_LEN)) {
		opt = SIOCSIFADDR;
	} else if (!strncmp(cmd, "del", NAME_WLAN_LEN)) {
		opt = SIOCDIFADDR;
	} else {
		printf("unsupport addr cfg cmd-2, cmd=%s.\n", cmd);
		cmd_help();
		return -1;
	}

	/* 配置参数3解析: <addr> */
	argv++;
	if (nip_get_addr(argv, &addr)) {
		printf("unsupport addr cfg cmd-3.\n");
		cmd_help();
		return 1;
	}

	ret = nip_add_addr(ifindex, &addr, opt);
	if (ret != 0)
		return -1;

	printf("%s (ifindex=%u) cfg addr success.\n", dev, ifindex);
	return 0;
}

