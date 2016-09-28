httpserver is an experimental simple http server. It can accept "GET" requests, making it acceptable to serve static web pages.
It works on windows and linux, and is written in C, using the operating system's native APIs to accomplish its tasks.  
NOTE: this software is not suitable for serving real web pages in a production environment, it is merely an educational tool useful for learing network programming in C.

Features:  
Multi-threaded design: spawning a new thread to handle each incoming request (making it possible to serve multiple clients at the same time).  
Server-side includes: special syntax is supported in a .html file to include other files server side. Example  
`####includes/header.inc.html`  
