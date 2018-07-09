#pragma once
#include <ntddk.h>
#include <wsk.h>

class WskSocket {
public:
	static NTSTATUS startup();
	static VOID cleanup();

	WskSocket() : socket_(nullptr) {}
	~WskSocket();
	NTSTATUS connect(CONST PWSTR host, CONST PWSTR port);
	NTSTATUS close();
	NTSTATUS send(CONST PVOID data, ULONG size);
	NTSTATUS recv(PVOID data, ULONG *size);

private:
	const static WSK_CLIENT_DISPATCH kClientDispatch;
	static WSK_REGISTRATION registration_;
	static WSK_CLIENT_NPI client_npi_;
	static WSK_PROVIDER_NPI provider_npi_;

	PWSK_SOCKET socket_;
};