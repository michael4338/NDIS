/*++

Copyright (c) 1992-2000  Microsoft Corporation
 
Module Name:
 
	passthru.c

Abstract:

	Ndis Intermediate Miniport driver sample. This is a passthru driver.

Author:

Environment:


Revision History:


--*/


#include "precomp.h"
#pragma hdrstop

#pragma NDIS_INIT_FUNCTION(DriverEntry)

NDIS_HANDLE						ProtHandle = NULL;
NDIS_HANDLE						DriverHandle = NULL;
NDIS_MEDIUM						MediumArray[4] =
									{
										NdisMedium802_3,	// Ethernet
										NdisMedium802_5,	// Token-ring
										NdisMediumFddi,		// Fddi
										NdisMediumWan       // NDISWAN
									};

NDIS_SPIN_LOCK	GlobalLock;

PADAPT			pAdaptList = NULL;
LONG            MiniportCount = 0;

NDIS_HANDLE		NdisWrapperHandle;

//
// To support ioctls from user-mode:
//


#define LINKNAME_STRING     L"\\DosDevices\\Passthru"
#define NTDEVICE_STRING     L"\\Device\\Passthru"

NDIS_HANDLE     NdisDeviceHandle = NULL;
PDEVICE_OBJECT  ControlDeviceObject = NULL;

enum _DEVICE_STATE
{
	PS_DEVICE_STATE_READY = 0,	// ready for create/delete
	PS_DEVICE_STATE_CREATING,	// create operation in progress
	PS_DEVICE_STATE_DELETING	// delete operation in progress
}				ControlDeviceState = PS_DEVICE_STATE_READY;



NTSTATUS
DriverEntry(
	IN	PDRIVER_OBJECT		DriverObject,
	IN	PUNICODE_STRING		RegistryPath
	)
/*++

Routine Description:

	First entry point to be called, when this driver is loaded.
	Register with NDIS as an intermediate driver.

Arguments:

	DriverObject - pointer to the system's driver object structure
		for this driver
	
	RegistryPath - system's registry path for this driver
	
Return Value:

	STATUS_SUCCESS if all initialization is successful, STATUS_XXX
	error code if not.

--*/
{
	NDIS_STATUS						Status;
	NDIS_PROTOCOL_CHARACTERISTICS	PChars;
	NDIS_MINIPORT_CHARACTERISTICS	MChars;
	PNDIS_CONFIGURATION_PARAMETER	Param;
	NDIS_STRING						Name;

	Status = NDIS_STATUS_SUCCESS;
	NdisAllocateSpinLock(&GlobalLock);

	//初始化自定义变量
	NdisAllocateSpinLock(&PacketListLock);
	InitializeListHead(&PacketList);

	NdisAllocateSpinLock(&ReadIrpLock);
	ReadIrp = NULL;
	ReadCount=0;
	PacketCount=0;
	RecvBufferPool=NULL;
	SendBufferPool=NULL;

	PendedSendCount=0;
    NdisAllocateSpinLock(&WriteIrpLock);
    InitializeListHead(&PendedWritesList);

#ifdef NDIS51
	PartialCancelId = NdisGeneratePartialCancelId();
    PartialCancelId <<= ((sizeof(PVOID) - 1) * 8);
#endif

	pAdaptTotal=NULL;
//	TotalMacBuffer=NULL;
	CurrentNum=0;

    NdisAllocateBufferPool(
            &Status,
            &RecvBufferPool,
            MAX_RECV_PACKET_POOL_SIZE);
	NdisAllocateBufferPool(
            &Status,
            &SendBufferPool,
            MAX_SEND_PACKET_POOL_SIZE);

	NdisMInitializeWrapper(&NdisWrapperHandle, DriverObject, RegistryPath, NULL);

	do
	{
		//
		// Register the miniport with NDIS. Note that it is the miniport
		// which was started as a driver and not the protocol. Also the miniport
		// must be registered prior to the protocol since the protocol's BindAdapter
		// handler can be initiated anytime and when it is, it must be ready to
		// start driver instances.
		//

		NdisZeroMemory(&MChars, sizeof(NDIS_MINIPORT_CHARACTERISTICS));

		MChars.MajorNdisVersion = PASSTHRU_MAJOR_NDIS_VERSION;
		MChars.MinorNdisVersion = PASSTHRU_MINOR_NDIS_VERSION;

		MChars.InitializeHandler = MPInitialize;
		MChars.QueryInformationHandler = MPQueryInformation;
		MChars.SetInformationHandler = MPSetInformation;
		MChars.ResetHandler = MPReset;
		MChars.TransferDataHandler = MPTransferData;
		MChars.HaltHandler = MPHalt;
#ifdef NDIS51_MINIPORT
		MChars.CancelSendPacketsHandler = MPCancelSendPackets;
		MChars.PnPEventNotifyHandler = MPDevicePnPEvent;
		MChars.AdapterShutdownHandler = MPAdapterShutdown;
#endif // NDIS51_MINIPORT

		//
		// We will disable the check for hang timeout so we do not
		// need a check for hang handler!
		//
		MChars.CheckForHangHandler = NULL;
		MChars.ReturnPacketHandler = MPReturnPacket;

		//
		// Either the Send or the SendPackets handler should be specified.
		// If SendPackets handler is specified, SendHandler is ignored
		//
		MChars.SendHandler = NULL;	// MPSend;
		MChars.SendPacketsHandler = MPSendPackets;

		Status = NdisIMRegisterLayeredMiniport(NdisWrapperHandle,
   											   &MChars,
   											   sizeof(MChars),
										   	   &DriverHandle);
		if (Status != NDIS_STATUS_SUCCESS)
		{
			break;
		}

#ifndef WIN9X
		NdisMRegisterUnloadHandler(NdisWrapperHandle, PtUnload);
#endif

		//
		// Now register the protocol.
		//
		NdisZeroMemory(&PChars, sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
		PChars.MajorNdisVersion = PASSTHRU_PROT_MAJOR_NDIS_VERSION;
		PChars.MinorNdisVersion = PASSTHRU_PROT_MINOR_NDIS_VERSION;

		//
		// Make sure the protocol-name matches the service-name
		// (from the INF) under which this protocol is installed.
		// This is needed to ensure that NDIS can correctly determine
		// the binding and call us to bind to miniports below.
		//
		NdisInitUnicodeString(&Name, L"Passthru");	// Protocol name
		PChars.Name = Name;
		PChars.OpenAdapterCompleteHandler = PtOpenAdapterComplete;
		PChars.CloseAdapterCompleteHandler = PtCloseAdapterComplete;
		PChars.SendCompleteHandler = PtSendComplete;
		PChars.TransferDataCompleteHandler = PtTransferDataComplete;
	
		PChars.ResetCompleteHandler = PtResetComplete;
		PChars.RequestCompleteHandler = PtRequestComplete;
		PChars.ReceiveHandler = PtReceive;
		PChars.ReceiveCompleteHandler = PtReceiveComplete;
		PChars.StatusHandler = PtStatus;
		PChars.StatusCompleteHandler = PtStatusComplete;
		PChars.BindAdapterHandler = PtBindAdapter;
		PChars.UnbindAdapterHandler = PtUnbindAdapter;
		PChars.UnloadHandler = PtUnloadProtocol;

		PChars.ReceivePacketHandler = PtReceivePacket;
		PChars.PnPEventHandler= PtPNPHandler;

		NdisRegisterProtocol(&Status,
 							&ProtHandle,
 							&PChars,
 							sizeof(NDIS_PROTOCOL_CHARACTERISTICS));

		if (Status != NDIS_STATUS_SUCCESS)
		{
			NdisIMDeregisterLayeredMiniport(DriverHandle);
			break;
		}

		NdisIMAssociateMiniport(DriverHandle, ProtHandle);
	}
	while (FALSE);

	if (Status != NDIS_STATUS_SUCCESS)
	{
		NdisTerminateWrapper(NdisWrapperHandle, NULL);
	}

	return(Status);
}


NDIS_STATUS
PtRegisterDevice(
	VOID
	)
/*++

Routine Description:

	Register an ioctl interface - a device object to be used for this
	purpose is created by NDIS when we call NdisMRegisterDevice.

	This routine is called whenever a new miniport instance is
	initialized. However, we only create one global device object,
	when the first miniport instance is initialized. This routine
	handles potential race conditions with PtDeregisterDevice via
	the ControlDeviceState and MiniportCount variables.

	NOTE: do not call this from DriverEntry; it will prevent the driver
	from being unloaded (e.g. on uninstall).

Arguments:

	None

Return Value:

	NDIS_STATUS_SUCCESS if we successfully register a device object.

--*/
{
	NDIS_STATUS			Status = NDIS_STATUS_SUCCESS;
	UNICODE_STRING		DeviceName;
	UNICODE_STRING		DeviceLinkUnicodeString;
	PDRIVER_DISPATCH	DispatchTable[IRP_MJ_MAXIMUM_FUNCTION];
	UINT				i;

	DBGPRINT(("==>PtRegisterDevice\n"));

	NdisAcquireSpinLock(&GlobalLock);

	++MiniportCount;
	
	if (1 == MiniportCount)
    {
		ASSERT(ControlDeviceState != PS_DEVICE_STATE_CREATING);

		//
		// Another thread could be running PtDeregisterDevice on
		// behalf of another miniport instance. If so, wait for
		// it to exit.
		//
		while (ControlDeviceState != PS_DEVICE_STATE_READY)
		{
			NdisReleaseSpinLock(&GlobalLock);
			NdisMSleep(1);
			NdisAcquireSpinLock(&GlobalLock);
		}

		ControlDeviceState = PS_DEVICE_STATE_CREATING;

		NdisReleaseSpinLock(&GlobalLock);

		for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
		{
		    DispatchTable[i] = PtDispatch;
		}

		NdisInitUnicodeString(&DeviceName, NTDEVICE_STRING);
		NdisInitUnicodeString(&DeviceLinkUnicodeString, LINKNAME_STRING);

		//
		// Create a device object and register our dispatch handlers
		//
		
		Status = NdisMRegisterDevice(
                    NdisWrapperHandle, 
                    &DeviceName,
                    &DeviceLinkUnicodeString,
                    &DispatchTable[0],
                    &ControlDeviceObject,
                    &NdisDeviceHandle
                    );

		NdisAcquireSpinLock(&GlobalLock);

		ControlDeviceState = PS_DEVICE_STATE_READY;
	}

	NdisReleaseSpinLock(&GlobalLock);

	DBGPRINT(("<==PtRegisterDevice: %x\n", Status));

	return (Status);
}


NTSTATUS
PtDispatch(
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				Irp
	)
/*++
Routine Description:

	Process IRPs sent to this device.

Arguments:

	DeviceObject - pointer to a device object
	Irp	  - pointer to an I/O Request Packet

Return Value:

	NTSTATUS - STATUS_SUCCESS always - change this when adding
	real code to handle ioctls.

--*/
{
	PIO_STACK_LOCATION  IrpStack;
	NTSTATUS	        status = STATUS_SUCCESS;

	PADAPT pAdapt;

	ULONG ControlCode;
	ULONG  BytesReturned;

//	DBGPRINT(("==>Pt Dispatch\n"));
	IrpStack = IoGetCurrentIrpStackLocation(Irp);

	ControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode; 
    //取得设备扩展的值
	//pAdapt = (PADAPT)IrpStack->FileObject->FsContext;
	pAdapt=pAdaptTotal;
	if(pAdapt==NULL)
	{
		DbgPrint("NULL OF PADAPTTOTAL");
        Irp->IoStatus.Status = status;
	    IoCompleteRequest(Irp, IO_NO_INCREMENT);
	    return status;
	}

	switch (IrpStack->MajorFunction)
	{
		case IRP_MJ_CREATE:
			break;
		case IRP_MJ_CLOSE:
			break;    
		case IRP_MJ_WRITE:
			//
			//如果要用WriteFile的话，那个拷贝内存语句
			//RtlCopyMemory( pData, pIrp->AssociatedIrp.SystemBuffer, DataLength);
			//总是失败重启，因此决定采用DeviceIoControl方式
			//

			//status=SecLabSendPacket(pAdapt,DeviceObject,Irp);
			//return status;
			break;
		case IRP_MJ_DEVICE_CONTROL:
			//
			// Add code here to handle ioctl commands sent to passthru.
			//
            switch (ControlCode) 
			{ 
			case IO_RECEIVE_PACKET:
		        //调用读例程
                status=SecLabReceivePacket(pAdapt,DeviceObject,Irp);
				return status;
				break;
			case IO_SEND_PACKET:
				//调用写例程
				status=SecLabSendPacket(pAdapt,DeviceObject,Irp);
				return status;
				break;
			case IO_GET_SRCMAC:
                /*
				OutputLength=IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
			
				RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,TotalMacBuffer,OutputLength);
	            for(BytesTxd=0;BytesTxd<OutputLength;BytesTxd++)
				{
		            DbgPrint("---%d---%u",BytesTxd,TotalMacBuffer[BytesTxd]);
				}
			    ExFreePool(TotalMacBuffer);
				return CompleteIrp(Irp,status,OutputLength);
				*/
				status = SecLabQueryOidValue(
                       pAdapt,
                       Irp->AssociatedIrp.SystemBuffer,
                       IrpStack->Parameters.DeviceIoControl.OutputBufferLength,
                       &BytesReturned
                       );
				if (status != STATUS_PENDING)
				{
                    Irp->IoStatus.Information = BytesReturned;
                    Irp->IoStatus.Status = status;
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);
				}
				return status;
				break;
			default:
				break;
			}
			
					
			break;   

		default:
			break;
	}

	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	DBGPRINT(("<== Pt Dispatch\n"));

	return status;

} 

USHORT ANSIstrlen( char* str)
{
	USHORT len = 0;
	for(;*str++!='\0';)
		len++;
	return len;
}

NDIS_STATUS
PtDeregisterDevice(
	VOID
	)
/*++

Routine Description:

	Deregister the ioctl interface. This is called whenever a miniport
	instance is halted. When the last miniport instance is halted, we
	request NDIS to delete the device object

Arguments:

	NdisDeviceHandle - Handle returned by NdisMRegisterDevice

Return Value:

	NDIS_STATUS_SUCCESS if everything worked ok

--*/
{
	NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

	DBGPRINT(("==>PassthruDeregisterDevice\n"));

	NdisAcquireSpinLock(&GlobalLock);

	ASSERT(MiniportCount > 0);

	--MiniportCount;
	
	if (0 == MiniportCount)
	{
		//
		// All miniport instances have been halted. Deregister
		// the control device.
		//

		ASSERT(ControlDeviceState == PS_DEVICE_STATE_READY);

		//
		// Block PtRegisterDevice() while we release the control
		// device lock and deregister the device.
		// 
		ControlDeviceState = PS_DEVICE_STATE_DELETING;

		NdisReleaseSpinLock(&GlobalLock);

		if (NdisDeviceHandle != NULL)
		{
			Status = NdisMDeregisterDevice(NdisDeviceHandle);
			NdisDeviceHandle = NULL;
		}

		NdisAcquireSpinLock(&GlobalLock);
		ControlDeviceState = PS_DEVICE_STATE_READY;
	}

	NdisReleaseSpinLock(&GlobalLock);

	DBGPRINT(("<== PassthruDeregisterDevice: %x\n", Status));
	return Status;
	
}


VOID
PtUnloadProtocol(
    VOID
)
{
	NDIS_STATUS Status;

	if (ProtHandle != NULL)
	{
		NdisDeregisterProtocol(&Status, ProtHandle);
		ProtHandle = NULL;
	}

	DBGPRINT(("PtUnloadProtocol: done!\n"));
}


//自定义函数的实现

VOID SecLabCancelIrp(IN PDEVICE_OBJECT DeviceObject,
					 IN PIRP Irp)
{
	IoReleaseCancelSpinLock(Irp->CancelIrql);

	// If this is our queued read, then unqueue it
	NdisAcquireSpinLock(&ReadIrpLock);
	if( Irp==ReadIrp)
	{
		ReadIrp = NULL;
		ReadCount=0;
	}
	NdisReleaseSpinLock(&ReadIrpLock);

	// Whatever Irp it is, just cancel it
	CompleteIrp(Irp,STATUS_CANCELLED,0);
}


BOOLEAN PickPacketFromList(PADAPT pAdapt,PIRP Irp)
{
	PIO_STACK_LOCATION IrpStack;
	ULONG PacketLenTotal=0;

    PNDIS_PACKET        pRcvPacket;
    PLIST_ENTRY         pRcvPacketEntry;
    PUCHAR              pSrc, pDst;
	PUCHAR              pDstFormer;
    ULONG               BytesRemaining; // at pDst
    PNDIS_BUFFER        pNdisBuffer;
    ULONG               BytesAvailable;
	ULONG               BytesToCopy;


    NdisAcquireSpinLock(&PacketListLock);

    if(!IsListEmpty(&PacketList))
    {
        //
        //  Get the first queued receive packet
        //
        pRcvPacketEntry = PacketList.Flink;
        RemoveEntryList(pRcvPacketEntry);

        PacketCount--;

        NdisReleaseSpinLock(&PacketListLock);


        pRcvPacket = SECLAB_LIST_ENTRY_TO_RCV_PKT(pRcvPacketEntry);

        //
        //  Copy as much data as possible from the receive packet to
        //  the IRP MDL.
        //
        pDst = (PUCHAR)ExAllocatePool(NonPagedPool,MY_MTU);
        
		if(pDst==NULL)
		{
			DbgPrint("Can not allocate enough pool");
			DbgBreakPoint();
		}
        pDstFormer=pDst;

		IrpStack = IoGetCurrentIrpStackLocation(Irp);
	    BytesRemaining = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

        pNdisBuffer = pRcvPacket->Private.Head;

        while (BytesRemaining && (pNdisBuffer != NULL))
        {

            NdisQueryBufferSafe(pNdisBuffer, &pSrc, &BytesAvailable, NormalPagePriority);

            if (pSrc == NULL) 
            {
                DbgPrint("PickPacketFromList: QueryBuffer failed for buffer");
                break;
            }

            if (BytesAvailable)
            {
                BytesToCopy = MIN(BytesAvailable, BytesRemaining);

                NdisMoveMemory(pDstFormer, pSrc, BytesToCopy);
                BytesRemaining -= BytesToCopy;
                pDstFormer += BytesToCopy;
				PacketLenTotal += BytesToCopy;
            }

            NdisGetNextBuffer(pNdisBuffer, &pNdisBuffer);
        }

        //
        //  Complete the IRP.
        //
		RtlCopyMemory( Irp->AssociatedIrp.SystemBuffer, pDst, PacketLenTotal);
	    IoSetCancelRoutine(Irp,NULL);
	    CompleteIrp(Irp,STATUS_SUCCESS,PacketLenTotal);

		ExFreePool(pDst);
        //
        //  Free up the receive packet - back to the miniport if it
        //  belongs to it, else reclaim it (local copy).
        //
        if (NdisGetPoolFromPacket(pRcvPacket) != pAdapt->RecvPacketPoolHandle)
        {
            NdisReturnPackets(&pRcvPacket, 1);
        }
        else
        {
            SecLabFreeReceivePacket(pAdapt, pRcvPacket);
        }

    }
	else
	{
		NdisReleaseSpinLock(&PacketListLock);
		return FALSE;
	}
   
	return TRUE;
}


NTSTATUS SecLabReceivePacket(PADAPT	pAdapt,IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{	
	// 某一时刻只有一个读IRP被允许存放在队列中

	NdisAcquireSpinLock(&ReadIrpLock);
	if(ReadIrp!=NULL /*&& ReadCount!=0*/)
	{
		NdisReleaseSpinLock(&ReadIrpLock);
		return CompleteIrp(Irp,STATUS_UNSUCCESSFUL,0);
	}

	// 看是否成功取出封包数据
	if( PickPacketFromList(pAdapt,Irp))
	{
        ReadCount=0;
		ReadIrp=NULL;
		NdisReleaseSpinLock(&ReadIrpLock);
		return STATUS_SUCCESS;
	}

	// 如果队列中没有封包数据，就将此IRP排队
	ReadIrp = Irp;
	ReadCount=1;

	// Mark Irp as pending and set Cancel routine
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_PENDING;
	IoMarkIrpPending(Irp);
	IoSetCancelRoutine(Irp,SecLabCancelIrp);

	NdisReleaseSpinLock(&ReadIrpLock);

	return STATUS_PENDING;
}

NTSTATUS CompleteIrp( PIRP Irp, NTSTATUS status, ULONG info)
{
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	return status;
}

VOID
EnListPacket(
	PADAPT                          pAdapt,
    IN PNDIS_PACKET                 pRcvPacket
    )
/*++

Routine Description:

    将一个接收到的包挂在接收包队列上。如果队列长度过长的话就丢弃一些包
    最后，通知并运行接收包例程
    
Arguments:
    
    pRcvPacket - 接收到的包

Return Value:

    None

--*/
{
    PLIST_ENTRY     pListEntry;
    PLIST_ENTRY     pDiscardEntry;
    PNDIS_PACKET    pDiscardPacket;

	
	P_PACKET_TOTAL  pPacketTotal;
	P_PACKET_TOTAL  pDiscardTotal;
	USHORT          Len;

    do
    {
		//将packet转化为PACKET_TOTAL结构，即给它加上ListEntry项
		pListEntry = SECLAB_RCV_PKT_TO_LIST_ENTRY(pRcvPacket);
        
		//将此包加入到接收包队列中
        NdisAcquireSpinLock(&PacketListLock);
	    InsertTailList(&PacketList,pListEntry);
		PacketCount++;

        NdisReleaseSpinLock(&PacketListLock);
        //
        //  检查接收包队列是否过长
        //
		NdisAcquireSpinLock(&PacketListLock);
        if (PacketCount > MAX_RECV_QUEUE_SIZE)
        {
            //
            //  删除队列头的包.
            //
			pDiscardEntry = PacketList.Flink;
            RemoveEntryList(pDiscardEntry);

            PacketCount--;

			// 得到封包
            pDiscardPacket = SECLAB_LIST_ENTRY_TO_RCV_PKT(pDiscardEntry);

            SecLabFreeReceivePacket(pAdapt,pDiscardPacket);
        }
        NdisReleaseSpinLock(&PacketListLock);

        //
        //  运行接收包例程
        //
        NdisAcquireSpinLock(&ReadIrpLock);
		if(ReadIrp!=NULL && ReadIrp->IoStatus.Status == STATUS_PENDING/*&& ReadCount!=0*/)
		{
			if(PickPacketFromList(pAdapt,ReadIrp))
			{
				ReadIrp=NULL;
				ReadCount=0;
			}
		}
        NdisReleaseSpinLock(&ReadIrpLock);
    }
    while (FALSE);
}


PNDIS_PACKET
SecLabAllocateReceivePacket(
    PADAPT			                pAdapt,
    IN UINT                         DataLength,
    OUT PUCHAR *                    ppDataBuffer
    )
/*++

Routine Description:

    分配用于拷贝和排队接收包的资源.

Arguments:

    DataLength - 封包的总长度，包括包头和数据
    ppDataBuffer - 返回的缓冲区地址

Return Value:

    如果成功则返回包的指针，否则为空.

--*/
{
    PNDIS_PACKET            pNdisPacket;
    PNDIS_BUFFER            pNdisBuffer;
    PUCHAR                  pDataBuffer;
    NDIS_STATUS             Status;

    pNdisPacket = NULL;
    pNdisBuffer = NULL;
    pDataBuffer = NULL;

    do
    {
		NdisAllocateMemoryWithTag((PVOID *)(&pDataBuffer), DataLength,'lceS');

        if (pDataBuffer == NULL)
        {
            DbgPrint("Can not Allocate resoures for packet");
            break;
        }

        //
        //  将其转化为NDIS_BUFFER.
        //
        NdisAllocateBuffer(
            &Status,
            &pNdisBuffer,
            RecvBufferPool,
            pDataBuffer,
            DataLength);
        
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DbgPrint("failed to allocate Ndis Buffer");
            break;
        }

        NdisAllocatePacket(&Status, &pNdisPacket, pAdapt->RecvPacketPoolHandle);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            DbgPrint("failed to alloc NDIS packet");
            break;
        }

        NDIS_SET_PACKET_STATUS(pNdisPacket, 0);
        SECLAB_RCV_PKT_TO_ORIGINAL_BUFFER(pNdisPacket) = NULL;

        NdisChainBufferAtFront(pNdisPacket, pNdisBuffer);

        *ppDataBuffer = pDataBuffer;

        break;
    }
    while (FALSE);

    if (pNdisPacket == NULL)
    {
        //
        //  Clean up
        //
        if (pNdisBuffer != NULL)
        {
            NdisFreeBuffer(pNdisBuffer);
        }

        if (pDataBuffer != NULL)
        {
            NdisFreeMemory(pDataBuffer, 0, 0);
        }
    }

    return (pNdisPacket);
}


VOID
SecLabFreeReceivePacket(
	PADAPT                          pAdapt,
    IN PNDIS_PACKET                 pNdisPacket
    )
/*++

Routine Description:

    释放所有与接收包相关的资源。如果这是一个本地拷贝，将此包释放到接收包池
	。否则释放到miniport.

Arguments:
    pNdisPacket - 指向要释放的包的指针

Return Value:

    None

--*/
{
    PNDIS_BUFFER        pNdisBuffer;
    UINT                TotalLength;
    UINT                BufferLength;
    PUCHAR              pCopyData;

    if (NdisGetPoolFromPacket(pNdisPacket) ==pAdapt->RecvPacketPoolHandle)
    {
        //
        //  This is a local copy.
        //
		
#ifdef NDIS51
		    NdisGetFirstBufferFromPacketSafe(
            pNdisPacket,
            &pNdisBuffer,
            (PVOID *)&pCopyData,
            &BufferLength,
            &TotalLength,
            NormalPagePriority);
#else
            NdisGetFirstBufferFromPacket(
            pNdisPacket,
            &pNdisBuffer,
            (PVOID *)&pCopyData,
            &BufferLength,
            &TotalLength);
#endif

        if(BufferLength != TotalLength || pNdisBuffer==NULL || pCopyData==NULL)
		{
			DbgPrint("Error! Failed in Free a Packet");
			DbgBreakPoint();
		}

        NdisFreePacket(pNdisPacket);

        NdisFreeBuffer(pNdisBuffer);

        NdisFreeMemory(pCopyData,0,0);

    }
    else
    {
        NdisReturnPackets(&pNdisPacket, 1);
    }
}
 
VOID
ClearQueues(PADAPT pAdapt)
{
	PLIST_ENTRY         pRcvPacketEntry;
    PNDIS_PACKET        pRcvPacket;

    NdisAcquireSpinLock(&PacketListLock);
    
    while (!IsListEmpty(&PacketList))
    {
        //
        //  Get the first queued receive packet
        //
        pRcvPacketEntry = PacketList.Flink;
        RemoveEntryList(pRcvPacketEntry);

        PacketCount--;

        NdisReleaseSpinLock(&PacketListLock);

        pRcvPacket = SECLAB_LIST_ENTRY_TO_RCV_PKT(pRcvPacketEntry);

        SecLabFreeReceivePacket(pAdapt, pRcvPacket);

        NdisAcquireSpinLock(&PacketListLock);
    }

    NdisReleaseSpinLock(&PacketListLock);

}



//
//发送相关函数的实现、
//
NTSTATUS
SecLabSendPacket(
                 PADAPT			         pAdapt,
				 IN PDEVICE_OBJECT       DeviceObject,
				 IN PIRP                 pIrp
				 )
{
	PIO_STACK_LOCATION      pIrpSp;
    ULONG                   FunctionCode;
    ULONG                   DataLength;
    NTSTATUS                NtStatus=STATUS_INVALID_HANDLE;
    NDIS_STATUS             Status;
    
    PNDIS_PACKET                pNdisPacket;
    PNDIS_BUFFER                pNdisBuffer;
//  SECLAB_ETH_HEADER UNALIGNED *pEthHeader;

	PUCHAR      pData;
	ULONG        i;

#ifdef NDIS51
    PVOID                   CancelId;
#endif

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    pNdisPacket = NULL;
	pNdisBuffer = NULL;

    do
    {
        if (pAdapt == NULL)
        {
            DbgPrint("Write: FileObject not yet associated with a device\n");
            NtStatus = STATUS_INVALID_HANDLE;
            break;
        }

        //
        // 检查发送数据包的长度
        //

		DataLength=pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
		
        if (DataLength > MAX_SEND_PACKETLEN )
        {
            DbgPrint("Write: Open data length larger than max frame size\n");

            NtStatus = STATUS_UNSUCCESSFUL;
            break;
        }

        //
        //  构造一个发送封包
        //

		if(pAdapt->SendPacketPoolHandle==NULL)
		{
			DbgPrint("The Packet Pool should not be NULL");
			DbgBreakPoint();
			break;
		}

	    do
		{
			 //其实怎么样分配内存是无关紧要的，忘记资源的释放也不是很严重的
			 //最多导致内存支出过多。关键是不能让程序引用不存在的内存，这样会
			 //造成崩溃，比如重复释放内存。崩溃的另一大可能是在Dispatch>=passive
			 //级别上调用非分页内存。
		     //pData=(PUCHAR)ExAllocatePool(NonPagedPool,DataLength);
			 NdisAllocateMemoryWithTag((PVOID *)(&pData), DataLength,'lceS');

		     if(pData==NULL)
			 {
			      DbgPrint("Can not allocate pool for send");
			      break;
			 }
		     //RtlCopyMemory(pData,pIrp->AssociatedIrp.SystemBuffer,DataLength);
             NdisMoveMemory(pData,pIrp->AssociatedIrp.SystemBuffer,DataLength);

             //
             //  将其转化为NDIS_BUFFER.
             //
             NdisAllocateBuffer(
                 &Status,
                 &pNdisBuffer,
                 SendBufferPool,
                 pData,
                 DataLength);
        
              if (Status != NDIS_STATUS_SUCCESS)
			  {
                   DbgPrint("failed to allocate Ndis Buffer");
                   break;
			  }		

              NdisAllocatePacket(&Status, &pNdisPacket, pAdapt->SendPacketPoolHandle);

              if (Status != NDIS_STATUS_SUCCESS)
			  {
                   DbgPrint("failed to alloc NDIS packet");
                   break;
			  }

              NDIS_SET_PACKET_STATUS(pNdisPacket, 0);
              pNdisBuffer->Next = NULL;
              NdisChainBufferAtFront(pNdisPacket, pNdisBuffer);
			  
		      pNdisPacket->Private.Head->Next=NULL;
		      pNdisPacket->Private.Tail=NULL;

              break;
		}
        while (FALSE);

        if (pNdisPacket == NULL || pNdisBuffer==NULL)
		{
            //
            //  Clean up
            //
            if (pNdisBuffer != NULL)
			{
                NdisFreeBuffer(pNdisBuffer);
			}

            if (pData != NULL)
			{
                NdisFreeMemory(pData, 0, 0);
			}
		}

        IoMarkIrpPending(pIrp);
		NtStatus = STATUS_PENDING;
        pIrp->IoStatus.Status = STATUS_PENDING;
        //
        //  初始化封包中的标志符。当标志符值为0时此包被释放
        //
        SECLAB_SEND_PKT_RSVD(pNdisPacket)->RefCount = 1;

#ifdef NDIS51

        //
        //  NDIS 5.1 supports cancelling sends. We set up a cancel ID on
        //  each send packet (which maps to a Write IRP), and save the
        //  packet pointer in the IRP. If the IRP gets cancelled, we use
        //  NdisCancelSendPackets() to cancel the packet.
        //

        CancelId = SECLAB_GET_NEXT_CANCEL_ID();
        NDIS_SET_PACKET_CANCEL_ID(pNdisPacket, CancelId);
        pIrp->Tail.Overlay.DriverContext[0] = (PVOID)pAdapt;
        pIrp->Tail.Overlay.DriverContext[1] = (PVOID)pNdisPacket;

		NdisInterlockedIncrement(&PendedSendCount);

		NdisAcquireSpinLock(&WriteIrpLock);
        InsertTailList(&PendedWritesList, &pIrp->Tail.Overlay.ListEntry);
        IoSetCancelRoutine(pIrp, SecLabCancelWrite);
		NdisReleaseSpinLock(&WriteIrpLock);

#endif // NDIS51
               
        //
        //  创建一个指针从packet回指向IRP
        //
        SECLAB_IRP_FROM_SEND_PKT(pNdisPacket) = pIrp;

		//
		//创建三个信号，以便在发送完成例程中指示此包
		//
		SECLAB_SIGNAL1_FROM_SEND_PKT(pNdisPacket)=SIGNAL1;
        SECLAB_SIGNAL2_FROM_SEND_PKT(pNdisPacket)=SIGNAL2;
		SECLAB_SIGNAL3_FROM_SEND_PKT(pNdisPacket)=SIGNAL3;

		//
		//发包
		//
        NdisSendPackets(pAdapt->BindingHandle, &pNdisPacket, 1); 
    }

    while (FALSE);

    if (NtStatus != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = NtStatus;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);	
    }

    return (NtStatus);
}



#ifdef NDIS51

VOID
SecLabCancelWrite(
    IN PDEVICE_OBJECT               pDeviceObject,
    IN PIRP                         pIrp
    )
/*++

Routine Description:

    Cancel a pending write IRP. This routine attempt to cancel the NDIS send.

Arguments:

    pDeviceObject - pointer to our device object
    pIrp - IRP to be cancelled

Return Value:

    None

--*/
{
    PADAPT                      pAdapt;
    PLIST_ENTRY                 pIrpEntry;
    PNDIS_PACKET                pNdisPacket;

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    //
    //  一个封包代表一个写IRP.
    //
    pNdisPacket = NULL;

    pAdapt = (PADAPT) pIrp->Tail.Overlay.DriverContext[0];
    if(pAdapt==NULL)
	{
		DbgPrint("The Adapt handle is NULL");
		DbgBreakPoint();
	}

	DbgPrint("once enter the cancel routain");

    //
    //  在写IRP阻塞队列中定位要取消的IRP. 发送完成例程可能已经运行并将其释放
    //
    NdisAcquireSpinLock(&WriteIrpLock);

    for (pIrpEntry = PendedWritesList.Flink;
         pIrpEntry != &PendedWritesList;
         pIrpEntry = pIrpEntry->Flink)
    {
        if (pIrp == CONTAINING_RECORD(pIrpEntry, IRP, Tail.Overlay.ListEntry))
        {
            pNdisPacket = (PNDIS_PACKET) pIrp->Tail.Overlay.DriverContext[1];

            //
            //  将此等待IRP代表的封包结构中的RefCount置1.这样在它
            //  完成之前就不会被释放或重用了
			//
            SECLAB_REF_SEND_PKT(pNdisPacket);
            break;
        }
    }

    NdisReleaseSpinLock(&WriteIrpLock);

    if (pNdisPacket != NULL)
    {
        //
        //  Either the send completion routine hasn't run, or we got a peak
        //  at the IRP/packet before it had a chance to take it out of the
        //  pending IRP queue.
        //
        //  We do not complete the IRP here - note that we didn't dequeue it
        //  above. This is because we always want the send complete routine to
        //  complete the IRP. And this in turn is because the packet that was
        //  prepared from the IRP has a buffer chain pointing to data associated
        //  with this IRP. Therefore we cannot complete the IRP before the driver
        //  below us is done with the data it pointed to.
        //

        //
        //  Request NDIS to cancel this send. The result of this call is that
        //  our SendComplete handler will be called (if not already called).
        //
        NdisCancelSendPackets(
            pAdapt->BindingHandle,
            NDIS_GET_PACKET_CANCEL_ID(pNdisPacket)
            );
        
        //
        //  It is now safe to remove the reference we had placed on the packet.
        //
        SECLAB_DEREF_SEND_PKT(pNdisPacket);
    }
    //
    //  else the send completion routine has already picked up this IRP.
    //
}

#endif // NDIS51

NDIS_STATUS
SecLabDoRequest(
    IN PADAPT			            pAdapt,
    IN NDIS_REQUEST_TYPE            RequestType,
    IN NDIS_OID                     Oid,
    IN PVOID                        InformationBuffer,
    IN UINT                         InformationBufferLength,
    OUT PUINT                       pBytesProcessed
    )
{
	SECLAB_REQUEST              ReqContext;
    PNDIS_REQUEST               pNdisRequest = &ReqContext.Request;
    NDIS_STATUS                 Status;

    NdisInitializeEvent(&ReqContext.ReqEvent);
    ReqContext.Signal=REQUEST_SIGNAL;

    pNdisRequest->RequestType = RequestType;
    
    switch (RequestType)
    {
        case NdisRequestQueryInformation:
            pNdisRequest->DATA.QUERY_INFORMATION.Oid = Oid;
            pNdisRequest->DATA.QUERY_INFORMATION.InformationBuffer =
                                    InformationBuffer;
            pNdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength =
                                    InformationBufferLength;
            break;

        case NdisRequestSetInformation:
            pNdisRequest->DATA.SET_INFORMATION.Oid = Oid;
            pNdisRequest->DATA.SET_INFORMATION.InformationBuffer =
                                    InformationBuffer;
            pNdisRequest->DATA.SET_INFORMATION.InformationBufferLength =
                                    InformationBufferLength;
            break;

        default:
            break;
    }

    NdisRequest(&Status,
                pAdapt->BindingHandle,
                pNdisRequest);
    

    if (Status == NDIS_STATUS_PENDING)
    {
        NdisWaitEvent(&ReqContext.ReqEvent, 0);
        Status = ReqContext.Status;
    }

    if (Status == NDIS_STATUS_SUCCESS)
    {
        *pBytesProcessed = (RequestType == NdisRequestQueryInformation)?
                            pNdisRequest->DATA.QUERY_INFORMATION.BytesWritten:
                            pNdisRequest->DATA.SET_INFORMATION.BytesRead;
    }

    return (Status);
}
NTSTATUS
SecLabQueryOidValue(
   IN PADAPT			           pAdapt,
   OUT PVOID                       pDataBuffer,
   IN  ULONG                       BufferLength,
   OUT PULONG                      pBytesWritten
    )
{
	NDIS_STATUS             Status;
    PSECLAB_QUERY_OID       pQuery;
    NDIS_OID                Oid;

    Oid = 0;

    do
    {
        if (BufferLength < sizeof(SECLAB_QUERY_OID))
        {
            Status = NDIS_STATUS_BUFFER_TOO_SHORT;
            break;
        }

        pQuery = (PSECLAB_QUERY_OID)pDataBuffer;
        Oid = pQuery->Oid;
		if(Oid == MY_OID_802_3_CURRENT_ADDRESS)
		{
			Oid = OID_802_3_CURRENT_ADDRESS;
		}

        Status = SecLabDoRequest(
                    pAdapt,
                    NdisRequestQueryInformation,
                    Oid,
                    &pQuery->Data[0],
                    BufferLength - FIELD_OFFSET(SECLAB_QUERY_OID, Data),
                    pBytesWritten);

        if (Status == NDIS_STATUS_SUCCESS)
        {
            *pBytesWritten += FIELD_OFFSET(SECLAB_QUERY_OID, Data);
        }

    }
    while (FALSE);

    return (Status);
}
