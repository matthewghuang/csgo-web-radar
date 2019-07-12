# Linux CS:GO Web Radar

This project was made to learn more about game hacking in Linux.

Technologies:
- Express for web server
- WebSockets for communication between web server and client

Dependencies:
- node
    - express
    - ws
- ninja
- LLVM (clang++)
- CMake

Usage:
1. "npm install" in the host folder to install required dependencies
2. Run the web server in the host folder with "node index.js"
3. Make sure the server is up @ localhost:23417
4. Navigate to the provider folder and run the executable under root (make sure CS:GO is open)

Issues:
1. Memory offsets are hardcoded and will become outdated eventually
