#include "lwi_private.h"
#include <string.h>
#include <limits.h>

/*
  Good enough for ISO-8859-1 ;-)
*/
EXPORT unsigned int
lwi_key_to_unicode(unsigned int key)
{
    X_LOCK();
    KeySym sym = XKeycodeToKeysym(X.dpy, LWI_KEY_CODE(key), key & 1);
    X_UNLOCK();

    if ((sym >= 0x20 && sym <= 0x7E) || (sym >= 0xA1 && sym <= 0xFF))
        return sym;

    return UINT_MAX;
}
