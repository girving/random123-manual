RNGNxW_TPL(philox, 2, 32)
RNGNxW_TPL(philox, 4, 32)
#if R123_USE_PHILOX_64BIT
RNGNxW_TPL(philox, 2, 64)
RNGNxW_TPL(philox, 4, 64)
#endif
RNGNxW_TPL(threefry, 2, 32)
RNGNxW_TPL(threefry, 2, 64)
RNGNxW_TPL(threefry, 4, 32)
RNGNxW_TPL(threefry, 4, 64)
#if R123_USE_AES_NI
RNGNxW_TPL(ars, 4, 32)
RNGNxW_TPL(aesni, 4, 32)
#endif
