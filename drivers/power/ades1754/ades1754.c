/***************************************************************************//**
 *   @file   ades1754.c
 *   @brief  Source file for the ADES1754 Driver
 *   @author Radu Sabau (radu.sabau@analog.com)
********************************************************************************
 * Copyright 2024(c) Analog Devices, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. “AS IS” AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL ANALOG DEVICES, INC. BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/
#include "ades1754.h"
#include "no_os_alloc.h"
#include "no_os_crc8.h"
#include "no_os_delay.h"
#include "no_os_error.h"

NO_OS_DECLARE_CRC8_TABLE(crc_table);

int ades1754_hello_all(struct ades1754_desc *desc)
{
	uint8_t data[5];

	data[0] = ADES1754_PREAMBLE_BYTE;
	data[1] = ADES1754_HELLO_ALL_BYTE;
	data[2] = 0x00;
	data[3] = desc->dev_addr;
	data[4] = ADES1754_STOP_BYTE;

	return no_os_uart_write(desc->uart_desc, data, 5);
}

int ades1754_write_dev(struct ades1754_desc *desc, uint8_t reg, uint16_t data)
{
	uint8_t tx_data[7];

	tx_data[0] = ADES1754_PREAMBLE_BYTE;
	tx_data[1] = no_os_field_prep(ADES1754_RW_ADDR_MASK, desc->dev_addr) |
		     ADES1754_WR_MASK;
	tx_data[2] = reg;
	tx_data[3] = no_os_field_get(ADES1754_LSB_MASK, data);
	tx_data[4] = no_os_field_get(ADES1754_MSB_MASK, data);
	tx_data[5] = no_os_crc8(crc_table, tx_data, 5, 0x00);
	tx_data[6] = ADES1754_STOP_BYTE;

	return no_os_uart_write(desc->uart_desc, tx_data, 7);
}

int ades1754_write_all(struct ades1754_desc *desc, uint8_t reg, uint16_t data)
{
	uint8_t tx_data[7];

	tx_data[0] = ADES1754_PREAMBLE_BYTE;
	tx_data[1] = 0x02;
	tx_data[2] = reg;
	tx_data[3] = no_os_field_get(ADES1754_LSB_MASK, data);
	tx_data[4] = no_os_field_get(ADES1754_MSB_MASK, data);
	tx_data[5] = no_os_crc8(crc_table, tx_data, 5, 0x00);
	tx_data[6] = ADES1754_STOP_BYTE;

	return no_os_uart_write(desc->uart_desc, tx_data, 7);
}

int ades1754_read_dev(struct ades1754_desc *desc, uint8_t reg, uint16_t *data)
{
	uint8_t tx_data[8];
	uint8_t rx_data[8];
	uint8_t pec;
	int ret;

	tx_data[0] = ADES1754_PREAMBLE_BYTE;
	tx_data[1] = no_os_field_prep(ADES1754_RW_ADDR_MASK, desc->dev_addr) |
		     ADES1754_RD_MASK;
	tx_data[2] = reg;
	tx_data[3] = 0x00;
	tx_data[4] = no_os_crc8(crc_table, tx_data, 4, 0x00);
	tx_data[5] = 0xC2;
	tx_data[6] = 0xD3;
	tx_data[7] = ADES1754_STOP_BYTE;

	ret = no_os_uart_write(desc->uart_desc, tx_data, 8);
	if (ret)
		return ret;

	ret = no_os_uart_read(desc->uart_desc, rx_data, 8);
	if (ret)
		return ret;

	pec = no_os_crc8(crc_table, rx_data, 6, 0x00);
	if (pec != rx_data[6])
		return -EINVAL;

	*data = no_os_field_prep(ADES1754_MSB_MASK, rx_data[4]) | rx_data[3];

	return 0;
}

int ades1754_read_all(struct ades1754_desc *desc, uint8_t reg, uint16_t *data)
{
	uint8_t tx_data[6 + 2 * desc->no_dev];
	uint8_t rx_data[6 + 2 * desc->no_dev];
	int ret, i, j, read_b = 0, retries = 0;
	uint8_t bytes_nb = 5 + 2 * desc->no_dev;
	uint8_t pec;

	tx_data[0] = ADES1754_PREAMBLE_BYTE;
	tx_data[1] = 0x03;
	tx_data[2] = reg;
	tx_data[3] = 0x00;
	tx_data[4] = no_os_crc8(crc_table, tx_data, 4, 0x00);

	for (i = 5; i < bytes_nb; i += 2) {
		tx_data[i] = 0xC2;
		tx_data[i + 1] = 0xD3;
	}

	tx_data[bytes_nb] = ADES1754_STOP_BYTE;

	ret = no_os_uart_write(desc->uart_desc, tx_data, bytes_nb);
	if (ret < 0)
		return ret;

	while (read_b < bytes_nb) {
		ret = no_os_uart_read(desc->uart_desc, &rx_data[read_b], bytes_nb - read_b);
		if (ret >= 0)
			read_b += ret;
		else {
			retries++;
			if (retries > 100)
				return -ETIMEDOUT;
		}
	}

	pec = no_os_crc8(crc_table, rx_data, 4 + 2 * desc->no_dev, 0x00);

	if (pec != rx_data[4 + 2 * desc->no_dev])
		return -EINVAL;

	j = 0;
	for (i = 3; i < 3 + 2 * desc->no_dev; i += 2) {
		data[j] = rx_data[i] | no_os_field_prep(ADES1754_MSB_MASK, rx_data[ i + 1]);
		j++;
	}

	return 0;
}

int ades1754_read_block(struct ades1754_desc *desc, uint8_t block, uint8_t reg,
			uint16_t *data, bool double_size)
{
	uint8_t pec, bytes_nb = 9;
	uint8_t tx_data[11];
	uint8_t rx_data[11];
	int ret;

	tx_data[0] = ADES1754_PREAMBLE_BYTE;
	tx_data[1] = no_os_field_prep(ADES1754_RW_ADDR_MASK, block) |
		     ADES1754_BL_MASK;
	tx_data[2] = desc->dev_addr;
	tx_data[3] = reg;
	tx_data[4] = 0x00; // DC
	tx_data[5] = no_os_crc8(crc_table, tx_data, 5, 0x00);
	tx_data[6] = 0xC2;
	tx_data[7] = 0xD3;
	tx_data[8] = ADES1754_STOP_BYTE;

	if (double_size) {
		tx_data[8] = tx_data[6];
		tx_data[9] = tx_data[7];
		tx_data[10]  = ADES1754_STOP_BYTE;

		bytes_nb = 11;
	}

	ret = no_os_uart_write(desc->uart_desc, tx_data, bytes_nb);
	if (ret)
		return ret;

	ret = no_os_uart_read(desc->uart_desc, rx_data, bytes_nb);
	if (ret)
		return ret;

	pec = no_os_crc8(crc_table, rx_data, bytes_nb - 2, 0x00);
	if (pec != rx_data[bytes_nb - 2])
		return -EINVAL;

	data[0] = no_os_field_prep(ADES1754_MSB_MASK, rx_data[5]) | rx_data[4];
	if (double_size)
		data[1] = no_os_field_prep(ADES1754_MSB_MASK, rx_data[7]) |
			  rx_data[6];

	return 0;
}

int ades1754_update_dev(struct ades1754_desc *desc, uint8_t reg, uint16_t mask,
			uint16_t val)
{
	uint16_t reg_val;
	int ret;

	ret = ades1754_read_dev(desc, reg, &reg_val);
	if (ret)
		return ret;

	reg_val &= ~mask;
	reg_val |= no_os_field_prep(mask, val);

	return ades1754_write_dev(desc, reg, reg_val);
}

int ades1754_set_adc_method(struct ades1754_desc *desc,
			    enum ades1754_scan_method scan_method)
{
	return ades1754_update_dev(desc, ADES1754_SCANCTRL_REG,
				   ADES1754_ADC_METHOD_MASK, scan_method);
}

int ades1754_switch_scan_mode(struct ades1754_desc *desc,
			      enum ades1754_scan_mode mode)
{
	int ret;

	ret = ades1754_update_dev(desc, ADES1754_SCANCTRL_REG,
				  ADES1754_SCAN_MODE_MASK, mode);
	if (ret)
		return ret;

	desc->scan_mode = mode;

	return 0;
}

int ades1754_set_cell_pol(struct ades1754_desc *desc,
			  enum ades1754_cell_polarity *cell_polarity)
{
	uint16_t reg_val = 0;
	int ret, i;

	for (i = 0; i < sizeof(cell_polarity); i++)
		reg_val |= NO_OS_BIT(i);

	ret = ades1754_write_dev(desc, ADES1754_POLARITYCTRL_REG, reg_val);
	if (ret)
		return ret;

	desc->cell_polarity = reg_val;

	return 0;
}

int ades1754_start_scan(struct ades1754_desc *desc, bool meas,
			uint16_t cell_mask)
{
	uint16_t reg_val;
	int ret;

	if (cell_mask > NO_OS_GENMASK(14, 1))
		return -EINVAL;

	ret = ades1754_write_dev(desc, ADES1754_MEASUREEN1_REG, cell_mask);
	if (ret)
		return ret;

	return ades1754_update_dev(desc, ADES1754_SCANCTRL_REG,
				   ADES1754_SCAN_REQUEST_MASK, meas);
}

int ades1754_get_cell_data(struct ades1754_desc *desc, uint8_t cell_nb,
			   int32_t *cell_voltage)
{
	uint16_t reg_val;
	int ret;

	ret = ades1754_read_dev(desc, ADES1754_CELLN_REG(cell_nb), &reg_val);
	if (ret)
		return ret;

	*cell_voltage = reg_val;

	if (no_os_field_get(NO_OS_BIT(cell_nb), desc->cell_polarity))
		*cell_voltage = no_os_sign_extend32(reg_val, 13);

	return 0;
}

int ades1754_set_iir(struct ades1754_desc *desc,
		     enum ades1754_iir_filter_coef coef)
{
	return ades1754_update_dev(desc, ADES1754_DEVCFG2_REG,
				   ADES1754_IIR_MASK, coef);
}

int ades1754_set_iir_ctrl(struct ades1754_desc *desc, bool alrtfilt, bool acc,
			  bool output)
{
	uint16_t val;

	val = no_os_field_prep(ADES1754_ALRTFILT_MASK, alrtfilt);
	val |= no_os_field_prep(ADES1754_AMENDFILT_MASK, acc);
	val |= no_os_field_prep(ADES1754_RDFILT_MASK, output);

	return ades1754_update_dev(desc, ADES1754_SCANCTRL_REG,
				   ADES1754_IIR_SCAN_MASK, val);
}

int ades1754_set_buffer_mode(struct ades1754_desc *desc,
			     enum ades1754_buffer_mode mode)
{
	return ades1754_update_dev(desc, ADES1754_DEVCFG_REG,
				   ADES1754_DBLBUFEN_MASK, mode);
}

int ades1754_set_alert_thr(struct ades1754_desc *desc,
			   enum ades1754_alert alert,
			   uint16_t thr)
{
	uint8_t reg, reg2;
	uint16_t mask;
	int ret;

	switch (alert) {
	case ADES1754_CELL_OV:
		mask = NO_OS_GENMASK(15, 2);
		reg = ADES1754_OVTHCLR_REG;
		reg2 = ADES1754_OVTHSET_REG;

		break;
	case ADES1754_CELL_UV:
		mask = NO_OS_GENMASK(15, 2);
		reg = ADES1754_UVTHCLR_REG;
		reg2 = ADES1754_UVTHSET_REG;

		break;
	case ADES1754_BIPOLAR_OV:
		mask = NO_OS_GENMASK(15, 2);
		reg = ADES1754_BIPOVTHCLR_REG;
		reg2 = ADES1754_BIPOVTHSET_REG;

		break;
	case ADES1754_BIPOLAR_UV:
		mask = NO_OS_GENMASK(15, 2);
		reg = ADES1754_BIPUVTHCLR_REG;
		reg2 = ADES1754_BIPUVTHSET_REG;

		break;
	case ADES1754_BLOCK_OV:
		mask = NO_OS_GENMASK(15, 2);
		reg = ADES1754_BLKOVTHCLR_REG;
		reg2 = ADES1754_BLKOVTHSET_REG;

		break;
	case ADES1754_BLOCK_UV:
		mask = NO_OS_GENMASK(15, 2);
		reg = ADES1754_BLKUVTHCLR_REG;
		reg2 = ADES1754_BLKUVTHSET_REG;

		break;
	case ADES1754_CELL_MISMATCH:
		mask = NO_OS_GENMASK(15, 2);

		return ades1754_write_dev(desc, ADES1754_MSMTCH_REG,
					  no_os_field_prep(mask, thr));
	case ADES1754_AUXIN_OV:
		mask = NO_OS_GENMASK(15, 2);
		reg = ADES1754_AUXAOVTHCLR_REG;
		reg2 = ADES1754_AUXAOVTHSET_REG;

		break;
	case ADES1754_AUXIN_UV:
		mask = NO_OS_GENMASK(15, 2);
		reg = ADES1754_AUXAUVTHCLR_REG;
		reg2 = ADES1754_AUXAUVTHSET_REG;

		break;
	default:
		return -EINVAL;
	}

	ret = ades1754_write_dev(desc, reg, no_os_field_prep(mask, thr));
	if (ret)
		return ret;

	return ades1754_write_dev(desc, reg2, no_os_field_prep(mask, thr));
}

int ades1754_get_alert(struct ades1754_desc *desc, enum ades1754_alert alert,
		       bool *enable)
{
	uint16_t mask, reg_val;
	int ret;

	switch (alert) {
	case ADES1754_CELL_OV:
	case ADES1754_BIPOLAR_OV:
		mask = NO_OS_BIT(12);

		break;
	case ADES1754_CELL_UV:
	case ADES1754_BIPOLAR_UV:
		mask = NO_OS_BIT(11);

		break;
	case ADES1754_BLOCK_OV:
		mask = NO_OS_BIT(10);

		break;
	case ADES1754_BLOCK_UV:
		mask = NO_OS_BIT(9);

		break;
	case ADES1754_CELL_MISMATCH:
		mask = NO_OS_BIT(13);

		break;
	case ADES1754_AUXIN_OV:
		mask = NO_OS_BIT(8);

		break;
	case ADES1754_AUXIN_UV:
		mask = NO_OS_BIT(7);

		break;
	default:
		return -EINVAL;
	}

	ret = ades1754_read_dev(desc, ADES1754_STATUS1_REG, &reg_val);
	if (ret)
		return ret;

	*enable = no_os_field_get(mask, reg_val);

	return 0;
}

int ades1754_set_balancing_mode(struct ades1754_desc *desc,
				enum ades1754_bal_mode mode)
{
	return ades1754_update_dev(desc, ADES1754_BALCTRL_REG,
				   ADES1754_CBMODE_MASK, mode);
}

int ades1754_set_balancing_meas(struct ades1754_desc *desc,
				enum ades1754_bal_meas meas)
{
	return ades1754_update_dev(desc, ADES1754_BALCTRL_REG,
				   ADES1754_CBMEASEN_MASK, meas);
}

int ades1754_set_balancing_calib(struct ades1754_desc *desc,
				 enum ades1754_bal_calib calib)
{
	return ades1754_update_dev(desc, ADES1754_BALDLYCTRL_REG,
				   ADES1754_CBCALDLY_MASK, calib);
}

int ades1754_init(struct ades1754_desc **desc,
		  struct ades1754_init_param *init_param)
{
	struct ades1754_desc *descriptor;
	int ret;

	if (init_param->dev_addr > ADES1754_DEV_ADDR_MAX)
		return -EINVAL;

	descriptor = (struct ades1754_desc *)no_os_calloc(sizeof(*descriptor),
			1);
	if (!descriptor)
		return -ENOMEM;

	ret = no_os_uart_init(&descriptor->uart_desc, init_param->uart_param);
	if (ret)
		goto free_desc;

	no_os_crc8_populate_msb(crc_table, 0xA6); // 0xB2 if 0xA6 doesn't work

	ret = ades1754_update_dev(desc, ADES1754_DEVCFG_REG,
				  ADES1754_UARTCFG_MASK, 0);
	if (ret)
		goto free_uart;

	descriptor->dev_addr = init_param->dev_addr;
	descriptor->no_dev = init_param->no_dev;
	descriptor->alive = false;

	*desc = descriptor;

	return 0;

free_uart:
	no_os_uart_remove(descriptor->uart_desc);
free_desc:
	no_os_free(descriptor);

	return ret;
}

int ades1754_remove(struct ades1754_desc *desc)
{
	no_os_mdelay(10);
	no_os_uart_remove(desc->uart_desc);
	no_os_free(desc);

	return 0;
}
