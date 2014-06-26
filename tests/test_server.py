#!/usr/bin/env python

from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler


class StoreHandler(BaseHTTPRequestHandler):
    __db = {}

    def do_GET(self):
        try:
            s = self.__db[self.path[1:]]
            self.send_response(200)

            self.send_header("Content-Type", "text/plain")
            self.end_headers()

            self.wfile.write(s)

        except KeyError:
            self.send_response(404)

    def do_HEAD(self):
        if self.path[1:] in self.__db:
            self.send_response(200)

        else:
            self.send_response(404)

    def do_PUT(self):
        ctp = self.headers.getheader('content-type')

        if ctp is not None and ctp != "text/plain":
            self.send_response(400)
            return

        sz = self.headers.getheader('content-length')

        self.__db[self.path[1:]] = self.rfile.read(int(sz) if sz is not None
                                                   else 0)
        self.send_response(201)

    def do_DELETE(self):
        try:
            del self.__db[self.path[1:]]

        except KeyError:
            pass

        self.send_response(204)

    def do_OPTIONS(self):
        self.send_response(200)
        self.__set_allowed()

    def __not_allowed(self):
        self.send_response(405)
        self.__set_allowed()

    do_POST = __not_allowed
    do_PATCH = __not_allowed

    def __set_allowed(self):
        self.send_header("Allow", "GET, PUT, DELETE")
        self.end_headers()


def main():
    port = 8080
    server = HTTPServer(('localhost', port), StoreHandler)
    server.serve_forever()


if __name__ == "__main__":
    import sys
    main(*sys.argv[1:])
