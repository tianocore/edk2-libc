"""
This is a sample HTTP echo server that echos the command / data
coming from the client.
Here the data is received from client through GET request in the
form of parameter of GET request.
The parameter is extracted and sent back to the client
in the response body.

Note that this server sample application needs to be run
on a system booted to OS.
"""

import os
import socket
import sys
from http.server import BaseHTTPRequestHandler, HTTPServer
from http.client import parse_headers


class MyHTTPRequestHandler(BaseHTTPRequestHandler):
    """HTTP request handler class"""

    # Handle GET command
    def do_GET(self):
        print("path {}".format(self.path))
        path = self.path.split("?")[0]
        param_name = self.path.split("?")[1].split("=")[0]
        param_value = self.path.split("?")[1].split("=")[1]
        print("param name {} value = {}".format(param_name, param_value))
        if path == "/echo":
            self.send_response(200)
            self.send_header("Content-type", "text/plain")
            self.end_headers()
            self.wfile.write(
                bytes("{}={}".format(param_name, param_value), encoding="utf-8")
            )
        else:
            print("invalid request")
            self.send_response(204)


def run():
    try:
        print("Starting the server...")
        server_address = (socket.gethostbyname(socket.gethostname()), 80)
        print("Server address :", server_address)
        httpd = HTTPServer(server_address, MyHTTPRequestHandler)
        print("\n\n\nPress CTRL+C to exit server application")
        httpd.serve_forever()
    except KeyboardInterrupt as exp:
        print("KeyboardInterrupt")
        sys.exit(0)
    except Exception as exp:
        print(str(exp))


if __name__ == "__main__":
    if os.name == "edk2":
        print("HTTP echo server not supported on EDk2")
        sys.exit(0)
    run()
