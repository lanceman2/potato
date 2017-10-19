
extern
char *poTime_get(char *buf, size_t bufLen);

extern
double poTime_getDouble();

// Don't use this in the server.  It will eat system resources.
extern
double poTime_getRealDouble();
