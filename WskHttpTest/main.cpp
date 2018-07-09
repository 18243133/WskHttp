#include <ntddk.h>
#include <WskHttp/WskHttp.hpp>


VOID DriverUnload(
	PDRIVER_OBJECT driver_obj
) {
	UNREFERENCED_PARAMETER(driver_obj);
}

EXTERN_C
NTSTATUS DriverEntry(
	PDRIVER_OBJECT driver_obj,
	PUNICODE_STRING registry_path
) {
	UNREFERENCED_PARAMETER(registry_path);

	driver_obj->DriverUnload = DriverUnload;

	WskSocket::startup();

	{
		WskHttp::get("d");
		WskSocket socket;
		UNICODE_STRING host = RTL_CONSTANT_STRING(L"192.168.0.105");
		UNICODE_STRING port = RTL_CONSTANT_STRING(L"19730");
		socket.connect(&host, &port);
		char str[] = "hello";
		socket.send(str, sizeof(str));
		char buf[100] = { 0 };
		SIZE_T size = sizeof(buf);
		socket.recv(buf, &size);
		DbgPrint("recv: %s\n", buf);
	}

	WskSocket::cleanup();

	return STATUS_SUCCESS;
}