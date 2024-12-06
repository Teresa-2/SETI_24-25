#!/bin/bash

# Indirizzo del server e porta
SERVER="localhost"
PORT=80
SLEEP=1.0

# Invia una richiesta HTTP al server
# Questo test non segnala la presenza di client diversi, sono tutti lo stesso client

#sleep $SLEEP;

#atteso: 403 NOT MODIFIED 
(echo -e "HEAD / HTTP/1.0\r\nIf-Modified-Since Mon, 11 Nov 2025 00:00:00 GMT\r\nCookie: UserID=3 \r\n"; sleep $SLEEP;) | telnet $SERVER $PORT

# (echo -e "HEAD / HTTP/1.0\r\nIf-Modified-Since Mon, 11 Nov 2020 00:00:00 GMT\r\nCookie: UserID=3 \r\n"; sleep $SLEEP;) | telnet $SERVER $PORT
# da sempre 200 OK

#(echo -e "GET / HTTP/1.0\r\n Cookie: UserID=3"; sleep $SLEEP;) | telnet $SERVER $PORT