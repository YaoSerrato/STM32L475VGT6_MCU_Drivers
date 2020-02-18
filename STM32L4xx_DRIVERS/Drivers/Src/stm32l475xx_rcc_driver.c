/*
 * stm32l475xx_rcc_driver.c
 *
 *  Created on: Feb 13, 2020
 *      Author: H369169
 */

#include <stm32l475xx_rcc_driver.h>
#include <stm32l475xx_flash_driver.h>

uint32_t MSIfrequencies[12] = {100000U,   200000U,   400000U,   800000U,  1000000U,  2000000U, 4000000U, \
								8000000U, 16000000U, 24000000U, 32000000U, 48000000U};

RCC_STATUS RCC_Config_MSI(uint32_t MSIspeed, uint32_t MSICalibrationValue, uint32_t AHB_Prescaler)
{
	RCC_STATUS status = RCC_STATUS_OK;
	uint32_t freq_new_HCLK = 0;
	uint32_t freq_current_SYSCLK = 0;
	uint32_t freq_current_HCLK = 0;

	/* Determining new desired frequency of HCLK */
	if(AHB_Prescaler == RCC_AHBPRESCALER_DIV1)
	{
		freq_new_HCLK = MSIspeed;
	}
	else if((AHB_Prescaler >= RCC_AHBPRESCALER_DIV2) & (AHB_Prescaler <= RCC_AHBPRESCALER_DIV512))
	{
		freq_new_HCLK = (MSIspeed) >> (AHB_Prescaler - 0x7);
	}
	else
	{
		/* An invalid pre scaler was entered */
		status = RCC_STATUS_ERROR;
		return status;
	}

	/* Get current SYSCLK */
	freq_current_SYSCLK = RCC_GetSYSCLK();

	/* Get current HCLK */
	freq_current_HCLK = RCC_GetHCLK();

	/* Comparing frequencies */
	if(freq_current_HCLK != freq_new_HCLK)
	{
		/* New HCLK frequency is different from the already set */
		if(freq_current_HCLK < freq_new_HCLK)
		{
			/* Increasing frequency */
			/* Program the wait states according to Dynamic Voltage Range selected and the new frequency */
			FLASH_SetLatency(freq_new_HCLK);

			/* Check if this new setting is being taken into account by reading the LATENCY bits in the FLASH_ACR register */

			/* Modify the CPU clock source by writing the SW bits in the RCC_CFGR register */

			/* Check that the new CPU clock source or/and the new CPU clock prescaler value is/are taken into account by reading the clock source status (SWS bits) */

			/* Now, you can increase the CPU frequency. */
		}
		else
		{
			/* Decreasing frequency */
		}
	}
	else
	{
		/* Frequencies are equal */
		/* Configure same frequency with new parameters (MSIspeed, MSICalibrationValue, AHB_Prescaler)  */
	}

	if(READ_REG_BIT(RCC->RCC_CR, REG_BIT_0) == 0 || READ_REG_BIT(RCC->RCC_CR, REG_BIT_1))
	{
		/* MSIRANGE can be modified when MSI is OFF (MSION=0) or when MSI is ready (MSIRDY=1).
		 * MSIRANGE must NOT be modified when MSI is ON and NOT ready (MSION=1 and MSIRDY=0) */

		/* Configure MSI range */
		RCC->RCC_CR &= ~(0xF << 4);
		RCC->RCC_CR	|= (MSIspeed << 4);		// MSIRANGE

		/* MSI clock range selection */
		SET_REG_BIT(RCC->RCC_CR, REG_BIT_3);		// MSIRGSEL

		/* Trim/calibrate the MSI oscillator */
		RCC->RCC_ICSCR &= ~(0xFF << 8);
		RCC->RCC_ICSCR |= (MSICalibrationValue) << 8;

		/* Select MSI as SYSCLK source clock */
		RCC->RCC_CFGR &= ~(0x3 << 0);
		RCC->RCC_CFGR |= RCC_SYSCLK_MSI;

		/* Enable MSI clock source */
		SET_REG_BIT(RCC->RCC_CR, 0);			// MSION

		/* Wait for MSI clock signal to stabilize */
		while(READ_REG_BIT(RCC->RCC_CR, REG_BIT_1) == 0);		// MSIRDY

		status = RCC_STATUS_OK;
	}
	else
	{
		status = RCC_STATUS_ERROR;
	}

	/* Registers to modify */
	// RCC_CR
		// MSIPLLEN
	// RCC_ICSCR
	// RCC_CFGR
		// STOPWUCK
		// HPRE
	// RCC_CSR
		// MSISRANGE

	return status;
}

void RCC_Config_MCO(uint8_t MCOprescaler, uint8_t MCOoutput)
{
	/* MCO clock pre-scaler */
	RCC->RCC_CFGR &= ~(0x7 << 28);
	RCC->RCC_CFGR |= (MCOprescaler << 28);

	/* MCO clock selection */
	RCC->RCC_CFGR &= ~(0xF << 24);
	RCC->RCC_CFGR |= (MCOoutput << 24);
}

uint32_t RCC_GetSYSCLK(void)
{
	uint32_t SYSCLK = 0;		/* Here the System Clock will be stored */
	uint32_t system_clock = 0;
	uint32_t msi_range = 0;

	/* Read bits SWS from RCC_CFGR */
	system_clock = ((RCC->RCC_CFGR) & (0xC)) >> (2);

	switch(system_clock)
	{
		case RCC_CFGR_SWS_MSI:
			if(READ_REG_BIT(RCC->RCC_CR, REG_BIT_3) == 0x0)
			{
				/* MSI Range is provided by MSISRANGE[3:0] in RCC_CSR register  */
				msi_range = ((RCC->RCC_CSR) & (0xF00)) >> (8);
				SYSCLK = MSIfrequencies[msi_range];
			}
			else
			{
				/* MSI Range is provided by MSIRANGE[3:0] in the RCC_CR register */
				msi_range = ((RCC->RCC_CR) & (0xF0)) >> (4);
				SYSCLK = MSIfrequencies[msi_range];
			}
			break;

		case RCC_CFGR_SWS_HSI16:
			SYSCLK = RCC_HSI16_VALUE;
			break;

		case RCC_CFGR_SWS_HSE:
			SYSCLK = RCC_HSE_VALUE;
			break;

		case RCC_CFGR_SWS_PLL:
			SYSCLK = 0;
			break;

		default:
			SYSCLK = 0;
	}

	return SYSCLK;
}

uint32_t RCC_GetHCLK(void)
{
	uint32_t SYSCLK = RCC_GetSYSCLK();
	uint32_t HCLK;
	uint32_t AHBPRESC;

	AHBPRESC = ((RCC->RCC_CFGR) & (0xF0)) >> (4);

	if((AHBPRESC >= 8) & (AHBPRESC <= 15))
	{
		HCLK = (SYSCLK) >> (AHBPRESC - 0x7);
	}
	else
	{
		HCLK = SYSCLK;
	}


	return HCLK;
}
