#include "WskHttp/WskSocket.hpp"


#define SYNC(irp, event, completion_routine, timeout, func, ...)\
	(\
		IoSetCompletionRoutine(irp, completion_routine, event, true, true, true),\
		func(__VA_ARGS__, irp),\
		KeWaitForSingleObject(event, Executive, KernelMode, false, timeout)\
	)

#define SECOND(n) -10*1000*1000*(n);


const WSK_CLIENT_DISPATCH WskSocket::kClientDispatch = {
	MAKE_WSK_VERSION(1,0),	// Use WSK version 1.0
	0,						// Reserved
	nullptr					// WskClientEvent callback not required for WSK version 1.0
};

WSK_REGISTRATION WskSocket::registration_;
WSK_CLIENT_NPI WskSocket::client_npi_;
WSK_PROVIDER_NPI WskSocket::provider_npi_;


NTSTATUS CompletionRoutine(PDEVICE_OBJECT reserved, PIRP irp, PVOID ctx) {
	UNREFERENCED_PARAMETER(reserved);
	UNREFERENCED_PARAMETER(irp);
	_Analysis_assume_(ctx != nullptr);

	KeSetEvent((PKEVENT)ctx, IO_NO_INCREMENT, false);

	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS WskSocket::startup() {
	NTSTATUS status;

	client_npi_.Dispatch = &kClientDispatch;
	status = WskRegister(&client_npi_, &registration_);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	status = WskCaptureProviderNPI(&registration_,
		WSK_INFINITE_WAIT, &provider_npi_);
	if (!NT_SUCCESS(status)) {
		WskDeregister(&registration_);
		return status;
	}

	return STATUS_SUCCESS;
}

VOID WskSocket::cleanup() {
	WskReleaseProviderNPI(&registration_);
	WskDeregister(&registration_);
}

NTSTATUS WskSocket::connect(PUNICODE_STRING host, PUNICODE_STRING port) {
	NTSTATUS status = STATUS_SUCCESS;
	PIRP irp = nullptr;
	KEVENT event;
	PADDRINFOEXW remote_addr_info = nullptr;
	SOCKADDR_IN local_addr;
	LARGE_INTEGER timeout;

	KeInitializeEvent(&event, SynchronizationEvent, false);

	IN4ADDR_SETANY(&local_addr);

	timeout.QuadPart = SECOND(5);


	irp = IoAllocateIrp(1, false);
	if (!irp) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto ret;
	}


	// DNS
	status = SYNC(irp, &event, CompletionRoutine, &timeout,
		provider_npi_.Dispatch->WskGetAddressInfo,
			provider_npi_.Client,
			host,
			port,
			NS_ALL,
			nullptr,
			nullptr,
			&remote_addr_info,
			nullptr,
			nullptr
	);

	if (status == STATUS_TIMEOUT) {
		IoCancelIrp(irp);
		goto ret;
	}

	if (!NT_SUCCESS(irp->IoStatus.Status)) {
		status = irp->IoStatus.Status;
		goto ret;
	}


	// connect
	IoReuseIrp(irp, STATUS_SUCCESS);

	status = SYNC(irp, &event, CompletionRoutine, &timeout,
		provider_npi_.Dispatch->WskSocketConnect,
			provider_npi_.Client,
			SOCK_STREAM,
			IPPROTO_TCP,
			(PSOCKADDR)&local_addr,
			(PSOCKADDR)remote_addr_info->ai_addr,
			0,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr
	);

	if (status == STATUS_TIMEOUT) {
		IoCancelIrp(irp);
		goto ret;
	}

	if (!NT_SUCCESS(irp->IoStatus.Status)) {
		status = irp->IoStatus.Status;
		goto ret;
	}


	socket_ = (PWSK_SOCKET)irp->IoStatus.Information;


ret:
	if (irp) {
		IoFreeIrp(irp);
	}

	if (remote_addr_info) {
		provider_npi_.Dispatch->WskFreeAddressInfo(
			provider_npi_.Client,
			remote_addr_info
		);
	}

	return status;
}

NTSTATUS WskSocket::close() {
	NTSTATUS status = STATUS_SUCCESS;
	KEVENT event;
	PIRP irp = nullptr;
	PWSK_PROVIDER_CONNECTION_DISPATCH dispatch;
	LARGE_INTEGER timeout;

	if (!socket_) {
		status = STATUS_OBJECTID_NOT_FOUND;
		goto ret;
	}

	KeInitializeEvent(&event, SynchronizationEvent, false);
	dispatch = (PWSK_PROVIDER_CONNECTION_DISPATCH)socket_->Dispatch;
	timeout.QuadPart = SECOND(5);

	irp = IoAllocateIrp(1, false);
	if (!irp) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto ret;
	}

	status = SYNC(irp, &event, CompletionRoutine, &timeout,
		dispatch->Basic.WskCloseSocket,
			socket_
	);

	if (status == STATUS_TIMEOUT) {
		IoCancelIrp(irp);
		goto ret;
	}

	if (!NT_SUCCESS(irp->IoStatus.Status)) {
		status = irp->IoStatus.Status;
		goto ret;
	}


	socket_ = nullptr;


ret:
	if (irp) {
		IoFreeIrp(irp);
	}

	return status;
}

WskSocket::~WskSocket() {
	close();
}

NTSTATUS WskSocket::send(CONST VOID *data, SIZE_T size) {
	NTSTATUS status = STATUS_SUCCESS;
	KEVENT event;
	PIRP irp = nullptr;
	PWSK_PROVIDER_CONNECTION_DISPATCH dispatch;
	WSK_BUF wsk_buf;
	PMDL mdl = nullptr;
	PVOID buf = nullptr;
	LARGE_INTEGER timeout;

	if (!socket_) {
		status = STATUS_OBJECTID_NOT_FOUND;
		goto ret;
	}

	dispatch = (PWSK_PROVIDER_CONNECTION_DISPATCH)socket_->Dispatch;
	KeInitializeEvent(&event, SynchronizationEvent, false);
	timeout.QuadPart = SECOND(5);

	buf = ExAllocatePool(NonPagedPool, size);
	if (!buf) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto ret;
	}

	memcpy(buf, data, size);

	mdl = IoAllocateMdl(buf, (ULONG)size, false, false, NULL);
	if (!mdl) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto ret;
	}

	MmBuildMdlForNonPagedPool(mdl);
	wsk_buf.Length = size;
	wsk_buf.Offset = 0;
	wsk_buf.Mdl = mdl;

	irp = IoAllocateIrp(1, false);
	if (!irp) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto ret;
	}

	status = SYNC(irp, &event, CompletionRoutine, &timeout,
		dispatch->WskSend,
			socket_,
			&wsk_buf,
			WSK_FLAG_NODELAY
	);

	if (status == STATUS_TIMEOUT) {
		IoCancelIrp(irp);
		goto ret;
	}

	status = irp->IoStatus.Status;


ret:
	if (mdl) {
		IoFreeMdl(mdl);
	}

	if (buf) {
		ExFreePool(buf);
	}

	if (irp) {
		IoFreeIrp(irp);
	}

	return status;
}

NTSTATUS WskSocket::recv(PVOID data, SIZE_T *size) {
	NTSTATUS status = STATUS_SUCCESS;
	KEVENT event;
	PIRP irp = nullptr;
	PWSK_PROVIDER_CONNECTION_DISPATCH dispatch;
	WSK_BUF wsk_buf;
	PMDL mdl = nullptr;
	PVOID buf = nullptr;
	LARGE_INTEGER timeout;

	if (!socket_) {
		status = STATUS_OBJECTID_NOT_FOUND;
		goto ret;
	}

	dispatch = (PWSK_PROVIDER_CONNECTION_DISPATCH)socket_->Dispatch;
	KeInitializeEvent(&event, SynchronizationEvent, false);
	timeout.QuadPart = SECOND(5);

	buf = ExAllocatePool(NonPagedPool, *size);
	if (!buf) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto ret;
	}


	mdl = IoAllocateMdl(buf, (ULONG)*size, false, false, NULL);
	if (!mdl) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto ret;
	}

	MmBuildMdlForNonPagedPool(mdl);
	wsk_buf.Length = *size;
	wsk_buf.Offset = 0;
	wsk_buf.Mdl = mdl;

	irp = IoAllocateIrp(1, false);
	if (!irp) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto ret;
	}

	status = SYNC(irp, &event, CompletionRoutine, &timeout,
		dispatch->WskReceive,
			socket_,
			&wsk_buf,
			0
	);

	if (status == STATUS_TIMEOUT) {
		IoCancelIrp(irp);
		goto ret;
	}

	status = irp->IoStatus.Status;
	
	if (NT_SUCCESS(status)) {
		*size = irp->IoStatus.Information;
		memcpy(data, buf, *size);
	}


ret:
	if (mdl) {
		IoFreeMdl(mdl);
	}

	if (buf) {
		ExFreePool(buf);
	}

	if (irp) {
		IoFreeIrp(irp);
	}

	return status;
}
