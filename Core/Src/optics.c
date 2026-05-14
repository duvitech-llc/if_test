#include "main.h"
#include "optics.h"
#include "util.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// #define BUFFER_SIZE 128

/* internal: basic handle validation */
static inline bool adc_handle_valid(const MCP3462_Handle *a) {
    return (a && a->hspi && a->cs_port);
}
static inline bool dac_handle_valid(const MCP4922_Handle *d) {
    return (d && d->hspi && d->cs_port);
}

static OpticsHwDesc* hw = NULL;
static uint8_t OpticsCount = 0;
static uint32_t active_optics_mask = 0;
static uint32_t active_laser_mask = 0;

/* Per-device: index into dev->channels[] for the channel currently being converted */
static uint8_t current_mux_idx[8] = {0};

/* Per-device cached MUX register bytes derived from dev->channels[] at init.
 * Sized to MAX_ADC_CHANNELS so [optic_index][i] addresses any logical channel. */
static uint8_t mux_bytes[8][MAX_ADC_CHANNELS] = {{0}};

/* Per-device map: channels[] index -> host mask bit position.
 * The host mask bit position equals the SCAN-bit position of the configured
 * source (e.g. MCP3462_SCAN_CH0_SE -> bit 0, MCP3462_SCAN_CH2_SE -> bit 2),
 * so the mask reads as the actual ADC channel selection rather than the
 * position in the enabled list. Set to 0xFF for entries with no mapping. */
static uint8_t host_bit_for_idx[8][MAX_ADC_CHANNELS] = {{0}};

/* Translate a single-bit MCP3462_ScanBits value into a per-device host mask
 * bit position (0..MAX_ADC_CHANNELS-1). Returns 0xFF if the scan bit cannot
 * be represented in the per-device 8-bit mask lane. */
static uint8_t scan_bit_to_host_bit(MCP3462_ScanBits b) {
    uint16_t v = (uint16_t)b;
    /* must be a single set bit */
    if (v == 0u || (v & (uint16_t)(v - 1u)) != 0u) return 0xFF;
    for (uint8_t i = 0; i < MAX_ADC_CHANNELS; ++i) {
        if (v == (uint16_t)(1u << i)) return i;
    }
    return 0xFF;
}

/* Weak default: links even if the product doesn’t provide a mapping */
__attribute__((weak))
OpticsHwDesc* OpticsConfig(void) {
	static OpticsHwDesc null_config = {
		.count = 0,
		.map = NULL
	};

	return &null_config;
}

HAL_StatusTypeDef optics_adcStartConversion(int optic_index) {
    if ((!hw) || (!hw->map) || (optic_index < 0) || ((uint8_t)optic_index >= OpticsCount)) {
        return HAL_ERROR;
    }

    OpticsDevice* dev = (OpticsDevice*)&hw->map[optic_index];
    if(dev->enOneshot){
    	return MCP3462_FastCommand(&dev->adc_handle, MCP3462_FC_CONV_START);
    }

    return HAL_OK;
}

HAL_StatusTypeDef optics_startLaser(int optic_index, uint16_t power) {
    if ((!hw) || (!hw->map) || (optic_index < 0) || ((uint8_t)optic_index >= OpticsCount)) {
        return HAL_ERROR;
    }

    OpticsDevice* dev = (OpticsDevice*)&hw->map[optic_index];

    dev->dacValue = (power > 100) ? 100 : power;
    dev->dacValue = (dev->dacValue * 4095) / 100;
    return MCP4922_WriteRaw(&dev->dac_handle,
    					   (MCP4922_Channel)optic_index,
                           MCP4922_BUF_OFF,
                           MCP4922_GAIN_1X,
                           MCP4922_ACTIVE,
                           dev->dacValue);
#if 0
    uint32_t fs_mV = dev->dac_handle.vref_mV;
    uint32_t out_mV = (uint32_t)dev->dacValue * fs_mV / 100u;


    return MCP4922_WritemV(&dev->dac_handle,
                           MCP4922_CH_A,
                           MCP4922_BUF_ON,
                           MCP4922_GAIN_1X,
                           MCP4922_ACTIVE,
                           out_mV);
#endif
}

HAL_StatusTypeDef optics_stopLaser(int optic_index) {
    if ((!hw) || (!hw->map) || (optic_index < 0) || ((uint8_t)optic_index >= OpticsCount)) {
        return HAL_ERROR;
    }

    OpticsDevice* dev = (OpticsDevice*)&hw->map[optic_index];
    dev->dacValue = 0;
    return MCP4922_WriteRaw(&dev->dac_handle,
    					   (MCP4922_Channel)optic_index,
                           MCP4922_BUF_OFF,
                           MCP4922_GAIN_1X,
                           MCP4922_ACTIVE,
                           0);
}

static HAL_StatusTypeDef initialize_optic_device(int optic_index) {
	HAL_StatusTypeDef st = HAL_OK;
	if ((!hw) || (!hw->map) || (optic_index < 0) || ((uint8_t)optic_index >= OpticsCount)) {
        return HAL_ERROR;
    }
    
    OpticsDevice* dev = (OpticsDevice*)&hw->map[optic_index];

    /* Validate wiring/handles before touching hardware */
    if (!adc_handle_valid(&dev->adc_handle)) {
        return HAL_ERROR;
    }
    if (!dac_handle_valid(&dev->dac_handle)) {
        return HAL_ERROR;
    }

    /* require a nonzero VREF for mV helper usage */
    if (dev->dac_handle.vref_mV == 0u) {
        return HAL_ERROR;
    }    

    /* Bring up devices */

    st  = MCP4922_Init(&dev->dac_handle);
    if (st != HAL_OK) {
    	return st;
    }

    st = MCP3462_Init(&dev->adc_handle);
    if (st != HAL_OK) {
    	return st;
    }

    /* Validate the per-device channel list and pre-compute MUX bytes */
    if (dev->channel_count == 0 || dev->channel_count > MAX_ADC_CHANNELS) {
        return HAL_ERROR;
    }
    for (uint8_t i = 0; i < MAX_ADC_CHANNELS; ++i) {
        host_bit_for_idx[optic_index][i] = 0xFF;
    }
    for (uint8_t i = 0; i < dev->channel_count; ++i) {
        uint8_t mb = MCP3462_MuxByteForScanBit(dev->channels[i]);
        if (mb == 0xFF && dev->channels[i] != MCP3462_SCAN_OFFSET) {
            return HAL_ERROR;  /* unrecognised scan bit */
        }
        mux_bytes[optic_index][i] = mb;

        uint8_t hb = scan_bit_to_host_bit(dev->channels[i]);
        if (hb >= MAX_ADC_CHANNELS) {
            /* This scan source cannot be addressed by the 8-bit per-device
             * host mask lane (e.g. differential or internal source). */
            return HAL_ERROR;
        }
        /* reject duplicate mappings within the same device */
        for (uint8_t j = 0; j < i; ++j) {
            if (host_bit_for_idx[optic_index][j] == hb) {
                return HAL_ERROR;
            }
        }
        host_bit_for_idx[optic_index][i] = hb;
    }

    /* Original-style configuration: no SCAN mode, manual MUX toggling between
     * conversions. Matches the proven working setup from before the refactor. */
    uint8_t cfg0 = 0x63;  /* internal clk, no current source, ADC in conversion */
    st = MCP3462_WriteReg(&dev->adc_handle, MCP3462_REG_CONFIG0, &cfg0, 1);
    if (st != HAL_OK) return st;

    uint8_t cfg1 = 0x08;
    st = MCP3462_WriteReg(&dev->adc_handle, MCP3462_REG_CONFIG1, &cfg1, 1);
    if (st != HAL_OK) return st;

    uint8_t irq = 0x07;
    st = MCP3462_WriteReg(&dev->adc_handle, MCP3462_REG_IRQ, &irq, 1);
    if (st != HAL_OK) return st;

    /* Start with the first channel selected */
    current_mux_idx[optic_index] = 0;
    uint8_t mux = mux_bytes[optic_index][0];
    st = MCP3462_WriteReg(&dev->adc_handle, MCP3462_REG_MUX, &mux, 1);
    if (st != HAL_OK) return st;

    /* Kick off first conversion */
    MCP3462_FastCommand(&dev->adc_handle, MCP3462_FC_CONV_START);

    /* Clear capture buffer */
	for(int i = 0; i <MAX_ADC_CHANNELS; i++){
		memset(&dev->adcSamples[i], 0, ADC_UART_BUFFER_SIZE);
		dev->dataPtr[i] = 0;
	}

    return HAL_OK;
}

HAL_StatusTypeDef optics_init() {
    
    hw = OpticsConfig();

    if (!hw || !hw->map || hw->count == 0) {
        return HAL_ERROR;
    }

    OpticsCount = hw->count;
    for (uint8_t i = 0; i < OpticsCount; ++i) {
        HAL_StatusTypeDef st = initialize_optic_device(i);
        if (st != HAL_OK) {
            return st;
        }
    }
    
    active_optics_mask = 0;
    active_laser_mask = 0;

	printf("Optics initialized with %d device(s)\r\n", OpticsCount);
    return HAL_OK;
}

void optics_clearBuffers(int optic_index)
{
	if ((!hw) || (!hw->map) || (optic_index < 0) || ((uint8_t)optic_index >= OpticsCount)) {
		return;
	}

    OpticsDevice* dev = (OpticsDevice*)&hw->map[optic_index];
    for(int i=0; i<MAX_ADC_CHANNELS; i++){
		memset(&dev->adcSamples[i], 0, ADC_UART_BUFFER_SIZE);
		dev->dataPtr[i] = 0;
    }

}

void optics_clearBuffer(int optic_index, uint8_t ch_id) {
	if ((!hw) || (!hw->map) || (optic_index < 0) || ((uint8_t)optic_index >= OpticsCount || ch_id >= MAX_ADC_CHANNELS)) {
		return;
	}

    OpticsDevice* dev = (OpticsDevice*)&hw->map[optic_index];
	memset(&dev->adcSamples[ch_id], 0, ADC_UART_BUFFER_SIZE);
	dev->dataPtr[ch_id] = 0;
}

uint16_t optics_getSize(int optic_index, uint8_t ch_id) {
	if ((!hw) || (!hw->map) || (optic_index < 0) || ((uint8_t)optic_index >= OpticsCount || ch_id >= MAX_ADC_CHANNELS)) {
		return 0;
	}

    OpticsDevice* dev = (OpticsDevice*)&hw->map[optic_index];
	return dev->dataPtr[ch_id];
}

uint8_t* optics_getBuffer(int optic_index, uint8_t ch_id) {
	if ((!hw) || (!hw->map) || (optic_index < 0) || ((uint8_t)optic_index >= OpticsCount || ch_id >= MAX_ADC_CHANNELS)) {
		return NULL;
	}

    OpticsDevice* dev = (OpticsDevice*)&hw->map[optic_index];
	return (uint8_t*)&(dev->adcSamples[ch_id]);
}

HAL_StatusTypeDef optics_adcReadSamples(int optic_index) {
	if ((!hw) || (!hw->map) || (optic_index < 0) || ((uint8_t)optic_index >= OpticsCount)) {
		return HAL_ERROR;
	}

    OpticsDevice* dev = (OpticsDevice*)&hw->map[optic_index];
    HAL_StatusTypeDef st = HAL_OK;

    uint8_t ch_id;
    int32_t code32;
    bool got_ch0 = false;
    bool got_ch1 = false;

	// In continuous mode, conversions happen automatically
	// No need to trigger, just read the data

	// In SCAN mode with 2 channels, we need to read exactly 2 samples per conversion cycle
	// Try up to 8 times to get both channels (allows for retries if data not ready)
	for(int i = 0; i < 8 && !(got_ch0 && got_ch1); i++){
		st = MCP3462_ReadScanSample(&dev->adc_handle, &ch_id, &code32);
		if (st == HAL_OK) {

			printf("  ch: %d value: 0x%04X\r\n", ch_id, (uint16_t)code32);

			// Reject CH_IDs that are out of the allocated sample array bounds.
			// The MCP3462 32_FULL format encodes CH_ID in bits [31:28] (range 0-15);
			// internal sources (temp, AVDD, VCM, offset) return IDs >= MAX_ADC_CHANNELS
			// and must not be used as array indices.
			if (ch_id >= MAX_ADC_CHANNELS) {
				continue;
			}

			// code32 now holds a signed 16-bit ADC code in its low 16 bits
			uint16_t code16 = (uint16_t)code32;

			// Store the sample pair only if there is room for both bytes
			if (dev->dataPtr[ch_id] + 2 <= ADC_UART_BUFFER_SIZE) {
				dev->adcSamples[ch_id][dev->dataPtr[ch_id]++] = (uint8_t)(code16 >> 8);
				dev->adcSamples[ch_id][dev->dataPtr[ch_id]++] = (uint8_t)(code16 & 0xFF);
			}

            if (ch_id == 0) {
				got_ch0 = true;
			} else if (ch_id == 1) {
				got_ch1 = true;
			} else {
				// other channels / internal sources, ignore for now
			}

		} else if (st != HAL_BUSY) {
			return st;
		} else {
			printf("  Read attempt %d: BUSY\r\n", i);
		}

		delay_us(1200);

	}

	return HAL_OK;
}

int optics_getDeviceCount(void) {
    return (int)OpticsCount;
}

HAL_StatusTypeDef optics_adcStart(uint32_t mask)
{
	HAL_StatusTypeDef status = HAL_OK;

    if ((!hw) || (!hw->map)) {
        return HAL_ERROR;
    }

	/* Only start devices that aren't already active */
	uint32_t new_mask = mask & ~active_optics_mask;

	/* Track which devices have been started this call to avoid duplicate CONV_STARTs */
	uint32_t started_devices = 0;

    for (uint8_t bit = 0; bit < 32; ++bit) {
        if ((new_mask & (1u << bit)) == 0) {
            continue; /* this bit not set */
        }

        uint8_t optic_index = (uint8_t)(bit >> 3);     /* bit / 8 */

        if (optic_index >= OpticsCount) {
            /* mark error but continue processing other bits */
            status = HAL_ERROR;
            break;
        }

        /* Skip device if already started during this call */
        if (started_devices & (1u << optic_index)) {
            continue;
        }

        OpticsDevice* dev = (OpticsDevice*)&hw->map[optic_index];
	    if(dev->enOneshot) // start first conversion
	    {
	    	if(MCP3462_FastCommand(&dev->adc_handle, MCP3462_FC_CONV_START) != HAL_OK)
	    	{
	    		status = HAL_ERROR;
	    	}
	    }
        started_devices |= (1u << optic_index);
    }

	/* update active mask with newly started devices */
    active_optics_mask |= mask;

	return status;
}

HAL_StatusTypeDef optics_adcStop(uint32_t mask)
{
	HAL_StatusTypeDef status = HAL_OK;
    if ((!hw) || (!hw->map)) {
        return HAL_ERROR;
    }

	/* Only stop devices that are currently active */
	uint32_t devices_to_stop = mask & active_optics_mask;

	/* Remove stopped devices from active mask */
    active_optics_mask &= ~devices_to_stop;

	// stop all devices when mask is zero
    if(active_optics_mask == 0)
    {
    	// all stopped
        for(int x=0; x<OpticsCount; x++)
        {

            OpticsDevice* dev = (OpticsDevice*)&hw->map[x];
            if(MCP3462_FastCommand(&dev->adc_handle, MCP3462_FC_STANDBY) != HAL_OK)
            {
                status = HAL_ERROR;
            }
        }
    }

	return status;
}

uint32_t optics_get_active_optics_mask()
{
	return active_optics_mask;
}

HAL_StatusTypeDef optics_adcRead()
{
	HAL_StatusTypeDef status = HAL_OK;

	if ((!hw) || (!hw->map)) {
		return HAL_ERROR;
	}

	if (active_optics_mask == 0) {
		return HAL_OK; /* nothing to read */
	}

	/* For each device: read the latest 16-bit ADCDATA, store under whichever logical
	 * channel is currently selected, then advance MUX to the next requested channel
	 * and start the next conversion. Mirrors the proven original optics_adcReadSamples. */
	uint32_t handled_devices = 0;

	for (uint8_t bit = 0; bit < 32; ++bit) {
		if ((active_optics_mask & (1u << bit)) == 0) continue;

		uint8_t optic_index = (uint8_t)(bit >> 3);
		if (optic_index >= OpticsCount) {
			status = HAL_ERROR;
			continue;
		}
		if (handled_devices & (1u << optic_index)) continue;
		handled_devices |= (1u << optic_index);

		OpticsDevice* dev = (OpticsDevice*)&hw->map[optic_index];
		uint8_t enabled_ch_mask = (uint8_t)((active_optics_mask >> (optic_index * 8)) & 0xFF);

		if (dev->channel_count == 0) continue;

		/* Which channels[] entry was just converted */
		uint8_t cur_idx = current_mux_idx[optic_index];
		if (cur_idx >= dev->channel_count) cur_idx = 0;
		uint8_t cur_host_bit = host_bit_for_idx[optic_index][cur_idx];

		/* Read 16-bit ADCDATA */
		uint8_t data[2] = {0};
		HAL_StatusTypeDef st = MCP3462_ReadReg(&dev->adc_handle, MCP3462_REG_ADCDATA, data, 2);

		if (st == HAL_OK && cur_host_bit < MAX_ADC_CHANNELS &&
		    (enabled_ch_mask & (1u << cur_host_bit))) {
			if (dev->dataPtr[cur_host_bit] + 2 <= ADC_UART_BUFFER_SIZE) {
				dev->adcSamples[cur_host_bit][dev->dataPtr[cur_host_bit]++] = data[0];
				dev->adcSamples[cur_host_bit][dev->dataPtr[cur_host_bit]++] = data[1];
			}
		} else if (st != HAL_OK) {
			status = HAL_ERROR;
		}

		/* Advance round-robin to next channels[] entry whose host bit is enabled */
		uint8_t next_idx = cur_idx;
		for (uint8_t i = 1; i <= dev->channel_count; i++) {
			uint8_t candidate = (uint8_t)((cur_idx + i) % dev->channel_count);
			uint8_t cand_host_bit = host_bit_for_idx[optic_index][candidate];
			if (cand_host_bit < MAX_ADC_CHANNELS &&
			    (enabled_ch_mask & (1u << cand_host_bit))) {
				next_idx = candidate;
				break;
			}
		}
		current_mux_idx[optic_index] = next_idx;

		/* Set MUX for next conversion and kick it off */
		uint8_t mux = mux_bytes[optic_index][next_idx];
		MCP3462_WriteReg(&dev->adc_handle, MCP3462_REG_MUX, &mux, 1);
		MCP3462_FastCommand(&dev->adc_handle, MCP3462_FC_CONV_START);
	}

	return status;
}

HAL_StatusTypeDef optics_getBuffer_byMask(uint32_t mask, uint8_t** out_buffer,  uint16_t* out_size)
{
    if((!hw) || (!hw->map) || !out_buffer || !out_size || mask == 0){
        return HAL_ERROR;
    }

    /* require exactly one bit set in mask */
    if ((mask & (mask - 1)) != 0u) return HAL_ERROR;  

	/* Find which bit is set */
	uint8_t bit = 0;
	uint32_t temp_mask = mask;
	while ((temp_mask & 1) == 0) {
		temp_mask >>= 1;
		bit++;
	}

	/* Extract optic_index and channel ID from bit position */
	uint8_t ch_id = (uint8_t)(bit & 0x7);          /* bit % 8 */
	uint8_t optic_index = (uint8_t)(bit >> 3);     /* bit / 8 */

	/* Validate that the optic_index and channel are within bounds */
	if (optic_index >= OpticsCount || ch_id >= MAX_ADC_CHANNELS) {
		return HAL_ERROR;
	}

	/* Get the device and return the buffer and size */
	OpticsDevice* dev = (OpticsDevice*)&hw->map[optic_index];
	

    *out_buffer = (uint8_t*)&(dev->adcSamples[ch_id]);
    *out_size = dev->dataPtr[ch_id];

	return HAL_OK;
}

HAL_StatusTypeDef optics_clearBuffer_byMask(uint32_t mask)
{
	HAL_StatusTypeDef status = HAL_OK;

    if ((!hw) || (!hw->map)) {
        return HAL_ERROR;
    }

    for (uint8_t bit = 0; bit < 32; ++bit) {
        if ((mask & (1u << bit)) == 0) {
            continue; /* this bit not set */
        }

        uint8_t ch_id = (uint8_t)(bit & 0x7);          /* bit % 8 */
        uint8_t optic_index = (uint8_t)(bit >> 3);     /* bit / 8 */

        if (optic_index >= OpticsCount) {
            /* mark error but continue processing other bits */
            status = HAL_ERROR;
            break;
        }

        // printf("Clear OPTICS IDX: %d  CH: %d\r\n", optic_index,  ch_id);
        /* call the provided clear function */
        OpticsDevice* dev = (OpticsDevice*)&hw->map[optic_index];
    	memset(&dev->adcSamples[ch_id], 0, ADC_UART_BUFFER_SIZE);
    	dev->dataPtr[ch_id] = 0;
    }

	return status;
}

HAL_StatusTypeDef optics_startLaser_byMask(uint32_t mask, uint16_t power) {
	HAL_StatusTypeDef status = HAL_OK;
	uint16_t set_power_level = 0;

    if ((!hw) || (!hw->map)) {
        return HAL_ERROR;
    }

	if (mask == 0) {
		return HAL_OK; /* nothing to set */
	}

    active_laser_mask |= mask;
    set_power_level = (power > 100) ? 100 : power;

	/* Iterate through all possible bits in the mask parameter (not active_laser_mask) */
	for (uint8_t bit = 0; bit < 32; ++bit) {
		if ((mask & (1u << bit)) == 0) {
			continue; /* this channel not in the requested mask */
		}

		/* Extract optic_index and channel ID from bit position */
		uint8_t ch_id = (uint8_t)(bit & 0x7);          /* bit % 8 */
		uint8_t optic_index = (uint8_t)(bit >> 3);     /* bit / 8 */

	    if (optic_index >= OpticsCount) {
	    	status = HAL_ERROR;
	    	continue;
	    }

	    OpticsDevice* dev = &hw->map[optic_index];

	    if (!dev) {
	    	status = HAL_ERROR;
	    	continue;
	    }

	    uint32_t calc = (((uint32_t)set_power_level * 4095) + 50) / 100;
	    dev->dacValue = (uint16_t)calc;

	    status = MCP4922_WriteRaw(&dev->dac_handle,
	    					   (MCP4922_Channel)ch_id,
	                           MCP4922_BUF_OFF,
	                           MCP4922_GAIN_1X,
	                           MCP4922_ACTIVE,
	                           dev->dacValue);
	}

    delay_us(100);
    return status;
}

HAL_StatusTypeDef optics_stopLaser_byMask(uint32_t mask) {
	HAL_StatusTypeDef status = HAL_OK;

    if ((!hw) || (!hw->map)) {
        return HAL_ERROR;
    }


	if (mask == 0) {
		return HAL_OK; /* nothing to set */
	}

	// Clear bits from the active laser mask
	active_laser_mask &= ~mask;

	/* Iterate through mask */
	for (uint8_t bit = 0; bit < 32; ++bit) {
		if ((mask & (1u << bit)) == 0) {
			continue; /* this channel not active */
		}

		/* Extract optic_index and channel ID from bit position */
		uint8_t ch_id = (uint8_t)(bit & 0x7);          /* bit % 8 */
		uint8_t optic_index = (uint8_t)(bit >> 3);     /* bit / 8 */
	    OpticsDevice* dev = (OpticsDevice*)&hw->map[optic_index];

	    dev->dacValue = 0;


	    status = MCP4922_WriteRaw(&dev->dac_handle,
	    					   (MCP4922_Channel)ch_id,
	                           MCP4922_BUF_OFF,
	                           MCP4922_GAIN_1X,
	                           MCP4922_ACTIVE,
	                           dev->dacValue);
	}
    return status;
}

uint32_t optics_get_active_laser_mask(void)
{
	return active_laser_mask;
}
