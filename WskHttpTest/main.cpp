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

	WskHttp::startup();

	{
		auto result = WskHttp::get("http://r.qzone.qq.com/fcg-bin/cgi_get_score.fcg?mask=7&uins=123456");
		if (NT_SUCCESS(result.status())) {
			DbgPrint("%d\n", result.response()->status());
			DbgPrint("%s\n", result.response()->data().c_str());
		} else {
			DbgPrint("%X\n", result.status());
		}
	}

	WskHttp::cleanup();

	return STATUS_SUCCESS;
}