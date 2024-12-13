#!/bin/bash

# Indirizzo del server e porta
SERVER="localhost"
PORT=8000
SLEEP=0.1

send_request() {
  echo -e "$1" | telnet $SERVER $PORT
}

# Invia tutte le richieste HTTP al server in una sola sessione di telnet
(
echo -e "GET / HTTP/1.1\r\nIf-Modified-Since Mon, 11 Nov 2024 00:00:00 GMT\r\nCookie: UserID=10\r\n" #atteso: 304
echo -e "GET / HTTP/1.1\r\nIf-Modified-Since Mon, 11 Nov 1999 00:00:00 GMT\r\nCookie: UserID=1\r\n" #atteso: 200
#sleep $SLEEP
echo -e "GET / HTTP/1.1\r\nCookie: UserID=16546546\r\nIf-Modified-Since: Mon, 11 Nov 2024 00:00:00 GMT\r\n"
echo -e "GET / HTTP/1.1\r\nIf-Modified-Since Mon, 11 Nov 1999 00:00:00 GMT\r\nCookie: UserID=1\r\n"

sleep $SLEEP
echo -e "GET / HTTP/1.1\r\nCookie: UserID=16\r\nIf-Modified-Since: Mon, 11 Nov 2024 00:00:00 GMT\r\n"
echo -e "HEAD / HTTP/1.1\r\nCookie: UserID=1\r\n"
#sleep $SLEEP
echo -e "HEAD / HTTP/1.0\r\nCookie: UserID=1\r\n"
#sleep $SLEEP
) | telnet $SERVER $PORT

send_request "GET / HTTP/1.0\r\nCookie: UserID=2\r\n"
#sleep $SLEEP

send_request "HEAD / HTTP/1.0\r\nCookie: UserID=2\r\n"
#sleep $SLEEP

send_request "HEAD / HTTP/1.0\r\n"
#sleep $SLEEP

send_request "HEAD / HTTP/1.0\r\nCookie: UserID=2\r\n"
#sleep $SLEEP