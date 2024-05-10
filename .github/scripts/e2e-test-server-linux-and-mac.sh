#!/bin/bash

## Example run command
# ./e2e-test-server-linux-and-mac.sh '../../examples/build/server' './e2e-test.py'

# Check for required arguments
if [[ $# -ne 2 ]]; then
    echo "Usage: $0 <path_to_binary> <path_to_python_file>"
    exit 1
fi

rm /tmp/python-file-execution-res.log /tmp/server.log

BINARY_PATH=$1
PYTHON_FILE_EXECUTION_PATH=$2

# Random port to ensure it's not used
min=10000
max=11000
range=$((max - min + 1))
PORT=$((RANDOM % range + min))

# Start the binary file
"$BINARY_PATH" 127.0.0.1 $PORT >/tmp/server.log &

pid=$!

if ! ps -p $pid >/dev/null; then
    echo "server failed to start. Logs:"
    cat /tmp/server.log
    exit 1
fi

# Wait for a few seconds to let the server start
sleep 3

# Run the curl commands
response1=$(curl --connect-timeout 60 -o /tmp/python-file-execution-res.log -s -w "%{http_code}" --location "http://127.0.0.1:$PORT/execute" \
    --header 'Content-Type: application/json' \
    --data '{
        "file_execution_path": "'$PYTHON_FILE_EXECUTION_PATH'"
    }')

error_occurred=0
if [[ "$response1" -ne 200 ]]; then
    echo "The python file execution curl command failed with status code: $response1"
    cat /tmp/python-file-execution-res.log
    error_occurred=1
fi

if [[ "$error_occurred" -eq 1 ]]; then
    echo "Server test run failed!!!!!!!!!!!!!!!!!!!!!!"
    echo "Server Error Logs:"
    cat /tmp/server.log
    kill $pid
    exit 1
fi

echo "----------------------"
echo "Log python runtime:"
cat /tmp/server.log


echo "Server test run successfully!"

# Kill the server process
kill $pid
