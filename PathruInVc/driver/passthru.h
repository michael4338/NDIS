/*++

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

	passthru.h

Abstract:

	Ndis Intermediate Miniport driver sample. This is a passthru driver.

Author:

Environment:


Revision History:

 
--*/

#ifdef NDIS51_MINIPORT
#define PASSTHRU_MAJOR_NDIS_VERSION			5
#define PASSTHRU_MINOR_NDIS_VERSION			1
#else
#define PASSTHRU_MAJOR_NDIS_VERSION			4
#define PASSTHRU_MINOR_NDIS_VERSION			0
#endif

#ifdef NDIS51
#define PASSTHRU_PROT_MAJOR_NDIS_VERSION	5
#define PASSTHRU_PROT_MINOR_NDIS_VERSION	0
#else
#define PASSTHRU_PROT_MAJOR_NDIS_VERSION	4
#define PASSTHRU_PROT_MINOR_NDIS_VERSION	0
#endif

#define MAX_BUNDLEID_LENGTH 50

#define TAG 'ImPa'
#define WAIT_INFINITE 0


//advance declaration
typedef struct _ADAPT ADAPT, *PADAPT;

extern
NTSTATUS
DriverEntry(
	IN	PDRIVER_OBJECT			DriverObject,
	IN	PUNICODE_STRING			RegistryPath
	);

NTSTATUS
PtDispatch(
	IN PDEVICE_OBJECT			DeviceObject,
	IN PIRP						Irp
	);

NDIS_STATUS
PtRegisterDevice(
	VOID
	);

NDIS_STATUS
PtDeregisterDevice(
	VOID
   );

VOID
PtUnloadProtocol(
    VOID
    );

//
// Protocol proto-types
//
extern
VOID
PtOpenAdapterComplete(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_STATUS				Status,
	IN	NDIS_STATUS				OpenErrorStatus
	);

extern
VOID
PtCloseAdapterComplete(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_STATUS				Status
	);

extern
VOID
PtResetComplete(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_STATUS				Status
	);

extern
VOID
PtRequestComplete(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNDIS_REQUEST			NdisRequest,
	IN	NDIS_STATUS				Status
	);

extern
VOID
PtStatus(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_STATUS				GeneralStatus,
	IN	PVOID					StatusBuffer,
	IN	UINT					StatusBufferSize
	);

extern
VOID
PtStatusComplete(
	IN	NDIS_HANDLE				ProtocolBindingContext
	);

extern
VOID
PtSendComplete(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNDIS_PACKET			Packet,
	IN	NDIS_STATUS				Status
	);

extern
VOID
PtTransferDataComplete(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNDIS_PACKET			Packet,
	IN	NDIS_STATUS				Status,
	IN	UINT					BytesTransferred
	);

extern
NDIS_STATUS
PtReceive(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_HANDLE				MacReceiveContext,
	IN	PVOID					HeaderBuffer,
	IN	UINT					HeaderBufferSize,
	IN	PVOID					LookAheadBuffer,
	IN	UINT					LookaheadBufferSize,
	IN	UINT					PacketSize
	);

extern
VOID
PtReceiveComplete(
	IN	NDIS_HANDLE				ProtocolBindingContext
	);

extern
INT
PtReceivePacket(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNDIS_PACKET			Packet
	);

extern
VOID
PtBindAdapter(
	OUT	PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				BindContext,
	IN	PNDIS_STRING			DeviceName,
	IN	PVOID					SystemSpecific1,
	IN	PVOID					SystemSpecific2
	);

extern
VOID
PtUnbindAdapter(
	OUT	PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_HANDLE				UnbindContext
	);
	
VOID
PtUnload(
	IN	PDRIVER_OBJECT			DriverObject
	);



extern 
NDIS_STATUS
PtPNPHandler(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNET_PNP_EVENT			pNetPnPEvent
	);




NDIS_STATUS
PtPnPNetEventReconfigure(
	IN	PADAPT			pAdapt,
	IN	PNET_PNP_EVENT	pNetPnPEvent
	);	

NDIS_STATUS 
PtPnPNetEventSetPower (
	IN PADAPT					pAdapt,
	IN	PNET_PNP_EVENT			pNetPnPEvent
	);
	

//
// Miniport proto-types
//
NDIS_STATUS
MPInitialize(
	OUT PNDIS_STATUS			OpenErrorStatus,
	OUT PUINT					SelectedMediumIndex,
	IN	PNDIS_MEDIUM			MediumArray,
	IN	UINT					MediumArraySize,
	IN	NDIS_HANDLE				MiniportAdapterHandle,
	IN	NDIS_HANDLE				WrapperConfigurationContext
	);

VOID
MPSendPackets(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	PPNDIS_PACKET			PacketArray,
	IN	UINT					NumberOfPackets
	);

NDIS_STATUS
MPSend(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	PNDIS_PACKET			Packet,
	IN	UINT					Flags
	);

/*My Function to print the packet information*/
VOID PrintPacket(
	IN PNDIS_PACKET packet
	);
////////////////////////////////////////////////

NDIS_STATUS
MPQueryInformation(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	NDIS_OID				Oid,
	IN	PVOID					InformationBuffer,
	IN	ULONG					InformationBufferLength,
	OUT PULONG					BytesWritten,
	OUT PULONG					BytesNeeded
	);

NDIS_STATUS
MPSetInformation(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	NDIS_OID				Oid,
	IN	PVOID					InformationBuffer,
	IN	ULONG					InformationBufferLength,
	OUT PULONG					BytesRead,
	OUT PULONG					BytesNeeded
	);

VOID
MPReturnPacket(
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	PNDIS_PACKET			Packet
	);

NDIS_STATUS
MPTransferData(
	OUT PNDIS_PACKET			Packet,
	OUT PUINT					BytesTransferred,
	IN	NDIS_HANDLE				MiniportAdapterContext,
	IN	NDIS_HANDLE				MiniportReceiveContext,
	IN	UINT					ByteOffset,
	IN	UINT					BytesToTransfer
	);

VOID
MPHalt(
	IN	NDIS_HANDLE				MiniportAdapterContext
	);

NDIS_STATUS
MPReset(
	OUT PBOOLEAN				AddressingReset,
	IN	NDIS_HANDLE				MiniportAdapterContext
	);

VOID
MPQueryPNPCapabilities(  
	OUT	PADAPT					MiniportProtocolContext, 
	OUT	PNDIS_STATUS			Status
	);


NDIS_STATUS
MPSetMiniportSecondary ( 
	IN	PADAPT					Secondary, 
	IN	PADAPT					Primary
	);

#ifdef NDIS51_MINIPORT

VOID
MPCancelSendPackets(
	IN	NDIS_HANDLE			MiniportAdapterContext,
	IN	PVOID				CancelId
	);

VOID
MPAdapterShutdown(
	IN NDIS_HANDLE				MiniportAdapterContext
	);

VOID
MPDevicePnPEvent(
	IN NDIS_HANDLE				MiniportAdapterContext,
	IN NDIS_DEVICE_PNP_EVENT	DevicePnPEvent,
	IN PVOID					InformationBuffer,
	IN ULONG					InformationBufferLength
	);

#endif // NDIS51_MINIPORT

VOID
MPFreeAllPacketPools(
	PADAPT					pAdapt
	);

NDIS_STATUS 
MPPromoteSecondary ( 
	IN	PADAPT					pAdapt 
	);


NDIS_STATUS 
MPBundleSearchAndSetSecondary (
	IN	PADAPT					pAdapt 
	);

VOID
MPProcessSetPowerOid(
	IN OUT PNDIS_STATUS      pNdisStatus,
	IN  PADAPT					pAdapt,
	IN	PVOID					InformationBuffer,
	IN	ULONG					InformationBufferLength,
	OUT PULONG					BytesRead,
	OUT PULONG					BytesNeeded
    );


//
// There should be no DbgPrint's in the Free version of the driver
//
#if DBG

#define DBGPRINT(Fmt)										\
	{														\
		DbgPrint("Passthru: ");								\
		DbgPrint Fmt;										\
	}

#else // if DBG

#define DBGPRINT(Fmt)											

#endif // if DBG 

#define	NUM_PKTS_IN_POOL	256


//
// Protocol reserved part of a sent packet that is allocated by us.
//
typedef struct _SEND_RSVD
{
	PNDIS_PACKET	OriginalPkt;
} SEND_RSVD, *PSEND_RSVD;

//
// Miniport reserved part of a received packet that is allocated by
// us. Note that this should fit into the MiniportReserved space
// in an NDIS_PACKET.
//
typedef struct _RECV_RSVD
{
	PNDIS_PACKET	OriginalPkt;
} RECV_RSVD, *PRECV_RSVD;

C_ASSERT(sizeof(RECV_RSVD) <= sizeof(((PNDIS_PACKET)0)->MiniportReserved));

//
// Event Codes related to the PassthruEvent Structure
//

typedef enum 
{
	Passthru_Invalid,
	Passthru_SetPower,
	Passthru_Unbind

} PASSSTHRU_EVENT_CODE, *PPASTHRU_EVENT_CODE; 

//
// Passthru Event with  a code to state why they have been state
//

typedef struct _PASSTHRU_EVENT
{
	NDIS_EVENT Event;
	PASSSTHRU_EVENT_CODE Code;

} PASSTHRU_EVENT, *PPASSTHRU_EVENT;


//
// Structure used by both the miniport as well as the protocol part of the intermediate driver
// to represent an adapter and its corres. lower bindings
//
typedef struct _ADAPT
{
	struct _ADAPT *				Next;
	
	NDIS_HANDLE					BindingHandle;	// To the lower miniport
	NDIS_HANDLE					MiniportHandle;	// NDIS Handle to for miniport up-calls
	NDIS_HANDLE					SendPacketPoolHandle;
	NDIS_HANDLE					RecvPacketPoolHandle;
	NDIS_STATUS					Status;			// Open Status
	NDIS_EVENT					Event;			// Used by bind/halt for Open/Close Adapter synch.
	NDIS_MEDIUM					Medium;
	NDIS_REQUEST				Request;		// This is used to wrap a request coming down
												// to us. This exploits the fact that requests
												// are serialized down to us.
	PULONG						BytesNeeded;
	PULONG						BytesReadOrWritten;
	BOOLEAN						IndicateRcvComplete;
	
	BOOLEAN						OutstandingRequests;  	// TRUE iff a request is pending
														// at the miniport below
	BOOLEAN						QueuedRequest;		    // TRUE iff a request is queued at
														// this IM miniport

	BOOLEAN						StandingBy;				// True - When the miniport or protocol is transitioning from a D0 to Standby (>D0) State
														// False - At all other times, - Flag is cleared after a transition to D0

	NDIS_DEVICE_POWER_STATE 	MPDeviceState;			// Miniport's Device State 
	NDIS_DEVICE_POWER_STATE 	PTDeviceState;			// Protocol's Device State 
	NDIS_STRING					DeviceName;				// For initializing the miniport edge
	NDIS_EVENT					MiniportInitEvent;		// For blocking UnbindAdapter while
														// an IM Init is in progress.
	BOOLEAN						MiniportInitPending;	// TRUE iff IMInit in progress
    NDIS_STATUS                 LastIndicatedStatus;    // The last indicated media status
    NDIS_STATUS                 LatestUnIndicateStatus; // The latest suppressed media status

} ADAPT, *PADAPT;

extern	NDIS_HANDLE						ProtHandle, DriverHandle;
extern	NDIS_MEDIUM						MediumArray[4];
extern	PADAPT							pAdaptList;
extern	NDIS_SPIN_LOCK					GlobalLock;


#define ADAPT_MINIPORT_HANDLE(_pAdapt)	((_pAdapt)->MiniportHandle)

//
// Custom Macros to be used by the passthru driver 
//
/*
BOOLEAN
IsIMDeviceStateOn(
   PADAPT 
   )

*/
#define IsIMDeviceStateOn(_pP)		((_pP)->MPDeviceState == NdisDeviceStateD0 && (_pP)->PTDeviceState == NdisDeviceStateD0 ) 

////////////////////////////////
//自定义全局变量及函数
////////////////////////////////
#define IO_RECEIVE_PACKET		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x925, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IO_SEND_PACKET			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x926, METHOD_BUFFERED, FILE_ANY_ACCESS) 
#define IO_GET_SRCMAC           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x927, METHOD_BUFFERED, FILE_ANY_ACCESS)

USHORT ANSIstrlen( char* str);

#ifdef WIN9X
#undef NdisGetPoolFromPacket
#define NdisGetPoolFromPacket(_Pkt) ((_Pkt)->Private.Pool)
#endif


#ifndef NDIS51
#define NdisGetPoolFromPacket(_Pkt) (_Pkt->Private.Pool)
#endif

//
//接收包相关变量
//

typedef struct _PACKET_TOTAL          //整个包的信息，包括数据部分,同时它是个链表结构
{
	LIST_ENTRY   ListEntry;             //下一个节点指针
	PNDIS_BUFFER pNdisBuffer;            //包的全部信息
} PACKET_TOTAL, *P_PACKET_TOTAL;

//增加的变量

PADAPT         pAdaptTotal;         //暂时用来存储绑定的PADAPT
//PUCHAR         TotalMacBuffer;      //用来存储本地MAC地址

#define        DefineNum     2      //暂时用来限制收包数量
USHORT         CurrentNum;          //暂时用来记录当前收包数量

PIRP           ReadIrp;				// "Read queue" of 1 IRP
NDIS_SPIN_LOCK ReadIrpLock;		// Spin lock to guard access to ReadIrp
ULONG          ReadCount;

LIST_ENTRY     PacketList;		// Doubly-linked list of written Events
ULONG          PacketCount;
NDIS_SPIN_LOCK PacketListLock;	// Spin lock to guard access to EventList

NDIS_HANDLE    RecvBufferPool;  //接收缓冲区
#define MAX_RECV_PACKET_POOL_SIZE    20  //接收缓冲区的最大长度

#define MAX_RECV_QUEUE_SIZE 1024         //接收队列中所放包的最大个数
#define MY_MTU              2048         //自定义的包的最大长度

//将包从队列中还原出来
#define SECLAB_LIST_ENTRY_TO_RCV_PKT(_pEnt)   \
    CONTAINING_RECORD(CONTAINING_RECORD(_pEnt, PACKET_TOTAL, ListEntry), NDIS_PACKET, ProtocolReserved)

//将包连接到队列中
#define SECLAB_RCV_PKT_TO_LIST_ENTRY(_pPkt)   \
    (&((P_PACKET_TOTAL)&((_pPkt)->ProtocolReserved[0]))->ListEntry)

//接收包转化为NDIS_BUFFER
#define SECLAB_RCV_PKT_TO_ORIGINAL_BUFFER(_pPkt)  \
    (((P_PACKET_TOTAL)&((_pPkt)->ProtocolReserved[0]))->pNdisBuffer)

//两数中取小者
#ifndef MIN
#define MIN(_a, _b) ((_a) < (_b)? (_a): (_b))
#endif





//
//发送包相关变量
//

typedef struct _SECLAB_ETH_HEADER
{
    UCHAR       DstAddr[6];
    UCHAR       SrcAddr[6];
    USHORT      EthType;

} SECLAB_ETH_HEADER;

#define  MAX_SEND_PACKETLEN    1024

NDIS_HANDLE     SendBufferPool;  //发送缓冲区
//
//  发送封包缓冲池
//
#define MIN_SEND_PACKET_POOL_SIZE    20
#define MAX_SEND_PACKET_POOL_SIZE    400
  
     
//
//发送封包IRP队列
//
typedef struct _SECLAB_SEND_PACKET_RSVD
{
    PIRP                    pIrp;
    ULONG                   RefCount;//用来同步发送线程和取消线程
	   
	USHORT                  Signal1;
    USHORT                  Signal2;
	USHORT                  Signal3;

} SECLAB_SEND_PACKET_RSVD, *PSECLAB_SEND_PACKET_RSVD;

//发送封包IRP队列
LIST_ENTRY              PendedWritesList;
//等待的发送封包的IRP请求数目
ULONG                   PendedSendCount; 
//维护同步访问队列的自旋锁
NDIS_SPIN_LOCK          WriteIrpLock;	


//自定义信号结构，用来指示自己程序所要发送的包与系统发送包之间的区别
typedef struct _SECLAB_SEND_PACKET_RSVD_SIGNAL
{
    USHORT                  Signal1;
    USHORT                  Signal2;
	USHORT                  Signal3;
} SECLAB_SEND_PACKET_RSVD_SIGNAL, *PSECLAB_SEND_PACKET_RSVD_SIGNAL;
//与此信号有关的宏
//从包中得到信号值

//Packet结构中的这个ProtocolReserved域只有16字节，所以不能分开用，用了ProtocolReserved[0]
//就不能用[1]了。

#define SECLAB_SIGNAL1_FROM_SEND_PKT(_pPkt)		\
	(((PSECLAB_SEND_PACKET_RSVD)&((_pPkt)->ProtocolReserved[0]))->Signal1)
#define SECLAB_SIGNAL2_FROM_SEND_PKT(_pPkt)		\
	(((PSECLAB_SEND_PACKET_RSVD)&((_pPkt)->ProtocolReserved[0]))->Signal2)
#define SECLAB_SIGNAL3_FROM_SEND_PKT(_pPkt)		\
	(((PSECLAB_SEND_PACKET_RSVD)&((_pPkt)->ProtocolReserved[0]))->Signal3)

//自定义的三个信号值
#define SIGNAL1  128
#define SIGNAL2  151
#define SIGNAL3  23


// 用来取消发送
UCHAR                   PartialCancelId;    
ULONG                   LocalCancelId; 
   
//自定义宏
#define SECLAB_IRP_FROM_SEND_PKT(_pPkt)		\
	(((PSECLAB_SEND_PACKET_RSVD)&((_pPkt)->ProtocolReserved[0]))->pIrp)

#define SECLAB_SEND_PKT_RSVD(_pPkt)           \
    ((PSECLAB_SEND_PACKET_RSVD)&((_pPkt)->ProtocolReserved[0]))


#define SECLAB_REF_SEND_PKT(_pPkt)            \
    (VOID)NdisInterlockedIncrement(&SECLAB_SEND_PKT_RSVD(_pPkt)->RefCount)


#define SECLAB_DEREF_SEND_PKT(_pPkt)          \
    {                                                                               \
        if (NdisInterlockedDecrement(&SECLAB_SEND_PKT_RSVD(_pPkt)->RefCount) == 0)    \
        {                                                                           \
            NdisFreePacket(_pPkt);                                                  \
        }                                                                           \
    }


#ifdef NDIS51

//
//  Cancel IDs are generated by using the partial cancel ID we got from
//  NDIS ORed with a monotonically increasing locally generated ID.
//
#define SECLAB_CANCEL_ID_LOW_MASK     (((ULONG_PTR)-1) >> 8) 

#define SECLAB_GET_NEXT_CANCEL_ID()                                                   \
        (PVOID)(PartialCancelId |                                           \
         ((NdisInterlockedIncrement(&LocalCancelId)) & SECLAB_CANCEL_ID_LOW_MASK))

#endif // NDIS51

//查询信息结构
# define REQUEST_SIGNAL               128  //标识是否为自己的查询            
# define MY_OID_802_3_CURRENT_ADDRESS 128  //标识查询以太网的源MAC地址
typedef struct _SECLAB_QUERY_OID
{
    NDIS_OID        Oid;
    UCHAR           Data[sizeof(ULONG)];

} SECLAB_QUERY_OID, *PSECLAB_QUERY_OID;
typedef struct _SECLAB_REQUEST
{
    NDIS_REQUEST            Request;
    NDIS_EVENT              ReqEvent;
    ULONG                   Status;
    USHORT                  Signal; //用来判断是否为自己的查询请求
} SECLAB_REQUEST, *PSECLAB_REQUEST;

//增加的函数

////////////////////////////////////发送////////////////////////////////
//发送封包，响应应用程序
NTSTATUS 
SecLabSendPacket(
                 PADAPT			         pAdapt,
				 IN PDEVICE_OBJECT DeviceObject,
				 IN PIRP pIrp);

//实现发送封包的功能
NTSTATUS 
SendPacketOut(PIRP Irp);

//取消发送
VOID
SecLabCancelWrite(
    IN PDEVICE_OBJECT               pDeviceObject,
    IN PIRP                         pIrp
    );

//查询信息，包括以太网中的源MAC地址和媒体类型
NTSTATUS
SecLabQueryOidValue(
   IN PADAPT			         pAdapt,
   OUT PVOID                       pDataBuffer,
   IN  ULONG                       BufferLength,
   OUT PULONG                      pBytesWritten
    );
NDIS_STATUS
SecLabDoRequest(
    IN PADAPT			            pAdapt,
    IN NDIS_REQUEST_TYPE            RequestType,
    IN NDIS_OID                     Oid,
    IN PVOID                        InformationBuffer,
    IN UINT                         InformationBufferLength,
    OUT PUINT                       pBytesProcessed
    );

/////////////////////////////////////////////////////////////////////////



////////////////////////////////////接收/////////////////////////////////
//接收封包，响应应用程序
NTSTATUS 
SecLabReceivePacket(
					PADAPT			        pAdapt,
					IN PDEVICE_OBJECT DeviceObject,
					IN PIRP Irp);

//从队列中读取封包
BOOLEAN
PickPacketFromList(
				   PADAPT			        pAdapt,
				   PIRP                     Irp
				   );

//分配封包资源函数
PNDIS_PACKET
SecLabAllocateReceivePacket(
	PADAPT			                pAdapt,
    IN UINT                         DataLength,
    OUT PUCHAR *                    ppDataBuffer
    );

//释放封包资源函数	
VOID
SecLabFreeReceivePacket(
	PADAPT                          pAdapt,
    IN PNDIS_PACKET                 pNdisPacket
    );

//封包入队列函数
VOID 
EnListPacket(
			 PADAPT                          pAdapt,
			 IN                              PNDIS_PACKET pRcvPacket
			 );

//取消读取封包例程
VOID SecLabCancelIrp(
					 IN PDEVICE_OBJECT DeviceObject,
					 IN PIRP Irp);
//////////////////////////////////////////////////////////////////////////



//清除封包队列
VOID
ClearQueues(PADAPT pAdapt);

//共用例程，完成IRP
NTSTATUS 
CompleteIrp(
	PIRP Irp, 
	NTSTATUS status, ULONG info
	);


