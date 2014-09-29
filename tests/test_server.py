#!/usr/bin/env python

try:
    from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler

except ImportError:
    from http.server import HTTPServer, BaseHTTPRequestHandler


class StoreHandler(BaseHTTPRequestHandler):
    __db = {}

    def do_GET(self):
        try:
            if self.__redirected_to_lower():
                return

            s = self.__db[self.path[1:]]
            self.send_response(200)

            self.send_header("Content-Type", "text/plain")
            self.send_header("Content-Length", len(s))
            self.send_header("Content-Encoding", "unknown")
            self.end_headers()

            self.wfile.write(s)

        except KeyError:
            self.send_response(404)
            self.send_header("Content-Length", 0)
            self.end_headers()

    def do_HEAD(self):
        if self.path[1:] in self.__db:
            self.send_response(200)

        else:
            self.send_response(404)

        self.send_header("Content-Length", 0)
        self.end_headers()

    def do_PUT(self):
        ctp = self.headers.get('content-type')

        if ctp is not None and ctp != "text/plain":
            self.send_response(400)
            self.send_header("Content-Length", 0)
            self.end_headers()
            return

        sz = self.headers.get('content-length')

        self.__db[self.path[1:]] = self.rfile.read(int(sz) if sz is not None
                                                   else 0)
        self.send_response(201)
        self.send_header("Content-Length", 0)
        self.end_headers()

    def do_DELETE(self):
        try:
            del self.__db[self.path[1:]]

        except KeyError:
            pass

        self.send_response(204)
        self.send_header("Content-Length", 0)
        self.end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.__set_allowed()

    def do_POST(self):
        if self.__redirected_to_lower():
            return

        self.__not_allowed()

    def __redirected_to_lower(self):
        pe = self.path.lower()

        if self.path != pe:
            self.send_response(302)
            self.send_header("Location", ("http://%s:%d" + pe) %
                             self.server.server_address)
            self.end_headers()

            return True
        else:
            return False

    def __not_allowed(self):
        self.send_response(405)
        self.__set_allowed()

    do_PATCH = __not_allowed

    def __set_allowed(self):
        self.send_header("Allow", "GET, PUT, DELETE")
        self.send_header("Content-Length", 0)
        self.end_headers()

    def do_ECHO(self):
        content_length = self.headers.get('content-length')
        sz = int(content_length) if content_length is not None else 0

        self.send_response(200)

        for h in self.headers.headers:
            if h[:2].lower() == "x-":
                self.wfile.write(h)

        self.send_header("Content-Length", sz)
        self.end_headers()

        while sz > 4096:
            self.wfile.write(self.rfile.read(4096))
            sz -= 4096

        self.wfile.write(self.rfile.read(sz))


def main():
    port = 8080
    server = HTTPServer(('localhost', port), StoreHandler)
    server.serve_forever()


if __name__ == "__main__":
    import sys
    main(*sys.argv[1:])
