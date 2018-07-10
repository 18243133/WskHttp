#include "WskHttp/WskHttp.hpp"


VOID DriverUnload(
	PDRIVER_OBJECT driver_object
) {
	UNREFERENCED_PARAMETER(driver_object);
}

EXTERN_C
NTSTATUS DriverEntry(
	PDRIVER_OBJECT  driver_object,
	PUNICODE_STRING registry_path
) {
	UNREFERENCED_PARAMETER(registry_path);
	driver_object->DriverUnload = DriverUnload;

	NTSTATUS status;

	status = WskHttp::startup();
	if (!NT_SUCCESS(status)) {
		DbgPrint("WskHttp::startup error %X\n", status);
		return status;
	}

	auto result_get = WskHttp::get("http://192.168.0.105:8000");
	if (NT_SUCCESS(result_get.status())) {
		DbgPrint("%s\n", result_get.response()->data().c_str());
	} else {
		DbgPrint("WskHttp::get error %X\n", result_get.status());
	}

	WskHttp::cleanup();

	return STATUS_SUCCESS;
}