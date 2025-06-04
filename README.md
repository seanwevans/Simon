# Multi-threaded Web Server in C

This project is a lightweight multi-threaded web server implemented in C. It supports basic HTTP request handling, serving static files, and logging client interactions. The server is capable of processing multiple requests concurrently through worker threads.

## Features
- **Multi-threaded architecture**: Efficient handling of concurrent client connections.
- **HTTP/1.0 and HTTP/1.1 support**: Basic request parsing and response generation.
- **Static file serving**: Serves files with appropriate MIME types.
- **Chunked Transfer Encoding**: Supports chunked HTTP responses for large files.
- **Logging**: Records errors, connections, and messages to log files.
- **Queue-based request handling**: Manages incoming connections with a semaphore-based queue.

## File Structure
```
.
├── logging.c      # Logging functions for messages and errors
├── main.c         # Entry point, initializes and starts the server
├── parseutf.c     # UTF-8/UTF-16/UTF-32 validation logic
├── queue.c        # Client queue management with mutex and semaphore
├── request.c      # HTTP request validation and file sending
├── server.c       # Core server logic: socket creation, listening, and threading
├── server.h       # Header file with server configuration and function prototypes
├── utils.c        # Utility functions (argument parsing, MIME type detection)
└── README.md      # Project documentation
```

`parseutf8.c` was an earlier alternative for UTF validation. The file
is no longer maintained and has been removed from the build in favor of
`parseutf.c`, which provides the current validation routines.

## How to Build and Run
### Prerequisites
- GCC (GNU Compiler Collection)
- POSIX-compliant operating system (Linux/Mac)

### Build
```bash
gcc -o webserver main.c server.c queue.c request.c logging.c utils.c parseutf.c -lpthread
```

### Run
```bash
./webserver <filename> [port] [core_count] [num_threads]
```
- `filename`: The default file to serve (e.g., index.html).
- `port` (optional): The port on which the server will listen (default: 8080).
- `core_count` (optional): Number of CPU cores to normalize load against (default: 16).
- `num_threads` (optional): Number of worker threads to spawn (default: 8).

### Example
```bash
./webserver index.html 8080
```

## How It Works
1. **Startup**: `main.c` parses command-line arguments and initializes the server.
2. **Thread Management**: `server.c` creates worker threads to handle incoming connections.
3. **Client Handling**: Incoming clients are queued (`queue.c`), and worker threads pop connections to process requests.
4. **Request Processing**: Requests are validated (`request.c`) and served with appropriate files or error responses.
5. **Logging**: All events and errors are logged using `logging.c`.

## Configuration
Default configurations can be modified in `server.h`:
```c
#define DEFAULT_PORT 8080
#define BUFFER_SIZE 2048
#define MAX_QUEUE_SIZE 1024
```

## Error Handling
- 400 Bad Request: Invalid HTTP request format.
- 404 Not Found: Requested file does not exist.
- 500 Internal Server Error: Generic error for unhandled exceptions.

## Future Improvements
- Implement HTTPS support.
- Add dynamic content generation (CGI or FastCGI support).
- Enhance logging with more detailed analytics.

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

