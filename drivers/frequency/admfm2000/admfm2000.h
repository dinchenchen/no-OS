/***************************************************************************//**
 *   @file   admfm2000.h
 *   @brief  Header file for admfm2000 Driver.
 *   @author Ramona Nechita (ramona.nechita@analog.com)
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

#ifndef SRC_ADMFM2000_H_
#define SRC_ADMFM2000_H_

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/
#include <stdint.h>
#include "no_os_gpio.h"

/******************************************************************************/
/********************** Macros and Constants Definitions **********************/
/******************************************************************************/
#define ADMFM2000_MIXER_MODE		0
#define ADMFM2000_DIRECT_IF_MODE	1
#define ADMFM2000_DSA_GPIOS		5
#define ADMFM2000_MODE_GPIOS		2
#define ADMFM2000_MAX_GAIN		0
#define ADMFM2000_MIN_GAIN		-31000
#define ADMFM2000_DEFAULT_GAIN		-0x20

/******************************************************************************/
/*************************** Types Declarations *******************************/
/******************************************************************************/

struct admfm2000_init_param {
        /* Mixer Mode */
        uint8_t mixer_mode;
        /* GAIN */
        int32_t dsa_gain;
        /* GPIO Control Switch chan 0 */
        struct no_os_gpio_init_param *gpio_sw0_param[2];
        /* GPIO Control Switch chan 1 */
        struct no_os_gpio_init_param *gpio_sw1_param[2];
        /* GPIO Control DSA chan 0 */
        struct no_os_gpio_init_param *gpio_dsa0_param[5];
        /* GPIO Control DSA chan 1 */
        struct no_os_gpio_init_param *gpio_dsa1_param[5];
};

struct admfm2000_dev {
        /* GPIO Control Switch chan 0 */
        struct no_os_gpio_desc *gpio_sw0[2];
        /* GPIO Control Switch chan 1 */
        struct no_os_gpio_desc *gpio_sw1[2];
        /* GPIO Control DSA chan 0 */
        struct no_os_gpio_desc *gpio_dsa0[5];
        /* GPIO Control DSA chan 1 */
        struct no_os_gpio_desc *gpio_dsa1[5];
};

/******************************************************************************/
/************************ Functions Declarations ******************************/
/******************************************************************************/

/**
 * @brief Initialize the admfm2000 device.
 * @param device - The device structure.
 * @param init_param - The structure that contains the device initial parameters.
 * @return Returns 0 in case of success or negative error code otherwise.
 */
int32_t admfm2000_init(struct admfm2000_dev **device,
                       struct admfm2000_init_param *init_param);

/**                                                                    
 * @brief Free the resources allocated by admfm2000_init().                     
 * @param dev - The device structure.                                            
 * @return ret - Result of the remove procedure.                                 
 */
int32_t admfm2000_remove(struct admfm2000_dev *dev);

/**
 * @brief Set the gain of the device.
 * @param dev - The device structure.
 * @param chan - The channel for which the gain is set.
 * @param gain - The gain value.
 * @return Returns 0 in case of success or negative error code otherwise.
 */
int32_t admfm2000_set_gain(struct admfm2000_dev *dev, uint8_t chan,
                           int32_t gain);

/**
 * @brief Get the gain of the device.
 * @param dev - The device structure.
 * @param chan - The channel for which the gain is read.
 * @param gain - The gain value.
 * @return Returns 0 in case of success or negative error code otherwise.
 */
int32_t admfm2000_get_gain(struct admfm2000_dev *dev, uint8_t chan,
                           int32_t *gain);

/**
 * @brief Set the channel mode.
 * @param dev - The device structure.
 * @param mode - The mode value.
 * @return Returns 0 in case of success or negative error code otherwise.
 */
int32_t admfm2000_get_channel_mode(struct admfm2000_dev *dev, uint8_t mode);
/**
 * @brief Set the channel configuration.
 * @param dev - The device structure.
 * @param config - The config 
 * @return Returns 0 in case of success or negative error code otherwise.
 */
int32_t admfm2000_set_channel_mode(struct admfm2000_dev *dev, uint8_t config);

#endif /* SRC_ADMFM2000_H_ */