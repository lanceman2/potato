 
// Using the murmur hash we generate pseudo random numbers.  This is not
// for cryptography.  murmur hash is not for cryptography.

/// \cond SKIP

struct PORandom
{
    uint32_t seed;
    uint32_t counter;
};

/// \endcond

static inline
struct PORandom *poRandom_init(struct PORandom *r, uint32_t seed)
{
    memset(r, 0, sizeof(*r));
    r->seed = seed;
    return r;
}

static inline
uint32_t poRandom_get(struct PORandom *r)
{
    ++r->counter;
    return poMurmurHash(&r->counter, sizeof(uint32_t), r->seed);
}
