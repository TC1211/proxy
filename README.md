# Proxy
Custom Web Proxy
## Group Members
- Grace Chen
- TC Dong
## Implementation Details
#### proxy.c
In our proxy.c file, which is the main file, our main functionality for handling client/server communications is housed. We handle socket-related and message parsing-related tasks.
_**The executable file that is printed is called "proxy".**_
#### cache.c
cache.c is the file that holds our definition of the cache entry, as well as various helper functions in navigating/modifying/updating the metadata of the cache.
#### http.c
http.c originally held more functions, but at the moment, it only holds char *host\_to\_ipaddr, which performs the getaddrinfo call and returns the IP address associated with the inputted URL.
## Sources Consulted
Piazza/http class slides
Boyang, Professor Benson
Online resources:
- StackOverflow
- Beej's guide on getaddrinfo
