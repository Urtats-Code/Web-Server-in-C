## C Concurrent Web Server

### Project Overview

This project demonstrates a simple yet effective concurrent web server built entirely in C. It leverages TCP sockets for communication and efficiently serves the following file types:

- HTML
- CSS
- JavaScript
- Images

### Key Features

Concurrent Request Handling: Handles multiple client requests simultaneously for improved performance.
HTTP GET Support: Responds to GET requests, the fundamental method for retrieving web resources.
Customizable File Size Limit: Easily adjust the maximum supported file size through the BUFFER_1MB variable.


### Limitations

- GET Requests Only: The server's scope is intentionally limited to handling GET requests.
- File Size Restriction: Currently, the maximum renderable file size is 1MB. (This can be modified if you change the BUFFER_1MB value )


### Getting Started

Download or Clone: Obtain the project files directly or clone the repository.


Choose Your Path:
- Precompiled Binary: Execute the provided server.out file.
- Compile Yourself: Use the command gcc -o server.out server.c
Usage Notes

After starting the server, point your web browser to the appropriate address (e.g., http://localhost:8080, where 8080 is your chosen port).

Let me know if you'd like any additional refinements or sections added!
