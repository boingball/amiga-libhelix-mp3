/* Temporary AAC diagnostic: replace TNS filtering with a no-op. */
#include "aac/coder.h"

#ifdef AMIGA_AAC_DISABLE_TNS_TEST
int TNSFilter(AACDecInfo *aacDecInfo, int ch)
{
    (void)aacDecInfo;
    (void)ch;
    return 0;
}
#endif
