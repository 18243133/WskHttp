/*
 * Copyright (c) 2014 Matt Fichman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, APEXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "WskHttp/Common.hpp"
#include "WskHttp/WskHttp.hpp"


namespace WskHttp {

Result send(Request const& request) {
    // Send an HTTP request.  Auto-fill the content-length headers.
    std::string const string = str(request); 
    
	std::string port;
    WskSocket socket;

    if (request.uri().scheme() == "http") {
        port = "80";
    } else {
        return Result(STATUS_PROTOCOL_NOT_SUPPORTED, nullptr);
    }
    if (request.uri().port().length()) {
        port = request.uri().port();
    }

	NTSTATUS status;

	ANSI_STRING host_ansi, port_ansi;
	UNICODE_STRING host_unicode, port_unicode;

	RtlInitAnsiString(&host_ansi, request.uri().host().c_str());
	status = RtlAnsiStringToUnicodeString(&host_unicode, &host_ansi, true);
	if (!NT_SUCCESS(status)) {
		return Result(status, nullptr);
	}

	RtlInitAnsiString(&port_ansi, port.c_str());
	status = RtlAnsiStringToUnicodeString(&port_unicode, &port_ansi, true);
	if (!NT_SUCCESS(status)) {
		return Result(status, nullptr);
	}

	status = socket.connect(&host_unicode, &port_unicode);
	RtlFreeUnicodeString(&host_unicode);
	RtlFreeUnicodeString(&port_unicode);

	if (!NT_SUCCESS(status)) {
		return Result(status, nullptr);
	}

	status = socket.send(string.c_str(), string.size());
    if (!NT_SUCCESS(status)) {
        return Result(status, nullptr);
    }

    std::vector<char> buffer(16384); // 16 KiB
    std::stringstream ss;
    
    SIZE_T size = buffer.size();
    for (;;) {
        status = socket.recv(&buffer[0], &size);
        if (!NT_SUCCESS(status)) {
            return Result(status, nullptr);
        }
        if (size == 0) {
            break;
        }
        ss.write(&buffer[0], size);
    }

	return Result(STATUS_SUCCESS, ss.str());
}

Result get(std::string const& path, std::string const& data) {
    // Shortcut for simple GET requests
    Request request;
    request.methodIs(Request::METHOD_GET);
    request.uriIs(Uri(path));
    request.dataIs(data);
	return send(request);
}

Result post(std::string const& path, std::string const& data) {
    // Shortcut for simple POST requests
    Request request;
    request.methodIs(Request::METHOD_POST);
    request.uriIs(Uri(path));
    request.dataIs(data);
    return send(request);
}

std::string str(Request::Method method) {
    switch (method) {
    case Request::METHOD_GET: return "GET";
    case Request::METHOD_HEAD: return "HEAD";
    case Request::METHOD_POST: return "POST";
    case Request::METHOD_PUT: return "PUT";
    case Request::METHOD_DELETE: return "DELETE";
    case Request::METHOD_TRACE: return "TRACE";
    case Request::METHOD_CONNECT: return "CONNECT";
    default: assert(!"unknown request method");
    }
    return "";
}

std::string str(Request const& request) {
    // Serialize a request to a string
    std::stringstream ss;
    auto path = request.path().empty() ? "/" : request.path();
    ss << str(request.method()) << ' ' << path << " HTTP/1.1\n";
    ss << Headers::HOST << ": " << request.uri().host() << "\n";
    ss << Headers::CONTENT_LENGTH << ": " << request.data().size() << "\n";
    ss << Headers::CONNECTION << ": close\n";
    ss << Headers::ACCEPT_ENCODING << ": identity\n";
    for(auto header : request.headers()) {
        ss << header.first << ": " << header.second << "\n";
    }
    ss << "\n";
    ss << request.data();
    return ss.str();
}

NTSTATUS startup() { return WskSocket::startup(); }
VOID cleanup() { WskSocket::cleanup(); }

}
