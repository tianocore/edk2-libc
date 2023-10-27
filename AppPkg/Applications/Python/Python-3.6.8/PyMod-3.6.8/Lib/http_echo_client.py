"""
This is a sample HTTP echo client sends data to the server
and gets echoed data from the server in response body
and prints the same to the console.

Note: This application needs to be run from UEFI shell using
the Python UEFI interpreter.
"""

import sys
import time
from http import client
from http.client import HTTPException
import traceback

_max_retries = 10
_retry_count = 0


def _print_usage():
    print("Sample http echo client application")
    print("Usage:")
    print("python.efi http_echo_client.py <ServerIPAddress>")


if len(sys.argv) != 2:
    _print_usage()
    sys.exit(0)

if sys.argv[1] == "-h":
    _print_usage()
    sys.exit(0)

http_server = sys.argv[1]
while True:
    try:
        name = input("Enter the parameter name:")
        value = input("Enter parameter value:")
        print("Connecting to server to send a get request with following parameter")
        print("{}={}".format(name, value))
        # replace space with %20
        value = value.replace(" ", "%20")
        conn = client.HTTPConnection(http_server)
        # Send GET request with some data
        conn.request("GET", "/echo?{}={}".format(name, value))
        rsp = conn.getresponse()
        if rsp.status == 204:
            print("No content")
            break
        elif rsp.status == 200:
            data_received = rsp.read()
            # replace %20 with space character before displaying to console
            data_received = data_received.replace(b"%20", b" ")
            print("from server:{}".format(data_received))
            conn.close()
            print("Closing the connection")
            break
        else:
            print("Invalid response code {}".format(rsp.status))
            conn.close()
            print("Closing the connection")
            break
    except HTTPException as exp:
        print("Got exception while connecting to server : {}".format(exp))
        traceback.print_exc()
        break
    except ConnectionRefusedError as exp:
        print("Got exception while connecting to server : {}".format(exp))
        print("Check & start the server, if it is not started")
        print(
            "Retrying connection after 10 seconds, retry count = {}".format(
                _retry_count + 1
            )
        )
        if _retry_count == _max_retries:
            print(
                "Exceeded max retries {} exiting the application".format(_max_retries)
            )
            break
        time.sleep(10)
        _retry_count += 1
