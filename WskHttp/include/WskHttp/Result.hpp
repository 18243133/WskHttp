#include "WskHttp/Response.hpp"
#include <ntddk.h>

namespace WskHttp {
class Result {
public:
	Result(NTSTATUS status, std::string const& text) {
		status_ = status;
		response_ = NT_SUCCESS(status) ? new Response(text) : nullptr;
	}

	~Result() {
		delete response_;
	}

	NTSTATUS status() {
		return status_;
	}

	Response *response() {
		return response_;
	}

private:
	NTSTATUS status_;
	Response *response_ = nullptr;
};
}