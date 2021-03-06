/**
  *********************************************************************************************************
  * @file    nbiotfunc.c
  * @author  Movebroad -- KK
  * @version V1.0
  * @date    2018-03-03
  * @brief   
  *********************************************************************************************************
  * @attention
  *
  *
  *
  *********************************************************************************************************
  */

#include "nbiotfunc.h"
#include "platform_config.h"
#include "platform_map.h"
#include "stm32l1xx_config.h"
#include "delay.h"
#include "usart.h"
#include "hal_rtc.h"

/**********************************************************************************************************
 @Function			static void NBIOT_Neul_NBxx_DictateEvent_SetTime(NBIOT_ClientsTypeDef* pClient, unsigned int TimeoutMsec)
 @Description			NBIOT_Neul_NBxx_DictateEvent_SetTime	: 事件运行控制器注入时间(内部使用)
 @Input				pClient							: NBIOT客户端实例
					TimeoutMsec						: 注入超时时间
 @Return				void
**********************************************************************************************************/
static void NBIOT_Neul_NBxx_DictateEvent_SetTime(NBIOT_ClientsTypeDef* pClient, unsigned int TimeoutMsec)
{
	Stm32_CalculagraphTypeDef ATCmd_timer_Ms;
	
	Stm32_Calculagraph_CountdownMS(&ATCmd_timer_Ms, TimeoutMsec);
	pClient->ATCmdStack->CmdWaitTime = ATCmd_timer_Ms;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_HardwarePoweroff(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_HardwareReboot			: 硬件断电
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_HardwarePoweroff(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
	GPIO_InitTypeDef GPIO_Initure;
	
	GPIO_Initure.Pin = NBIOT_RESET_PIN;
	GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_Initure.Pull = GPIO_NOPULL;
	GPIO_Initure.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(NBIOT_RESET_GPIOx, &GPIO_Initure);
	
	NBIOTPOWER(OFF);
	HAL_GPIO_WritePin(NBIOT_RESET_GPIOx, NBIOT_RESET_PIN, GPIO_PIN_RESET);
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_HardwareReboot(NBIOT_ClientsTypeDef* pClient, u32 rebootTimeoutMS)
 @Description			NBIOT_Neul_NBxx_HardwareReboot			: 硬件重启NB模块
 @Input				pClient								: NBIOT客户端实例
					rebootTimeoutMS						: 重启等待超时时间
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_HardwareReboot(NBIOT_ClientsTypeDef* pClient, u32 rebootTimeoutMS)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
	GPIO_InitTypeDef GPIO_Initure;
	
	GPIO_Initure.Pin = NBIOT_RESET_PIN;
	GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_Initure.Pull = GPIO_NOPULL;
	GPIO_Initure.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(NBIOT_RESET_GPIOx, &GPIO_Initure);
	
	memset((void*)&NBIOTBaudRate.NBIOTBaudRateNow, 0, sizeof(NBIOTBaudRate.NBIOTBaudRateNow));
	memset((void*)&NBIOTBaudRate.NBIOTBaudRateCal, 0, sizeof(NBIOTBaudRate.NBIOTBaudRateCal));
	NBIOTBaudRate.EnBaudRateState = true;
	
	NBIOTPOWER(OFF);
	Delay_MS(2000);
	NBIOTPOWER(ON);
	Delay_MS(300);
	
	HAL_GPIO_WritePin(NBIOT_RESET_GPIOx, NBIOT_RESET_PIN, GPIO_PIN_SET);
	Delay_MS(200);
	HAL_GPIO_WritePin(NBIOT_RESET_GPIOx, NBIOT_RESET_PIN, GPIO_PIN_RESET);
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, rebootTimeoutMS);
	
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	NBStatus = pClient->ATCmdStack->Read(pClient->ATCmdStack);
	/* NBIOT串口波特率计算 */
	if (NBStatus == NBIOT_OK) {
		NBIOTBaudRate.Baud = 10 * ((NBIOTBaudRate.NBIOTBaudRateCal.TotalNum * 10000) / 
							 ((NBIOTBaudRate.NBIOTBaudRateCal.EndMs - NBIOTBaudRate.NBIOTBaudRateCal.StartMs - 1) * 10 + 
							 ((SysTick->LOAD - NBIOTBaudRate.NBIOTBaudRateCal.EndClock + NBIOTBaudRate.NBIOTBaudRateCal.StartClock) * 10) / SysTick->LOAD));
		if ((NBIOTBaudRate.Baud > 8600) && (NBIOTBaudRate.Baud < 10600)) {
			Uart1_Init(NBIOTBaudRate.Baud);
		}
	}
	NBIOTBaudRate.EnBaudRateState = false;
	
	TCFG_Utility_Add_Nbiot_BootCount();
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SoftwareReboot(NBIOT_ClientsTypeDef* pClient, u32 rebootTimeoutMS)
 @Description			NBIOT_Neul_NBxx_SoftwareReboot			: 软件重启NB模块
 @Input				pClient								: NBIOT客户端实例
					rebootTimeoutMS						: 重启等待超时时间
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SoftwareReboot(NBIOT_ClientsTypeDef* pClient, u32 rebootTimeoutMS)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, rebootTimeoutMS);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+NRB\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+NRB\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
#if NBIOT_PRINT_ERROR_CODE_TYPE
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_ERROR) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#else
	NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack);
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_ClearStoredEarfcn(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_ClearStoredEarfcn			: 清除小区频点
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_ClearStoredEarfcn(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+NCSEARFCN\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+NCSEARFCN\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
#if NBIOT_PRINT_ERROR_CODE_TYPE
	NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack);
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_ERROR) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#else
	NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack);
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadManufacturer(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadManufacturer		: 检出制造商标识
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadManufacturer(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+CGMI\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+CGMI\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		memset((void *)pClient->Parameter.manufacturer, 0x0, sizeof(pClient->Parameter.manufacturer));
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "%s\r\n", pClient->Parameter.manufacturer) <= 0) {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadManufacturerModel(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadManufacturerModel	: 检出制造商型号
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadManufacturerModel(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+CGMM\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+CGMM\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		memset((void *)pClient->Parameter.manufacturermode, 0x0, sizeof(pClient->Parameter.manufacturermode));
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "%s\r\n", pClient->Parameter.manufacturermode) <= 0) {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadModuleVersion(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadModuleVersion		: 检出模组软件版本号
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadModuleVersion(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+CGMR\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+CGMR\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		memset((void *)pClient->Parameter.modelversion, 0x0, sizeof(pClient->Parameter.modelversion));
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nSECURITY,%*11s%s\r\n", pClient->Parameter.modelversion) <= 0) {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadIMEI(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadIMEI				: 检出IMEI号
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadIMEI(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+CGSN=1\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+CGSN=1\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		memset((void *)pClient->Parameter.imei, 0x0, sizeof(pClient->Parameter.imei));
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "%*[^+]+CGSN:%[^\r]", pClient->Parameter.imei) <= 0) {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadIMEISV(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadIMEISV			: 检出IMEISV号
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadIMEISV(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+CGSN=2\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+CGSN=2\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		memset((void *)pClient->Parameter.imeisv, 0x0, sizeof(pClient->Parameter.imeisv));
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "%*[^+]+CGSN:%[^\r]", pClient->Parameter.imeisv) <= 0) {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadRSSI(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadRSSI				: 检出RSSI号
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadRSSI(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+CSQ\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+CSQ\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "%*[^+]+CSQ:%d,%*d\r\n", &pClient->Parameter.rssi) <= 0) {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadStatisticsRADIO(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadStatisticsRADIO	: 检出基站连接参数RADIO
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadStatisticsRADIO(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+NUESTATS\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+NUESTATS\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if ( sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, 
			"%*[^S]Signal power:%d%*[^T]Total power:%d%*[^T]TX power:%d%*[^T]TX time:%d%*[^R]RX time:%d%*[^C]Cell ID:%d%*[^E]ECL:%d%*[^S]SNR:%d%*[^E]EARFCN:%d%*[^P]PCI:%d%*[^R]RSRQ:%d", 
			&pClient->Parameter.statisticsRADIO.Signalpower,
			&pClient->Parameter.statisticsRADIO.Totalpower,
			&pClient->Parameter.statisticsRADIO.TXpower,
			&pClient->Parameter.statisticsRADIO.TXtime,
			&pClient->Parameter.statisticsRADIO.RXtime,
			&pClient->Parameter.statisticsRADIO.CellID,
			&pClient->Parameter.statisticsRADIO.ECL,
			&pClient->Parameter.statisticsRADIO.SNR,
			&pClient->Parameter.statisticsRADIO.EARFCN,
			&pClient->Parameter.statisticsRADIO.PCI,
			&pClient->Parameter.statisticsRADIO.RSRQ) <= 0) {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadStatisticsCELL(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadStatisticsCELL		: 检出基站连接参数CELL
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadStatisticsCELL(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+NUESTATS=CELL\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+NUESTATS=CELL\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if ( sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, 
			"\r\nNUESTATS:CELL,%d,%d,%d,%d,%d,%d,%d\r", 
			&pClient->Parameter.statisticsCELL.earfcn, 
			&pClient->Parameter.statisticsCELL.physicalcellID, 
			&pClient->Parameter.statisticsCELL.primarycell, 
			&pClient->Parameter.statisticsCELL.rsrp, 
			&pClient->Parameter.statisticsCELL.rsrq, 
			&pClient->Parameter.statisticsCELL.rssi, 
			&pClient->Parameter.statisticsCELL.snr) <= 0) {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadAreaCode(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadAreaCode			: 检出基站区域码
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadAreaCode(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+CEREG=2\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+CEREG=2\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) != NBIOT_OK) {
		goto exit;
	}
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+CEREG?\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+CEREG?\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "%*[^+]+CEREG:%*d,%*d,%hx,%x,%*d\r\n", &pClient->Parameter.networkRegStatus.tac, &pClient->Parameter.networkRegStatus.cellID) <= 0) {
			NBStatus = NBIOT_ERROR;
			goto exit;
		}
	}
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+CEREG=0\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+CEREG=0\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) != NBIOT_OK) {
		goto exit;
	}
	
exit:
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadICCID(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadICCID			: 检出ICCID卡号
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadICCID(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+NCCID\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+NCCID\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		memset((void *)pClient->Parameter.iccid, 0x0, sizeof(pClient->Parameter.iccid));
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "%*[^+]+NCCID:%[^\r]", pClient->Parameter.iccid) <= 0) {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadIMSI(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadIMSI				: 检出IMSI号
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadIMSI(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+CIMI\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+CIMI\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		memset((void *)pClient->Parameter.imsi, 0x0, sizeof(pClient->Parameter.imsi));
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\n%[^\r]", pClient->Parameter.imsi) <= 0) {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadCGPADDR(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadCGPADDR			: 检出CGPADDR地址
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadCGPADDR(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+CGPADDR\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+CGPADDR\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+CGPADDR:0,")) {
			memset((void *)pClient->Parameter.cgpaddr, 0x0, sizeof(pClient->Parameter.cgpaddr));
			if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "%*[^+]+CGPADDR:0,%[^\r]", pClient->Parameter.cgpaddr) <= 0) {
				NBStatus = NBIOT_ERROR;
			}
			else {
				NBStatus = NBIOT_OK;
			}
		}
		else {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadCGDCONT(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadCGDCONT			: 检出CGDCONT网址
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadCGDCONT(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+CGDCONT?\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+CGDCONT?\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+CGDCONT:0,\"IP\",\"")) {
			memset((void *)pClient->Parameter.cgdcont, 0x0, sizeof(pClient->Parameter.cgdcont));
			if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\n+CGDCONT:0,\"IP\",\"%[^\"]", pClient->Parameter.cgdcont) <= 0) {
				NBStatus = NBIOT_ERROR;
			}
			else {
				NBStatus = NBIOT_OK;
			}
		}
		else {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadDateTime(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadDateTime			: 检出DateTime基站时间
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadDateTime(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
	struct tm Datetime;
	int zoneTime;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+CCLK?\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+CCLK?\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if ( sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, 
			"\r\n+CCLK:%d/%d/%d,%d:%d:%d+%d\r", 
			&pClient->Parameter.dataTime.year, 
			&pClient->Parameter.dataTime.month, 
			&pClient->Parameter.dataTime.day, 
			&pClient->Parameter.dataTime.hour, 
			&pClient->Parameter.dataTime.min, 
			&pClient->Parameter.dataTime.sec, 
			&zoneTime) <= 0) {
			pClient->Parameter.dataTime.year		= 18;
			pClient->Parameter.dataTime.month		= 01;
			pClient->Parameter.dataTime.day		= 01;
			pClient->Parameter.dataTime.hour		= 00;
			pClient->Parameter.dataTime.min		= 00;
			pClient->Parameter.dataTime.sec		= 00;
			NBStatus = NBIOT_ERROR;
		}
		else {
			pClient->Parameter.dataTime.localstamp = RTC_TimeToStamp(pClient->Parameter.dataTime.year, 
														  pClient->Parameter.dataTime.month, 
														  pClient->Parameter.dataTime.day, 
														  pClient->Parameter.dataTime.hour, 
														  pClient->Parameter.dataTime.min, 
														  pClient->Parameter.dataTime.sec) + (zoneTime * 15 * 60);
			Datetime = RTC_ConvUnixToCalendar(pClient->Parameter.dataTime.localstamp);
			pClient->Parameter.dataTime.year		= Datetime.tm_year;
			pClient->Parameter.dataTime.month		= Datetime.tm_mon;
			pClient->Parameter.dataTime.day		= Datetime.tm_mday;
			pClient->Parameter.dataTime.hour		= Datetime.tm_hour;
			pClient->Parameter.dataTime.min		= Datetime.tm_min;
			pClient->Parameter.dataTime.sec		= Datetime.tm_sec;
			NBStatus = NBIOT_OK;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadPowerSavingModeStatus(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadPowerSavingModeStatus	: 查询终端PSM状态
 @Input				pClient									: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef							: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadPowerSavingModeStatus(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+NPSMR=1\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+NPSMR=1\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) != NBIOT_OK) {
		goto exit;
	}
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+NPSMR?\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+NPSMR?\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NPSMR:1,1")) {
			pClient->Parameter.psmstate = PowerSavingMode;
		}
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NPSMR:1,0")) {
			pClient->Parameter.psmstate = NormalMode;
		}
		else {
			NBStatus = NBIOT_ERROR;
			goto exit;
		}
	}
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+NPSMR=0\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+NPSMR=0\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) != NBIOT_OK) {
		goto exit;
	}
	
exit:
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadSignalConnectionStatus(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadSignalConnectionStatus	: 查询终端Connected状态
 @Input				pClient									: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef							: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadSignalConnectionStatus(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+CSCON?\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+CSCON?\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		/* Connected */
		if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+CSCON:0,1")) {
			pClient->Parameter.connectedstate = ConnectedMode;
		}
		/* Idle */
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+CSCON:0,0")) {
			pClient->Parameter.connectedstate = IdleMode;
		}
		else {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadMessageRegistrationStatus(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadMessageRegistrationStatus: 查询终端MessageRegistration状态
 @Input				pClient									: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef							: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadMessageRegistrationStatus(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+NMSTATUS?\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+NMSTATUS?\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		/* UNINITIALISED */
		if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NMSTATUS:UNINITIALISED")) {
			pClient->Parameter.messageregistration = UNINITIALISED;
		}
		/* MISSING_CONFIG */
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NMSTATUS:MISSING_CONFIG")) {
			pClient->Parameter.messageregistration = MISSING_CONFIG;
		}
		/* INIT_FAILED */
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NMSTATUS:INIT_FAILED")) {
			pClient->Parameter.messageregistration = INIT_FAILED;
		}
		/* INIITIALISED */
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NMSTATUS:INIITIALISED")) {
			pClient->Parameter.messageregistration = INIITIALISED;
		}
		/* REGISTERING */
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NMSTATUS:REGISTERING")) {
			pClient->Parameter.messageregistration = REGISTERING;
		}
		/* REREGISTERING */
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NMSTATUS:REREGISTERING")) {
			pClient->Parameter.messageregistration = REREGISTERING;
		}
		/* REGISTERED */
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NMSTATUS:REGISTERED")) {
			pClient->Parameter.messageregistration = REGISTERED;
		}
		/* REREGISTERED */
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NMSTATUS:REREGISTERED")) {
			pClient->Parameter.messageregistration = REREGISTERED;
		}
		/* MO_DATA_ENABLED */
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NMSTATUS:MO_DATA_ENABLED")) {
			pClient->Parameter.messageregistration = MO_DATA_ENABLED;
		}
		/* NO_UE_IP */
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NMSTATUS:NO_UE_IP")) {
			pClient->Parameter.messageregistration = NO_UE_IP;
		}
		/* MEMORY_ERROR */
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NMSTATUS:MEMORY_ERROR")) {
			pClient->Parameter.messageregistration = MEMORY_ERROR;
		}
		/* COAP_ERROR */
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NMSTATUS:COAP_ERROR")) {
			pClient->Parameter.messageregistration = COAP_ERROR;
		}
		/* MSG_SEND_FAILED */
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NMSTATUS:MSG_SEND_FAILED")) {
			pClient->Parameter.messageregistration = MSG_SEND_FAILED;
		}
		/* REJECTED_BY_SERVER */
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NMSTATUS:REJECTED_BY_SERVER")) {
			pClient->Parameter.messageregistration = REJECTED_BY_SERVER;
		}
		/* TIMEOUT_AND_RETRYING */
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NMSTATUS:TIMEOUT_AND_RETRYING")) {
			pClient->Parameter.messageregistration = TIMEOUT_AND_RETRYING;
		}
		/* TIMEOUT_AND_FAILED */
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NMSTATUS:TIMEOUT_AND_FAILED")) {
			pClient->Parameter.messageregistration = TIMEOUT_AND_FAILED;
		}
		else {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SetAttachOrDetach(NBIOT_ClientsTypeDef* pClient, NBIOT_NetstateTypeDef attdet)
 @Description			NBIOT_Neul_NBxx_SetAttachOrDetach			: 设置终端入网退网
 @Input				pClient								: NBIOT客户端实例
					attdet								: Detach / Attach
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SetAttachOrDetach(NBIOT_ClientsTypeDef* pClient, NBIOT_NetstateTypeDef attdet)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	if (attdet == Detach) {												//退网
		sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+CGATT=0\r");
		pClient->ATCmdStack->ATSendlen = strlen("AT+CGATT=0\r");
	}
	else if (attdet == Attach) {											//入网
		sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+CGATT=1\r");
		pClient->ATCmdStack->ATSendlen = strlen("AT+CGATT=1\r");
	}
	else {
		NBStatus = NBIOT_ERROR;
	}
	
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
#if NBIOT_PRINT_ERROR_CODE_TYPE
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_ERROR) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#else
	NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack);
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadAttachOrDetach(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadAttachOrDetach		: 查询终端入网退网
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadAttachOrDetach(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+CGATT?\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+CGATT?\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+CGATT:1")) {
			pClient->Parameter.netstate = Attach;
		}
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+CGATT:0")) {
			pClient->Parameter.netstate = Detach;
		}
		else {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SetMinOrFullFunc(NBIOT_ClientsTypeDef* pClient, NBIOT_FunctionalityTypeDef minfull)
 @Description			NBIOT_Neul_NBxx_SetMinOrFullFunc			: 设置设备功能
 @Input				pClient								: NBIOT客户端实例
					minfull								: MinFunc / FullFunc
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SetMinOrFullFunc(NBIOT_ClientsTypeDef* pClient, NBIOT_FunctionalityTypeDef minfull)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	if (minfull == MinFunc) {											//最小功能
		sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+CFUN=0\r");
		pClient->ATCmdStack->ATSendlen = strlen("AT+CFUN=0\r");
	}
	else if (minfull == FullFunc) {										//全功能
		sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+CFUN=1\r");
		pClient->ATCmdStack->ATSendlen = strlen("AT+CFUN=1\r");
	}
	else {
		NBStatus = NBIOT_ERROR;
		goto exit;
	}
	
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
#if NBIOT_PRINT_ERROR_CODE_TYPE
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_ERROR) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#else
	NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack);
#endif
	
exit:
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadMinOrFullFunc(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadMinOrFullFunc		: 查询设备功能
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadMinOrFullFunc(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+CFUN?\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+CFUN?\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+CFUN:1")) {
			pClient->Parameter.functionality = FullFunc;
		}
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+CFUN:0")) {
			pClient->Parameter.functionality = MinFunc;
		}
		else {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SetNewMessageIndications(NBIOT_ClientsTypeDef* pClient, NBIOT_OpenOrCloseFuncTypeDef state)
 @Description			NBIOT_Neul_NBxx_SetNewMessageIndications	: 设置NNMI功能是否开启
 @Input				pClient								: NBIOT客户端实例
					state								: CloseFunc / OpenFunc
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SetNewMessageIndications(NBIOT_ClientsTypeDef* pClient, NBIOT_OpenOrCloseFuncTypeDef state)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	if (state == CloseFunc) {											//关闭
		sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+NNMI=0\r");
		pClient->ATCmdStack->ATSendlen = strlen("AT+NNMI=0\r");
	}
	else if (state == OpenFunc) {											//开启
		sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+NNMI=1\r");
		pClient->ATCmdStack->ATSendlen = strlen("AT+NNMI=1\r");
	}
	else {
		NBStatus = NBIOT_ERROR;
		goto exit;
	}
	
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
#if NBIOT_PRINT_ERROR_CODE_TYPE
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_ERROR) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#else
	NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack);
#endif
	
exit:
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadNewMessageIndications(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadNewMessageIndications	: 查询NNMI功能是否开启
 @Input				pClient									: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef							: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadNewMessageIndications(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+NNMI?\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+NNMI?\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NNMI:1")) {
			pClient->Parameter.nnmistate = OpenFunc;
		}
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NNMI:0")) {
			pClient->Parameter.nnmistate = CloseFunc;
		}
		else {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SetSentMessageIndications(NBIOT_ClientsTypeDef* pClient, NBIOT_OpenOrCloseFuncTypeDef state)
 @Description			NBIOT_Neul_NBxx_SetSentMessageIndications	: 设置NSMI功能是否开启
 @Input				pClient								: NBIOT客户端实例
					state								: CloseFunc / OpenFunc
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SetSentMessageIndications(NBIOT_ClientsTypeDef* pClient, NBIOT_OpenOrCloseFuncTypeDef state)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	if (state == CloseFunc) {											//关闭
		sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+NSMI=0\r");
		pClient->ATCmdStack->ATSendlen = strlen("AT+NSMI=0\r");
	}
	else if (state == OpenFunc) {											//开启
		sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+NSMI=1\r");
		pClient->ATCmdStack->ATSendlen = strlen("AT+NSMI=1\r");
	}
	else {
		NBStatus = NBIOT_ERROR;
		goto exit;
	}
	
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
#if NBIOT_PRINT_ERROR_CODE_TYPE
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_ERROR) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#else
	NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack);
#endif
	
exit:
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadSentMessageIndications(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadSentMessageIndications	: 查询NSMI功能是否开启
 @Input				pClient									: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef							: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadSentMessageIndications(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+NSMI?\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+NSMI?\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NSMI:1")) {
			pClient->Parameter.nsmistate = OpenFunc;
		}
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NSMI:0")) {
			pClient->Parameter.nsmistate = CloseFunc;
		}
		else {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SetSupportedBands(NBIOT_ClientsTypeDef* pClient, NBIOT_BandTypeDef bands)
 @Description			NBIOT_Neul_NBxx_SetSupportedBands			: 设置Band
 @Input				pClient								: NBIOT客户端实例
					bands								: ChinaTelecom						= 5
														  ChinaMobile						= 8
														  ChinaUnicom						= 20
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SetSupportedBands(NBIOT_ClientsTypeDef* pClient, NBIOT_BandTypeDef bands)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	memset((void *)pClient->DataProcessStack, 0x0, pClient->DataProcessStack_size);
	sprintf((char *)pClient->DataProcessStack, "AT+NBAND=%d\r", bands);
	
	memcpy(pClient->ATCmdStack->ATSendbuf, pClient->DataProcessStack, strlen((char *)pClient->DataProcessStack));
	pClient->ATCmdStack->ATSendlen = strlen((char *)pClient->DataProcessStack);
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
#if NBIOT_PRINT_ERROR_CODE_TYPE
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_ERROR) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#else
	NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack);
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadSupportedBands(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadSupportedBands		: 查询Band
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadSupportedBands(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
	int bands = 0;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+NBAND?\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+NBAND?\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\n+NBAND:%d\r", &bands) <= 0) {
			NBStatus = NBIOT_ERROR;
		}
		else {
			if (bands == ChinaTelecom) {
				pClient->Parameter.band = ChinaTelecom;
			}
			else if (bands == ChinaMobile) {
				pClient->Parameter.band = ChinaMobile;
			}
			else if (bands == ChinaUnicom) {
				pClient->Parameter.band = ChinaUnicom;
			}
			else {
				NBStatus = NBIOT_ERROR;
			}
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SetReportTerminationError(NBIOT_ClientsTypeDef* pClient, NBIOT_ReportErrorTypeDef enable)
 @Description			NBIOT_Neul_NBxx_SetReportTerminationError	: 设置CMEE
 @Input				pClient								: NBIOT客户端实例
					enable								: CMEDisable						= 0
														  CMEEnable						= 1
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SetReportTerminationError(NBIOT_ClientsTypeDef* pClient, NBIOT_ReportErrorTypeDef enable)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	memset((void *)pClient->DataProcessStack, 0x0, pClient->DataProcessStack_size);
	sprintf((char *)pClient->DataProcessStack, "AT+CMEE=%d\r", enable);
	
	memcpy(pClient->ATCmdStack->ATSendbuf, pClient->DataProcessStack, strlen((char *)pClient->DataProcessStack));
	pClient->ATCmdStack->ATSendlen = strlen((char *)pClient->DataProcessStack);
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack);
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadReportTerminationError(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadReportTerminationError	: 查询CMEE
 @Input				pClient									: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef							: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadReportTerminationError(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
	int enable = 0;
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+CMEE?\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+CMEE?\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\n+CMEE:%d\r", &enable) <= 0) {
			NBStatus = NBIOT_ERROR;
		}
		else {
			if (enable == CMEDisable) {
				pClient->Parameter.cmeestate = CMEDisable;
			}
			else if (enable == CMEEnable) {
				pClient->Parameter.cmeestate = CMEEnable;
			}
			else {
				NBStatus = NBIOT_ERROR;
			}
		}
	}
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SetCDPServer(NBIOT_ClientsTypeDef* pClient, const char *host, unsigned short port)
 @Description			NBIOT_Neul_NBxx_SetCDPServer				: 设置CDP服务器
 @Input				pClient								: NBIOT客户端实例
					host									: CDP Server Address
					port									: CDP Server Port
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SetCDPServer(NBIOT_ClientsTypeDef* pClient, const char *host, unsigned short port)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	memset((void *)pClient->DataProcessStack, 0x0, pClient->DataProcessStack_size);
	sprintf((char *)pClient->DataProcessStack, "AT+NCDP=%s,%d\r", host, port);
	
	memcpy(pClient->ATCmdStack->ATSendbuf, pClient->DataProcessStack, strlen((char *)pClient->DataProcessStack));
	pClient->ATCmdStack->ATSendlen = strlen((char *)pClient->DataProcessStack);
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack);
#if NBIOT_PRINT_ERROR_CODE_TYPE
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_ERROR) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#else
	NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack);
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadCDPServer(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadCDPServer			: 查询CDP服务器
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadCDPServer(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	memset((void *)&pClient->Parameter.cdpserver, 0x0, sizeof(pClient->Parameter.cdpserver));
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+NCDP?\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+NCDP?\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\n+NCDP:%[^,],%hu\r", (char *)&pClient->Parameter.cdpserver.CDPServerHost, &pClient->Parameter.cdpserver.CDPServerPort) <= 0) {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SetConfigUE(NBIOT_ClientsTypeDef* pClient, const char *ncmd, NBIOT_NConfigTypeDef state)
 @Description			NBIOT_Neul_NBxx_SetConfigUE				: 配置NCONFIG模式
 @Input				pClient								: NBIOT客户端实例
					ncmd									: Neul配置项
					state								: Neul配置模式
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SetConfigUE(NBIOT_ClientsTypeDef* pClient, const char *ncmd, NBIOT_NConfigTypeDef state)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	memset((void *)pClient->DataProcessStack, 0x0, pClient->DataProcessStack_size);
	sprintf((char *)pClient->DataProcessStack, "AT+NCONFIG=%s,%s\r", ncmd, (state == NConfigTrue)?"TRUE":"FALSE");
	
	memcpy(pClient->ATCmdStack->ATSendbuf, pClient->DataProcessStack, strlen((char *)pClient->DataProcessStack));
	pClient->ATCmdStack->ATSendlen = strlen((char *)pClient->DataProcessStack);
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
#if NBIOT_PRINT_ERROR_CODE_TYPE
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_ERROR) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#else
	NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack);
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadConfigUE(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadConfigUE			: 查询NCONFIG模式
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadConfigUE(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+NCONFIG?\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+NCONFIG?\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NCONFIG:AUTOCONNECT,TRUE")) {
			pClient->Parameter.nconfig.autoConnect = NConfigTrue;
		}
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NCONFIG:AUTOCONNECT,FALSE")) {
			pClient->Parameter.nconfig.autoConnect = NConfigFalse;
		}
		else {
			NBStatus = NBIOT_ERROR;
			goto exit;
		}
		
		if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NCONFIG:CR_0354_0338_SCRAMBLING,TRUE")) {
			pClient->Parameter.nconfig.crScrambling = NConfigTrue;
		}
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NCONFIG:CR_0354_0338_SCRAMBLING,FALSE")) {
			pClient->Parameter.nconfig.crScrambling = NConfigFalse;
		}
		else {
			NBStatus = NBIOT_ERROR;
			goto exit;
		}
		
		if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NCONFIG:CR_0859_SI_AVOID,TRUE")) {
			pClient->Parameter.nconfig.crSiAvoid = NConfigTrue;
		}
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NCONFIG:CR_0859_SI_AVOID,FALSE")) {
			pClient->Parameter.nconfig.crSiAvoid = NConfigFalse;
		}
		else {
			NBStatus = NBIOT_ERROR;
			goto exit;
		}
		
		if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NCONFIG:COMBINE_ATTACH,TRUE")) {
			pClient->Parameter.nconfig.combineAttach = NConfigTrue;
		}
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NCONFIG:COMBINE_ATTACH,FALSE")) {
			pClient->Parameter.nconfig.combineAttach = NConfigFalse;
		}
		else {
			NBStatus = NBIOT_ERROR;
			goto exit;
		}
		
		if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NCONFIG:CELL_RESELECTION,TRUE")) {
			pClient->Parameter.nconfig.cellReselection = NConfigTrue;
		}
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NCONFIG:CELL_RESELECTION,FALSE")) {
			pClient->Parameter.nconfig.cellReselection = NConfigFalse;
		}
		else {
			NBStatus = NBIOT_ERROR;
			goto exit;
		}
		
		if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NCONFIG:ENABLE_BIP,TRUE")) {
			pClient->Parameter.nconfig.enableBip = NConfigTrue;
		}
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+NCONFIG:ENABLE_BIP,FALSE")) {
			pClient->Parameter.nconfig.enableBip = NConfigFalse;
		}
		else {
			NBStatus = NBIOT_ERROR;
			goto exit;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
exit:
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_QuerySendMessageCOAPPayload(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_QuerySendMessageCOAPPayload	: COAP查询消息已送达NB
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_QuerySendMessageCOAPPayload(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+NQMGS\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+NQMGS\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = NULL;
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nPENDING=%d,SENT=%d,ERROR=%d\r", &pClient->Parameter.coapSendMessage.pending,
																				   &pClient->Parameter.coapSendMessage.sent,
																				   &pClient->Parameter.coapSendMessage.error) <= 0) {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_QueryReadMessageCOAPPayload(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_QueryReadMessageCOAPPayload	: COAP查询消息是否下行
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_QueryReadMessageCOAPPayload(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+NQMGR\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+NQMGR\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nBUFFERED=%d,RECEIVED=%d,DROPPED=%d\r", &pClient->Parameter.coapReadMessage.buffered,
																						&pClient->Parameter.coapReadMessage.received,
																						&pClient->Parameter.coapReadMessage.dropped) <= 0) {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SendCOAPPayload(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_SendCOAPPayload			: COAP发送一条负载数据
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
 @attention			必须注网成功才可发送负载数据
					最大有效数据长度为512字节
					每次只能缓存一条消息
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SendCOAPPayload(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
	u16 length = 0;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	if ((pClient->Sendlen > pClient->Sendbuf_size) || (((2 * pClient->Sendlen) + 15) > pClient->DataProcessStack_size) || (((2 * pClient->Sendlen) + 15) > pClient->ATCmdStack->ATSendbuf_size)) {
		NBStatus = NBIOT_ERROR;
		goto exit;
	}
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	memset((void *)pClient->DataProcessStack, 0x0, pClient->DataProcessStack_size);
	sprintf((char *)pClient->DataProcessStack, "AT+NMGS=%d,", pClient->Sendlen);
	length = strlen((const char*)pClient->DataProcessStack);
	for (int i = 0; i < pClient->Sendlen; i++) {
		sprintf((char *)(pClient->DataProcessStack + length + i * 2), "%02X", pClient->Sendbuf[i]);
	}
	length = length + pClient->Sendlen * 2;
	sprintf((char *)(pClient->DataProcessStack + length), "%c", '\r');
	
	memcpy(pClient->ATCmdStack->ATSendbuf, pClient->DataProcessStack, strlen((char *)pClient->DataProcessStack));
	pClient->ATCmdStack->ATSendlen = strlen((char *)pClient->DataProcessStack);
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
#if NBIOT_PRINT_ERROR_CODE_TYPE
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_ERROR) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#else
	NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack);
#endif
	
exit:
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_ReadCOAPPayload(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_ReadCOAPPayload			: COAP读取一条负载数据
 @Input				pClient								: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_ReadCOAPPayload(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
	u32 utmp = 0;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	memset((void *)pClient->DataProcessStack, 0x0, pClient->DataProcessStack_size);
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+NMGR\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+NMGR\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\n%hd,%s\r", &pClient->Recvlen, pClient->DataProcessStack) <= 0) {
			NBStatus = NBIOT_ERROR;
			goto exit;
		}
		memset((void *)pClient->Recvbuf, 0x0, pClient->Recvbuf_size);
		for (int i = 0; i < pClient->Recvlen; i++) {
			sscanf((const char*)(pClient->DataProcessStack + i * 2), "%02X", &utmp);
			pClient->Recvbuf[i] = utmp;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
exit:
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadCONDataStatus(NBIOT_ClientsTypeDef* pClient)
 @Description			NBIOT_Neul_NBxx_CheckReadCONDataStatus			: 查询终端CON数据发送状态
 @Input				pClient									: NBIOT客户端实例
 @Return				NBIOT_StatusTypeDef							: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CheckReadCONDataStatus(NBIOT_ClientsTypeDef* pClient)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	sprintf((char *)pClient->ATCmdStack->ATSendbuf, "AT+MLWULDATASTATUS?\r");
	pClient->ATCmdStack->ATSendlen = strlen("AT+MLWULDATASTATUS?\r");
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		/* Unsent */
		if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+MLWULDATASTATUS:0")) {
			pClient->Parameter.condatastate = Unsent;
		}
		/* WaitResponse */
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+MLWULDATASTATUS:1")) {
			pClient->Parameter.condatastate = WaitResponse;
		}
		/* SendFail */
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+MLWULDATASTATUS:2")) {
			pClient->Parameter.condatastate = SendFail;
		}
		/* SendTimeout */
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+MLWULDATASTATUS:3")) {
			pClient->Parameter.condatastate = SendTimeout;
		}
		/* SendSussess */
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+MLWULDATASTATUS:4")) {
			pClient->Parameter.condatastate = SendSussess;
		}
		/* RecvRST */
		else if (strstr((const char*)pClient->ATCmdStack->ATRecvbuf, "+MLWULDATASTATUS:5")) {
			pClient->Parameter.condatastate = RecvRST;
		}
		else {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SendCOAPPayloadFlag(NBIOT_ClientsTypeDef* pClient, const char *flag)
 @Description			NBIOT_Neul_NBxx_SendCOAPPayloadFlag		: COAP发送一条负载数据(FLAG)
 @Input				pClient								: NBIOT客户端实例
					flag							0x0000	: NON 一般
												0x0001	: NON 立马 IDLE
												0x0100	: CON 一般
												0x0101	: CON 立马 IDLE
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
 @attention			必须注网成功才可发送负载数据
					最大有效数据长度为512字节
					每次只能缓存一条消息
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SendCOAPPayloadFlag(NBIOT_ClientsTypeDef* pClient, const char *flag)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
	u16 length = 0;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	if ((pClient->Sendlen > pClient->Sendbuf_size) || (((2 * pClient->Sendlen) + 29) > pClient->DataProcessStack_size) || (((2 * pClient->Sendlen) + 29) > pClient->ATCmdStack->ATSendbuf_size)) {
		NBStatus = NBIOT_ERROR;
		goto exit;
	}
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	memset((void *)pClient->DataProcessStack, 0x0, pClient->DataProcessStack_size);
	sprintf((char *)pClient->DataProcessStack, "AT+MLWULDATAEX=%d,", pClient->Sendlen);
	length = strlen((const char*)pClient->DataProcessStack);
	for (int i = 0; i < pClient->Sendlen; i++) {
		sprintf((char *)(pClient->DataProcessStack + length + i * 2), "%02X", pClient->Sendbuf[i]);
	}
	length = length + pClient->Sendlen * 2;
	sprintf((char *)(pClient->DataProcessStack + length), "%c", ',');
	length = length + 1;
	sprintf((char *)(pClient->DataProcessStack + length), "%s", flag);
	length = length + strlen(flag);
	sprintf((char *)(pClient->DataProcessStack + length), "%c", '\r');
	
	memcpy(pClient->ATCmdStack->ATSendbuf, pClient->DataProcessStack, strlen((char *)pClient->DataProcessStack));
	pClient->ATCmdStack->ATSendlen = strlen((char *)pClient->DataProcessStack);
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
#if NBIOT_PRINT_ERROR_CODE_TYPE
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_ERROR) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#else
	NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack);
#endif
	
exit:
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CreateUDPSocket(NBIOT_ClientsTypeDef* pClient, u16 localport, u8 receivectl, int *socket)
 @Description			NBIOT_Neul_NBxx_CreateUDPSocket			: 创建UDP链接
 @Input				pClient								: NBIOT客户端实例
					localport								: 0 ~ 65535
					receivectl							: 1 is Enable Receive 0 is Ignored Receive
					socket								: Created Socket Num
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CreateUDPSocket(NBIOT_ClientsTypeDef* pClient, u16 localport, u8 receivectl, int *socket)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	memset((void *)pClient->DataProcessStack, 0x0, pClient->DataProcessStack_size);
	sprintf((char *)pClient->DataProcessStack, "AT+NSOCR=%s,%d,%d,%d\r", "DGRAM", 17, localport, receivectl);
	
	memcpy(pClient->ATCmdStack->ATSendbuf, pClient->DataProcessStack, strlen((char *)pClient->DataProcessStack));
	pClient->ATCmdStack->ATSendlen = strlen((char *)pClient->DataProcessStack);
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\n%d\r\n", socket) <= 0) {
			NBStatus = NBIOT_ERROR;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CloseUDPSocket(NBIOT_ClientsTypeDef* pClient, int socket)
 @Description			NBIOT_Neul_NBxx_CloseUDPSocket			: 关闭UDP连接
 @Input				pClient								: NBIOT客户端实例
					socket								: Closeed Socket Num
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_CloseUDPSocket(NBIOT_ClientsTypeDef* pClient, int socket)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	memset((void *)pClient->DataProcessStack, 0x0, pClient->DataProcessStack_size);
	sprintf((char *)pClient->DataProcessStack, "AT+NSOCL=%d\r", socket);
	
	memcpy(pClient->ATCmdStack->ATSendbuf, pClient->DataProcessStack, strlen((char *)pClient->DataProcessStack));
	pClient->ATCmdStack->ATSendlen = strlen((char *)pClient->DataProcessStack);
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
#if NBIOT_PRINT_ERROR_CODE_TYPE
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_ERROR) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#else
	NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack);
#endif
	
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SendUDPPayload(NBIOT_ClientsTypeDef* pClient, int socket, const char *addr, u16 port)
 @Description			NBIOT_Neul_NBxx_SendUDPPayload			: UDP发送负载数据
 @Input				pClient								: NBIOT客户端实例
					socket								: Socket Num
					addr									: 目标IP
					port									: 目标端口
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SendUDPPayload(NBIOT_ClientsTypeDef* pClient, int socket, const char *addr, u16 port)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
	u16 length = 0;
	u16 rdsocket = 0;
	u16 rdlength = 0;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	if ((pClient->Sendlen > pClient->Sendbuf_size) || (((2 * pClient->Sendlen) + 15) > pClient->DataProcessStack_size) || (((2 * pClient->Sendlen) + 15) > pClient->ATCmdStack->ATSendbuf_size)) {
		NBStatus = NBIOT_ERROR;
		goto exit;
	}
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	memset((void *)pClient->DataProcessStack, 0x0, pClient->DataProcessStack_size);
	sprintf((char *)pClient->DataProcessStack, "AT+NSOST=%d,%s,%d,%d,", socket, addr, port, pClient->Sendlen);
	length = strlen((const char*)pClient->DataProcessStack);
	for (int i = 0; i < pClient->Sendlen; i++) {
		sprintf((char *)(pClient->DataProcessStack + length + i * 2), "%02X", pClient->Sendbuf[i]);
	}
	length = length + pClient->Sendlen * 2;
	sprintf((char *)(pClient->DataProcessStack + length), "%c", '\r');
	
	memcpy(pClient->ATCmdStack->ATSendbuf, pClient->DataProcessStack, strlen((char *)pClient->DataProcessStack));
	pClient->ATCmdStack->ATSendlen = strlen((char *)pClient->DataProcessStack);
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\n%hu,%hu\r\n", &rdsocket, &rdlength) <= 0) {
			NBStatus = NBIOT_ERROR;
		}
		else {
			if ((rdsocket != socket) || (rdlength != pClient->Sendlen)) {
				NBStatus = NBIOT_ERROR;
				goto exit;
			}
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
exit:
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SendUDPPayloadFlag(NBIOT_ClientsTypeDef* pClient, int socket, const char *addr, u16 port, const char *flag)
 @Description			NBIOT_Neul_NBxx_SendUDPPayloadFlag			: UDP发送负载数据(携带标志)
 @Input				pClient								: NBIOT客户端实例
					socket								: Socket Num
					addr									: 目标IP
					port									: 目标端口
					flag							0x100	: 发送高优先级消息
												0x200	: 发送成功后马上进入IDLE
												0x400	: 发送数据得到回复后进入IDLE
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_SendUDPPayloadFlag(NBIOT_ClientsTypeDef* pClient, int socket, const char *addr, u16 port, const char *flag)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
	u16 length = 0;
	u16 rdsocket = 0;
	u16 rdlength = 0;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	if ((pClient->Sendlen > pClient->Sendbuf_size) || (((2 * pClient->Sendlen) + 15) > pClient->DataProcessStack_size) || (((2 * pClient->Sendlen) + 15) > pClient->ATCmdStack->ATSendbuf_size)) {
		NBStatus = NBIOT_ERROR;
		goto exit;
	}
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	memset((void *)pClient->DataProcessStack, 0x0, pClient->DataProcessStack_size);
	sprintf((char *)pClient->DataProcessStack, "AT+NSOSTF=%d,%s,%d,%s,%d,", socket, addr, port, flag, pClient->Sendlen);
	length = strlen((const char*)pClient->DataProcessStack);
	for (int i = 0; i < pClient->Sendlen; i++) {
		sprintf((char *)(pClient->DataProcessStack + length + i * 2), "%02X", pClient->Sendbuf[i]);
	}
	length = length + pClient->Sendlen * 2;
	sprintf((char *)(pClient->DataProcessStack + length), "%c", '\r');
	
	memcpy(pClient->ATCmdStack->ATSendbuf, pClient->DataProcessStack, strlen((char *)pClient->DataProcessStack));
	pClient->ATCmdStack->ATSendlen = strlen((char *)pClient->DataProcessStack);
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\n%hu,%hu\r\n", &rdsocket, &rdlength) <= 0) {
			NBStatus = NBIOT_ERROR;
		}
		else {
			if ((rdsocket != socket) || (rdlength != pClient->Sendlen)) {
				NBStatus = NBIOT_ERROR;
				goto exit;
			}
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
exit:
	return NBStatus;
}

/**********************************************************************************************************
 @Function			NBIOT_StatusTypeDef NBIOT_Neul_NBxx_ReadUDPPayload(NBIOT_ClientsTypeDef* pClient, int socket, int maxrlen, int *rleft, char *addr, u16 *port)
 @Description			NBIOT_Neul_NBxx_ReadUDPPayload			: UDP读取负载数据
 @Input				pClient								: NBIOT客户端实例
					socket								: Socket Num
					maxrlen								: 最大读取数据大小
					rleft								: 剩余读取数据长度
					addr									: 目标IP
					port									: 目标端口
 @Return				NBIOT_StatusTypeDef						: NBIOT处理状态
**********************************************************************************************************/
NBIOT_StatusTypeDef NBIOT_Neul_NBxx_ReadUDPPayload(NBIOT_ClientsTypeDef* pClient, int socket, int maxrlen, int *rleft, char *addr, u16 *port)
{
	NBIOT_StatusTypeDef NBStatus = NBIOT_OK;
	u32 utmp = 0;
	int rdsocket = 0;
#if NBIOT_PRINT_ERROR_CODE_TYPE
	int retErrorCode;
#endif
	
	NBIOT_Neul_NBxx_DictateEvent_SetTime(pClient, pClient->Command_Timeout_Msec);
	
	memset((void *)pClient->DataProcessStack, 0x0, pClient->DataProcessStack_size);
	sprintf((char *)pClient->DataProcessStack, "AT+NSORF=%d,%d\r", socket, maxrlen);
	
	memcpy(pClient->ATCmdStack->ATSendbuf, pClient->DataProcessStack, strlen((char *)pClient->DataProcessStack));
	pClient->ATCmdStack->ATSendlen = strlen((char *)pClient->DataProcessStack);
	pClient->ATCmdStack->ATack = "OK";
	pClient->ATCmdStack->ATNack = "ERROR";
	memset((void *)pClient->DataProcessStack, 0x0, pClient->DataProcessStack_size);
	if ((NBStatus = pClient->ATCmdStack->Write(pClient->ATCmdStack)) == NBIOT_OK) {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\x0D\x0A%d,%[^,],%hu,%hd,%[^,],%d\r", &rdsocket, addr, port, &pClient->Recvlen, pClient->DataProcessStack, rleft) <= 0) {
			NBStatus = NBIOT_ERROR;
			goto exit;
		}
		else {
			if (rdsocket != socket) {
				NBStatus = NBIOT_ERROR;
				goto exit;
			}
		}
		memset((void *)pClient->Recvbuf, 0x0, pClient->Recvbuf_size);
		for (int i = 0; i < pClient->Recvlen; i++) {
			sscanf((const char*)(pClient->DataProcessStack + i * 2), "%02X", &utmp);
			pClient->Recvbuf[i] = utmp;
		}
	}
#if NBIOT_PRINT_ERROR_CODE_TYPE
	else {
		if (sscanf((const char*)pClient->ATCmdStack->ATRecvbuf, "\r\nERROR: %d\r\n", &retErrorCode) > 0) {
			NBStatus = (NBIOT_StatusTypeDef)retErrorCode;
		}
	}
#endif
	
exit:
	return NBStatus;
}

/********************************************** END OF FLEE **********************************************/
