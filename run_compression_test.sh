#!/bin/bash
echo "MRDesktop H.265 Compression Test"
echo "=================================="
echo

SERVER_EXE="build/debug/MRDesktopServer"
CLIENT_EXE="build/debug/MRDesktopConsoleClient"

if [ ! -f "$SERVER_EXE" ]; then
    echo "ERROR: Server not found at $SERVER_EXE"
    echo "Please build the project first"
    exit 1
fi

if [ ! -f "$CLIENT_EXE" ]; then
    echo "ERROR: Client not found at $CLIENT_EXE"
    echo "Please build the project first"
    exit 1
fi

echo "Starting server in test mode..."
"$SERVER_EXE" --test &
SERVER_PID=$!

echo "Waiting 3 seconds for server to start..."
sleep 3

echo "Starting client in test mode with H.265 compression..."
"$CLIENT_EXE" --test --compression=h265
CLIENT_EXIT_CODE=$?

echo
echo "Cleaning up server process..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo
if [ $CLIENT_EXIT_CODE -eq 0 ]; then
    echo "H.265 COMPRESSION TEST PASSED!"
    echo "Compressed frames were successfully encoded and decoded."
else
    echo "H.265 COMPRESSION TEST FAILED with exit code $CLIENT_EXIT_CODE"
fi

echo
exit $CLIENT_EXIT_CODE