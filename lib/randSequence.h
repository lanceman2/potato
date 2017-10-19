
// rlen = length of string returned without '/0' terminator.
// TODO: This is inefficient.  There must be a way to do this with less
// variables and less computation.
//
// This returns a string that is just a function of the what the input buf
// string is.  The sequence is hex encoded.
extern
char *poRandSequence_string(const char *ibuf, char *buf,
        size_t *rlen/*returned*/, size_t minLen, size_t maxLen);
