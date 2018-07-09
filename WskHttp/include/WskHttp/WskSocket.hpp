#pragma once
#include "WskHttp/Common.hpp"
#include <ntddk.h>
#include <wsk.h>


class WskSocket {
public:
	static NTSTATUS startup();
	static VOID cleanup();

	~WskSocket();
	NTSTATUS connect(PUNICODE_STRING host, PUNICODE_STRING port);
	NTSTATUS close();
	NTSTATUS send(CONST VOID *data, SIZE_T size);
	NTSTATUS recv(PVOID data, SIZE_T *size);

private:
	const static WSK_CLIENT_DISPATCH kClientDispatch;
	static WSK_REGISTRATION registration_;
	static WSK_CLIENT_NPI client_npi_;
	static WSK_PROVIDER_NPI provider_npi_;

	PWSK_SOCKET socket_ = nullptr;
};