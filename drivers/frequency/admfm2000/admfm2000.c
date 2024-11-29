/***************************************************************************//**
 *   @file   admfm2000.c
 *   @brief  Implementation of admfm2000 Driver.
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

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/
#include "admfm2000.h"
#include "no_os_alloc.h"
#include "no_os_error.h"


int32_t admfm2000_get_gain(struct admfm2000_dev *dev, uint8_t chan,
                           int32_t *gain)
{
        int32_t ret;
        uint8_t i;
        uint8_t bit;

        if (chan > 1)
                return -EINVAL;

        if(chan)
                for (i = 0; i < ADMFM2000_DSA_GPIOS; i++) {
                        ret = no_os_gpio_get_value(dev->gpio_dsa1[i], &bit);
                        if (ret != 0)
                                return ret;
                        *gain &= bit << i;
                }
        else
                for (i = 0; i < ADMFM2000_DSA_GPIOS; i++) {
                        ret = no_os_gpio_get_value(dev->gpio_dsa0[i], &bit);
                        if (ret != 0)
                                return ret;
                        *gain &= bit << i;
                }

        return 0;
}       

int32_t admfm2000_set_gain(struct admfm2000_dev *dev, uint8_t chan, int32_t gain)
{
        int32_t ret;
        uint8_t i;

        if (gain > ADMFM2000_MAX_GAIN || gain < ADMFM2000_MIN_GAIN)
                return -EINVAL;

        if (chan)
                for(i = 0; i < ADMFM2000_DSA_GPIOS; i++) {
                        ret = no_os_gpio_set_value(dev->gpio_dsa1[i],
                                                   gain & (1 << i));
                        if (ret != 0)
                                return ret;
                }
        else
                for(i = 0; i < ADMFM2000_DSA_GPIOS; i++) {
                        ret = no_os_gpio_set_value(dev->gpio_dsa0[i],
                                                   gain & (1 << i));
                        if (ret != 0)
                                return ret;
                }

        return 0;
}

int32_t admfm2000_set_channel_config(struct admfm2000_dev *dev, uint8_t config)
{
        int32_t ret;
        int32_t i;

        if (config > 1)
                return -EINVAL;

        if (config) {
                ret = no_os_gpio_set_value(dev->gpio_sw0[0], 0);
                ret = no_os_gpio_set_value(dev->gpio_sw0[1], 1);
                ret = no_os_gpio_set_value(dev->gpio_sw1[0], 1);
                ret = no_os_gpio_set_value(dev->gpio_sw1[1], 0);
        } else {
                ret = no_os_gpio_set_value(dev->gpio_sw0[0], 1);
                ret = no_os_gpio_set_value(dev->gpio_sw0[1], 0);
                ret = no_os_gpio_set_value(dev->gpio_sw1[0], 0);
                ret = no_os_gpio_set_value(dev->gpio_sw1[1], 1);
        }

        return ret;
}

int32_t admfm2000_init(struct admfm2000_dev **device,
                       struct admfm2000_init_param *init_param)
{
        int32_t ret;
        uint8_t i;
        struct admfm2000_dev *dev;

        dev = (struct admfm2000_dev *)no_os_calloc(1, sizeof(*dev));
        if (!dev)
                return -ENOMEM;

        for (i = 0; i < ADMFM2000_MODE_GPIOS; i++) {
                ret = no_os_gpio_get(&dev->gpio_sw0[i],
                                     init_param->gpio_sw0_param[i]);
                if (ret)
                        goto error;
        }

        for (i = 0; i < ADMFM2000_MODE_GPIOS; i++) {
                ret = no_os_gpio_get(&dev->gpio_sw1[i],
                                     init_param->gpio_sw1_param[i]);
                if (ret)
                        goto error;
        }

        for (i = 0; i < ADMFM2000_DSA_GPIOS; i++) {
                ret = no_os_gpio_get(&dev->gpio_dsa0[i],
                                     init_param->gpio_dsa0_param[i]);
                if (ret)
                        goto error;
        }

        for (i = 0; i < ADMFM2000_DSA_GPIOS; i++) {
                ret = no_os_gpio_get(&dev->gpio_dsa1[i],
                                     init_param->gpio_dsa1_param[i]);
                if (ret)
                        goto error;
        }

        ret = admfm2000_set_channel_config(dev, init_param->mixer_mode);
        if (ret)
                goto error;

        ret = admfm2000_set_gain(dev, 0, init_param->dsa_gain);
        if (ret)
                goto error;

        ret = admfm2000_set_gain(dev, 1, init_param->dsa_gain);
        if (ret)
                goto error;

        *device = dev;

        return 0;

error:
        no_os_free(dev);

        return ret;
}

int32_t admfm2000_remove(struct admfm2000_dev *dev)
{
        uint8_t i;

        for (i = 0; i < ADMFM2000_MODE_GPIOS; i++)
                no_os_gpio_remove(dev->gpio_sw0[i]);

        for (i = 0; i < ADMFM2000_MODE_GPIOS; i++)
                no_os_gpio_remove(dev->gpio_sw1[i]);

        for (i = 0; i < ADMFM2000_DSA_GPIOS; i++)
                no_os_gpio_remove(dev->gpio_dsa0[i]);

        for (i = 0; i < ADMFM2000_DSA_GPIOS; i++)
                no_os_gpio_remove(dev->gpio_dsa1[i]);

        no_os_free(dev);

        return 0;
}
