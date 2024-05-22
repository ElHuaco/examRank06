A more elegant way to examRank06
=================================

## Allowed Functions:
- write, close, select, socket, accept, listen, send, recv, bind
- malloc, realloc, free, calloc, bzero, memset
- atoi, sprintf, strlen, strcpy, strcat, strstr
- exit

## Optimizations
  1. `recv()` with relatively small buffer size, then add to cache if no newline was found, this way you don't need a huge buffer.
  2. Iterate only once through every char in the cache and received buffers, so that huge lines or files with huge amounts of `\n` don't bite you.
  3. Client list struct to avoid looping over unused file descriptors, or using global variables for id, or caches.
  4. No need to rewind the list every time.

Results
---------------------
It's able to send quite quickly, to several clients, the infamous files for the GNL exam: those of ~5MB and 100K lines, and 12 lines of ~100KB each.
