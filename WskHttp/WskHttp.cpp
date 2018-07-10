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
    
	uint16_t port;
    WskSocket socket;

    if (request.uri().scheme() == "http") {
        port = 80;
    } else {
        return Result(STATUS_PROTOCOL_NOT_SUPPORTED, nullptr);
    }
    if (request.uri().port()) {
        port = request.uri().port();
    }

	NTSTATUS status;

	ANSI_STRING host_ansi;
	UNICODE_STRING host_unicode;

	RtlInitAnsiString(&host_ansi, request.uri().host().c_str());
	status = RtlAnsiStringToUnicodeString(&host_unicode, &host_ansi, true);
	if (!NT_SUCCESS(status)) {
		return Result(status, nullptr);
	}

	status = socket.connect(&host_unicode, port);
	RtlFreeUnicodeString(&host_unicode);

	if (!NT_SUCCESS(status)) {
		return Result(status, nullptr);
	}

	status = socket.send(string.c_str(), string.size());
    if (!NT_SUCCESS(status)) {
        return Result(status, nullptr);
    }

    std::vector<char> buffer(16384); // 16 KiB
    std::string s;
    
    SIZE_T size = buffer.size();
    for (;;) {
        status = socket.recv(&buffer[0], &size);
        if (!NT_SUCCESS(status)) {
            return Result(status, nullptr);
        }
        if (size == 0) {
            break;
        }
        s.append(&buffer[0], size);
    }

	return Result(STATUS_SUCCESS, s);
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
    std::string s;
    auto path = request.path().empty() ? "/" : request.path();

	s.append(str(request.method()));
	s.append(" ");
	s.append(path);
	s.append(" HTTP/1.1\n");

	s.append(Headers::HOST);
	s.append(": ");
	s.append(request.uri().host());
	s.append("\n");
	char size[128];
	sprintf_s(size, "%zd", request.data().size());
	s.append(Headers::CONTENT_LENGTH);
	s.append(": ");
	s.append(size);
	s.append("\n");

	s.append(Headers::CONNECTION);
	s.append(": close\n");

	s.append(Headers::CONNECTION);
	s.append(": identity\n");

    for(auto header : request.headers()) {
		s.append(header.first);
		s.append(": ");
		s.append(header.second);
		s.append("\n");
    }
	s.append("\n");
	s.append(request.data());
    return s;
}

NTSTATUS startup() { return WskSocket::startup(); }
VOID cleanup() { WskSocket::cleanup(); }

}
