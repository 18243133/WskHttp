WskHttp
=======

Windows kernel drivers simple HTTP library for modern C++. Fork of [http](https://github.com/mfichman/http.git). Here's the basic usage:

```cpp

NTSTATUS status;

status = WskHttp::startup();
if (!NT_SUCCESS(status)) {
	DbgPrint("WskHttp::startup error %X\n", status);
	return status;
}

WskHttp::Result result_get = WskHttp::get("http://192.168.0.105:8000");
if (NT_SUCCESS(result_get.status())) {
	DbgPrint("%s\n", result_get.response()->data().c_str());
} else {
	DbgPrint("WskHttp::get error %X\n", result_get.status());
}

WskHttp::Result result_post = WskHttp::post("http://192.168.0.105:8000",
	"something to post"
);
if (NT_SUCCESS(result_post.status())) {
	DbgPrint("%s\n", result_post.response()->data().c_str());
} else {
	DbgPrint("WskHttp::post error %X\n", result_get.status());
}

WskHttp::cleanup();

```


