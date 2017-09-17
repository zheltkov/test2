# test2
client server file transfer

Command for build server.

cd test2_server
mkdir build
cd build
cmake ..
cmake --build .

To run:

./test2_server [--port <server port>]  

(Default port 8877)

Command for build client.

cd test2_client
mkdir build
cd build
cmake ..
cmake --build .

To run:

./test2_client --server_ip <server ip> [--port <server port>] --file <path to transfer file>

(Default port 8877)



