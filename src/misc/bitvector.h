#ifndef BITVECTOR_H
#define BITVECTOR_H

typedef unsigned long bits_t;

#define BITVECTOR(name, n) bits_t name[BIT_WORDS(n)]

#define NBITS (sizeof(bits_t) * 8)
#define BIT_WORDS(n) (((n)+NBITS-1)/NBITS)
#define BIT_WORD(b,i) ((b)[(i)/NBITS])
#define BIT_MASK(i) (1 << ((i)%NBITS))

#define BIT_TST(b,i) (BIT_WORD(b,i) & BIT_MASK(i))
#define BIT_SET(b,i) (BIT_WORD(b,i) |= BIT_MASK(i))
#define BIT_CLR(b,i) (BIT_WORD(b,i) &= ~BIT_MASK(i))
#define BIT_TOG(b,i) (BIT_WORD(b,i) ^= BIT_MASK(i))

#endif /* BITVECTOR_H */
