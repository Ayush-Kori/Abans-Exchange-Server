ABX Mock Exchange Client & Server Setup
Overview
This project consists of two main parts:

Client (client.cpp) - A C++ program that connects to a server and streams packets, handles missing packets, and saves the order book to a JSON file.
Server (main.js) - A Node.js server that simulates the exchange server. It provides the necessary data packets to the client upon request.
Prerequisites
C++ Development Environment:

Install g++ to compile the C++ client.
For Ubuntu:

bash
Copy
Edit
sudo apt update
sudo apt install build-essential
Node.js Environment:

Install Node.js to run the server.
For Ubuntu:

bash
Copy
Edit
sudo apt update
sudo apt install nodejs npm
Libraries:

The client code uses the nlohmann/json library. You can install it using the following command:
bash
Copy
Edit
sudo apt-get install nlohmann-json3-dev
The server is built using the Node.js environment, so you’ll need to have npm installed for the necessary dependencies.
Setting up and Running the Project
1. Start the Server (main.js)
First, clone or download the main.js file from this repository.
Navigate to the project directory and install any dependencies for the server:
bash
Copy
Edit
npm install
Run the server:
bash
Copy
Edit
node main.js
The server will listen for incoming connections on port 3000 by default, sending packets as per the mock exchange protocol.
2. Compile and Run the Client (client.cpp)
Navigate to the Client Directory:

Open a terminal window and navigate to the folder containing the client.cpp file.
Compile the Client:

Use the following g++ command to compile the client.cpp into an executable named client:
bash
Copy
Edit
g++ -o client client.cpp -lnlohmann_json
This will generate an executable named client in the same directory.

Run the Client:

Start the client by running:
bash
Copy
Edit
./client
Interacting with the Client:

The client will display a menu with the following options:

Stream All Packets: Retrieves the full order book from the server and saves it to output.json.
Resend Specific Packet: Allows you to enter a sequence number of a missing packet to request it from the server.
Exit: Exits the client.
After starting the client, it will prompt you to select an action. For example, if you select option 1, the client will attempt to stream all available packets and save the data to a file named output.json.

Example Flow
Start the Server:

bash
Copy
Edit
node main.js
Compile the Client:

bash
Copy
Edit
g++ -o client client.cpp -lnlohmann_json
Run the Client:

bash
Copy
Edit
./client
Menu Options:

Option 1: Stream all packets (This will retrieve the order book and save it to a JSON file).
Option 2: Resend a specific missing packet (If you know a missing sequence number, you can enter it here).
Option 3: Exit the client.
Sample Output (when choosing option 1):

bash
Copy
Edit
=== ABX Mock Exchange Client ===
1. Stream All Packets (Retrieve Full Order Book)
2. Resend Specific Packet (Request Missing Data)
3. Exit
Enter your choice: 1
Order book saved to output.json
output.json will  come
