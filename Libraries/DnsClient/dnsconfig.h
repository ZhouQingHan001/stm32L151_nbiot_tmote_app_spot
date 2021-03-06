#ifndef __DNS_CONFIG_H
#define   __DNS_CONFIG_H

#include "sys.h"
#include "nbiotconfig.h"
#include "net_nbiot_app.h"

#define DNS_COMMAND_TIMEOUT_SEC			30
#define DNS_COMMAND_FAILURE_CNT			3

/* DNS 协议栈开辟缓存大小 */
#define DNS_BUFFER_SIZE					256
#define DNS_DATASTACK_SIZE				256
#define DNS_HOSTNAME_SIZE				30
#define DNS_HOSTIP_SIZE					17

#define DNS_ANALYSIS_DATA				2
#define DNS_ANALYSIS_HOSTNAME1			"mqtt.movebroad.cn"
#define DNS_ANALYSIS_HOSTIP1				"47.98.140.145"

#define DNS_ANALYSIS_HOSTNAME2			"movebroad.cn"
#define DNS_ANALYSIS_HOSTIP2				"106.14.142.169"

typedef struct DNS_SocketNetTypeDef		DNS_SocketNetTypeDef;
typedef struct DNS_ClientsTypeDef			DNS_ClientsTypeDef;

/* DNS Is ProcessState */
typedef enum
{
	DNS_PROCESS_CREAT_UDP_SOCKET			= 0x00,
	DNS_PROCESS_SEND_DNS_STRUCT_DATA		= 0x01,
	DNS_PROCESS_RECV_DNS_STRUCT_DATA		= 0x02,
	DNS_PROCESS_CLOSE_UDP_SOCKET			= 0x03,
	DNS_PROCESS_OVER_DNS_ANALYSIS			= 0x04
}DNS_ProcessStateTypeDef;

/* DNS Status */
typedef enum
{
	DNS_OK       						= 0x00,
	DNS_ERROR    						= 0x01,
}DNS_StatusTypeDef;

/* DNS Header 12Byte */
typedef struct
{
	// 2 byte ID
	unsigned short						id;
	
	// 2 byte flag broken down with bitfields
	// BYTE 1 in REVERSE ORDER											//pos
	unsigned char						rd		:1;						//7
	unsigned char						tc		:1;						//6
	unsigned char						aa		:1;						//5
	unsigned char						opcode	:4;						//4-1
	unsigned char						qr		:1;						//0
	
	// BYTE 2 in REVERSE ORDER
	unsigned char						rcode	:4;						//15-12
	unsigned char						z		:3;						//11-9
	unsigned char						ra		:1;						//8
	
	// counts, each 2 bytes
	unsigned short						q_count;
	unsigned short						ans_count;
	unsigned short						auth_count;
	unsigned short						add_count;
}DNS_HeaderTypeDef;

/* DNS Question */
typedef struct
{
	unsigned short						qtype;
	unsigned short						qclass;
}DNS_QuestionTypeDef;

/* DNS Record */
typedef struct
{
	unsigned short						name;
	unsigned short						type;
	unsigned short						class;
	unsigned int						ttl;
	unsigned short						data_len;
	unsigned char*						rdata;
}DNS_RecordTypeDef;

/* DNS Query */
typedef struct
{
	unsigned char*						name;
	DNS_QuestionTypeDef*				ques;
}DNS_QueryTypeDef;

/* DNS Analysis Data */
typedef struct
{
	unsigned char*						hostnameAddr;
	unsigned char						hostname[DNS_HOSTNAME_SIZE];
	unsigned char*						hostIPAddr;
	unsigned char						hostIP[DNS_HOSTIP_SIZE];
}DNS_AnalysisDataTypeDef;

/* DNS Socket */
struct DNS_SocketNetTypeDef
{
	int								SocketHandler;
	unsigned short						LocalPort;
	unsigned short						ServerPort;
	unsigned char						ServerHost[16];
	NBIOT_ClientsTypeDef*				NBIotStack;
	DNS_StatusTypeDef					(*Open)(DNS_SocketNetTypeDef*);
	DNS_StatusTypeDef					(*Close)(DNS_SocketNetTypeDef*);
	DNS_StatusTypeDef					(*Write)(DNS_SocketNetTypeDef*, const char*, u16);
	DNS_StatusTypeDef					(*Read)(DNS_SocketNetTypeDef*, char*, u16, int*, int*);
};

/* DNS Clients */
struct DNS_ClientsTypeDef
{
	unsigned char*						Sendbuf;
	unsigned char*						Recvbuf;
	size_t							Sendbuf_size;
	size_t							Recvbuf_size;
	short							Sendlen;
	short							Recvlen;
	unsigned char*						DataProcessStack;
	size_t							DataProcessStack_size;
	unsigned short						Command_Timeout_Sec;
	unsigned short						Command_Failure_Cnt;
	
	int								LetterCounter;
	
	unsigned char						AnalysisTick;
	DNS_AnalysisDataTypeDef				AnalysisData[DNS_ANALYSIS_DATA];
	
	struct DNSDictateRuningCtlTypeDef
	{
		bool							dictateEnable;
		unsigned int					dictateTimeoutSec;
		unsigned char					dictateCreatUDPSocketFailureCnt;
		unsigned char					dictateSendDnsStructDataFailureCnt;
		unsigned char					dictateRecvDnsStructDataFailureCnt;
		unsigned char					dictateCloseUDPSocketFailureCnt;
		unsigned char					dictateOverDnsAnalysis;
		Stm32_CalculagraphTypeDef		dictateRunTime;
	}DictateRunCtl;
	
	DNS_ProcessStateTypeDef				ProcessState;
	DNS_SocketNetTypeDef*				SocketStack;
	NET_NBIOT_ClientsTypeDef*			NetNbiotStack;
};

/* Application Programming Interface */
void DNS_Client_Init(DNS_ClientsTypeDef* pClient, DNS_SocketNetTypeDef* NetSock, NET_NBIOT_ClientsTypeDef* NetNbiotStack);		//DNS客户端初始化

#endif
