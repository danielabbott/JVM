#ifndef _endian_h_
#define _endian_h_

static inline unsigned short SwapEndiannessS (unsigned short i) {
	return ((i & 0xff) << 8) | (i >> 8);
}

static inline unsigned SwapEndiannessI (unsigned i) {
	return ((i & 0xff) << 24) | ((i & 0xff00) << 8) | ((i & 0xff0000) >> 8) | ((i & 0xff000000) >> 24);
}

#endif