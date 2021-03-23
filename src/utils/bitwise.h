#ifndef BITWISE_H_INCLUDED
#define BITWISE_H_INCLUDED

#define GETBIT(x,n) (((int)x < 1) ? 0 : ((x >> (n - 1)) & 1))
#define SETBIT(x,n,v) x ^= (-v ^ x) & (1UL << (n - 1))

#endif // BITWISE_H_INCLUDED
