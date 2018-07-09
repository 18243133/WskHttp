#include <ntddk.h>
#include <WskHttp/WskSocket.h>


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
		WskSocket socket;
		socket.connect(L"192.168.0.105", L"19730");
		char str[] = "hello";
		socket.send(str, sizeof(str));
		char buf[100] = { 0 };
		ULONG size = sizeof(buf);
		socket.recv(buf, &size);
		DbgPrint("recv: %s\n", buf);
	}

	WskSocket::cleanup();

	return STATUS_SUCCESS;
}