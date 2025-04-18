/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (C) 2013-2024 OpenMV, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * WINC1500 bus wrapper.
 */
#include <stdint.h>
#include <string.h>

#include "py/mphal.h"

#include "omv_boardconfig.h"
#include "omv_gpio.h"
#include "omv_spi.h"

#include "conf_winc.h"
#include "bsp/include/nm_bsp.h"
#include "common/include/nm_common.h"
#include "bus_wrapper/include/nm_bus_wrapper.h"

#define NM_BUS_MAX_TRX_SZ   (4096)
#define NM_BUS_SPI_TIMEOUT  (1000)

static omv_spi_t spi_bus;

tstrNmBusCapabilities egstrNmBusCapabilities = {
	NM_BUS_MAX_TRX_SZ
};

sint8 nm_bus_init(void *pvinit) {
	sint8 result = M2M_SUCCESS;

    omv_spi_config_t spi_config;
    omv_spi_default_config(&spi_config, OMV_WINC_SPI_ID);

    spi_config.baudrate    = OMV_WINC_SPI_BAUDRATE;
    spi_config.nss_enable  = false; // Soft NSS

    if (omv_spi_init(&spi_bus, &spi_config) != 0) {
        result = M2M_ERR_BUS_FAIL;
    }

    nm_bsp_reset();

	return result;
}

sint8 nm_bus_deinit(void) {
    omv_spi_deinit(&spi_bus);
	return M2M_SUCCESS;
}

static sint8 nm_bus_rw(uint8 *txbuf, uint8 *rxbuf, uint16 size) {
    sint8 result = M2M_SUCCESS;
    omv_spi_transfer_t spi_xfer = {
        .txbuf = txbuf,
        .rxbuf = rxbuf,
        .size = size,
        .timeout = NM_BUS_SPI_TIMEOUT,
        .flags = OMV_SPI_XFER_BLOCKING,
        .callback = NULL,
        .userdata = NULL,
    };

    if (txbuf == NULL) {
        memset(rxbuf, 0, size);
        spi_xfer.txbuf = rxbuf;
        spi_xfer.rxbuf = rxbuf;
    }

    omv_gpio_write(spi_bus.cs, 0);
    omv_spi_transfer_start(&spi_bus, &spi_xfer);
    omv_gpio_write(spi_bus.cs, 1);

    return result;
}

sint8 nm_bus_ioctl(uint8 cmd, void *arg) {
	sint8 ret = 0;
	switch (cmd) {
		case NM_BUS_IOCTL_RW: {
			tstrNmSpiRw *spi_rw = (tstrNmSpiRw *) arg;
			ret = nm_bus_rw(spi_rw->pu8InBuf, spi_rw->pu8OutBuf, spi_rw->u16Sz);
		}
		break;
		default:
			ret = -1;
			M2M_ERR("Invalid IOCTL\n");
			break;
	}
	return ret;
}
