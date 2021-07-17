A more elegant way to examRank06
=================================
  1. `recv()` with relatively small buffer size, then add to cache if no newline was found, this way you don't need a huge buffer.
  2. Iterate only once through every char in the buffer, so that huge lines or files with huge amounts of `\n` don't bite you.
  3. Client list struct to avoid looping over unused file descriptors, using global variables for id, or caches.
  4. No need to rewind the list every time.

Results
---------------------
It's able to send quite quickly, to several clients, the infamous files for the GNL exam: those of ~5MB and 100K lines, and 12 lines of ~100KB each.
