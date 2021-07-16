`recv()` with defined buffer size, then add to cache if no newline was found, this way you don't need an inelegantly huge buffer.
