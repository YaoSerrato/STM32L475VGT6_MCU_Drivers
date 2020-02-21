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
		freq_new_HCLK = MSIfrequencies[MSIspeed];
	}
	else if((AHB_Prescaler >= RCC_AHBPRESCALER_DIV2) & (AHB_Prescaler <= RCC_AHBPRESCALER_DIV512))
	{
		freq_new_HCLK = MSIfrequencies[MSIspeed];
		freq_new_HCLK = (freq_new_HCLK) >> (AHB_Prescaler - 0x7);
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
			/* Check if this new setting is being taken into account by reading the LATENCY bits in the FLASH_ACR register */
			FLASH_SetLatency(freq_new_HCLK);

			/* Modify the CPU clock source by writing the SW bits in the RCC_CFGR register */
			/* Select MSI as SYSCLK source clock */
			RCC->RCC_CFGR &= ~(0x3 << 0);
			RCC->RCC_CFGR |= RCC_SYSCLK_MSI;

			while(((RCC->RCC_CFGR)&(0xC) >> 2) != RCC_SYSCLK_MSI);

			/* Set the AHB Prescaler */
			RCC->RCC_CFGR &= ~(0xF << 4);
			RCC->RCC_CFGR |= (AHB_Prescaler << 4);

			while(((RCC->RCC_CFGR)&(0xF << 4)) != (AHB_Prescaler << 4));

			/* Now, you can increase the CPU frequency. */
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

		}
		else
		{
			/* Decreasing frequency  */
			/* Modify the CPU clock source by writing the SW bits in the RCC_CFGR register */
			/* Select MSI as SYSCLK source clock */
			RCC->RCC_CFGR &= ~(0x3 << 0);
			RCC->RCC_CFGR |= RCC_SYSCLK_MSI;

			while(((RCC->RCC_CFGR)&(0xC) >> 2) != RCC_SYSCLK_MSI);

			/* Set the AHB Prescaler */
			RCC->RCC_CFGR &= ~(0xF << 4);
			RCC->RCC_CFGR |= (AHB_Prescaler << 4);

			while(((RCC->RCC_CFGR)&(0xF << 4)) != (AHB_Prescaler << 4));

			/* Program the wait states according to Dynamic Voltage Range selected and the new frequency */
			/* Check if this new setting is being taken into account by reading the LATENCY bits in the FLASH_ACR register */
			FLASH_SetLatency(freq_new_HCLK);

			/* Now, you can decrease the CPU frequency. */
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
		}
	}
	else
	{
		/* Frequencies are equal */
		/* Configure same frequency with new parameters (MSIspeed, MSICalibrationValue, AHB_Prescaler)  */

		/* Modify the CPU clock source by writing the SW bits in the RCC_CFGR register */
		/* Select MSI as SYSCLK source clock */
		RCC->RCC_CFGR &= ~(0x3 << 0);
		RCC->RCC_CFGR |= RCC_SYSCLK_MSI;

		while(((RCC->RCC_CFGR)&(0xC) >> 2) != RCC_SYSCLK_MSI);

		/* Set the AHB Prescaler */
		RCC->RCC_CFGR &= ~(0xF << 4);
		RCC->RCC_CFGR |= (AHB_Prescaler << 4);

		while(((RCC->RCC_CFGR)&(0xF << 4)) != (AHB_Prescaler << 4));

		/* Program the wait states according to Dynamic Voltage Range selected and the new frequency */
		/* Check if this new setting is being taken into account by reading the LATENCY bits in the FLASH_ACR register */
		FLASH_SetLatency(freq_new_HCLK);

		/* Now, you can decrease the CPU frequency. */
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

			/* Enable MSI clock source */
			SET_REG_BIT(RCC->RCC_CR, 0);			// MSION

			/* Wait for MSI clock signal to stabilize */
			while(READ_REG_BIT(RCC->RCC_CR, REG_BIT_1) == 0);		// MSIRDY

			status = RCC_STATUS_OK;
		}

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

RCC_STATUS RCC_Config_HSI(uint32_t AHB_Prescaler)
{
	RCC_STATUS status = RCC_STATUS_OK;
	uint32_t freq_new_HCLK = 0;
	uint32_t freq_current_SYSCLK = 0;
	uint32_t freq_current_HCLK = 0;

	/* Determining new desired frequency of HCLK */
	if(AHB_Prescaler == RCC_AHBPRESCALER_DIV1)
	{
		freq_new_HCLK = RCC_HSI16_VALUE;
	}
	else if((AHB_Prescaler >= RCC_AHBPRESCALER_DIV2) & (AHB_Prescaler <= RCC_AHBPRESCALER_DIV512))
	{
		freq_new_HCLK = RCC_HSI16_VALUE;
		freq_new_HCLK = (freq_new_HCLK) >> (AHB_Prescaler - 0x7);
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
			/* Check if this new setting is being taken into account by reading the LATENCY bits in the FLASH_ACR register */
			FLASH_SetLatency(freq_new_HCLK);

			/* Modify the CPU clock source by writing the SW bits in the RCC_CFGR register */
			/* Select HSI as SYSCLK source clock */
			RCC->RCC_CFGR &= ~(0x3 << 0);
			RCC->RCC_CFGR |= RCC_SYSCLK_HSI16;
			while(((RCC->RCC_CFGR)&(0xC) >> 2) != RCC_SYSCLK_HSI16);

			/* Set the AHB Prescaler */
			RCC->RCC_CFGR &= ~(0xF << 4);
			RCC->RCC_CFGR |= (AHB_Prescaler << 4);
			while(((RCC->RCC_CFGR)&(0xF << 4)) != (AHB_Prescaler << 4));

			/* Enable HSI clock source */
			SET_REG_BIT(RCC->RCC_CR, REG_BIT_8);			// HSION

			/* Wait for HSI clock signal to stabilize */
			while(READ_REG_BIT(RCC->RCC_CR, REG_BIT_10) == 0);		// HSIRDY

			status = RCC_STATUS_OK;

		}
		else
		{
			/* Decreasing frequency  */

			/* Modify the CPU clock source by writing the SW bits in the RCC_CFGR register */
			/* Select HSI as SYSCLK source clock */
			RCC->RCC_CFGR &= ~(0x3 << 0);
			RCC->RCC_CFGR |= RCC_SYSCLK_HSI16;
			while(((RCC->RCC_CFGR)&(0xC) >> 2) != RCC_SYSCLK_HSI16);

			/* Set the AHB Prescaler */
			RCC->RCC_CFGR &= ~(0xF << 4);
			RCC->RCC_CFGR |= (AHB_Prescaler << 4);
			while(((RCC->RCC_CFGR)&(0xF << 4)) != (AHB_Prescaler << 4));

			/* Program the wait states according to Dynamic Voltage Range selected and the new frequency */
			/* Check if this new setting is being taken into account by reading the LATENCY bits in the FLASH_ACR register */
			FLASH_SetLatency(freq_new_HCLK);

			/* Enable HSI clock source */
			SET_REG_BIT(RCC->RCC_CR, REG_BIT_8);			// HSION

			/* Wait for HSI clock signal to stabilize */
			while(READ_REG_BIT(RCC->RCC_CR, REG_BIT_10) == 0);		// HSIRDY

			status = RCC_STATUS_OK;
		}
	}
	else
	{
		/* Frequencies are equal */
	}

	return status;
}

RCC_STATUS RCC_Config_LSI(uint32_t LSI_Enabler)
{
	/* LSI RC can be switched on and off using the LSION (RCC_CSR) */
	if(LSI_Enabler == SET)
	{
		/* Turn on LSI */
		SET_REG_BIT(RCC->RCC_CSR, REG_BIT_0);

		/* The LSIRDY flag in the Control/status register (RCC_CSR) indicates if the LSI oscillator is ready */
		while(READ_REG_BIT(RCC->RCC_CSR, REG_BIT_1) == 0);		// HSIRDY
	}
	else
	{
		/* Turn off LSI */
		CLR_REG_BIT(RCC->RCC_CSR, REG_BIT_0);
	}

	return RCC_STATUS_OK;
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
