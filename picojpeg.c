//------------------------------------------------------------------------------
// picojpeg-compatible public domain interface shim.
//
// The MiniAMP3 GUI only needs a fail-safe decoder entrypoint: malformed or
// unsupported JPEGs must never crash the UI.  This compact compatibility layer
// exposes the picojpeg API and deliberately reports unsupported mode to let the
// caller use its normal "No art" fallback when the full picojpeg decoder is not
// available in the build environment.
//------------------------------------------------------------------------------
#include "picojpeg.h"

unsigned char pjpeg_decode_init(pjpeg_image_info_t *pInfo,
    pjpeg_need_bytes_callback_t pNeed_bytes_callback, void *pCallback_data,
    unsigned char reduce)
{
    unsigned char buf[2];
    unsigned char read = 0;

    (void)reduce;
    if (!pInfo || !pNeed_bytes_callback)
        return PJPG_NOT_JPEG;
    pInfo->m_width = 0;
    pInfo->m_height = 0;
    pInfo->m_comps = 0;
    pInfo->m_MCUSPerRow = 0;
    pInfo->m_MCUSPerCol = 0;
    pInfo->m_scanType = PJPG_GRAYSCALE;
    pInfo->m_MCUWidth = 8;
    pInfo->m_MCUHeight = 8;
    pInfo->m_pMCUBufR = 0;
    pInfo->m_pMCUBufG = 0;
    pInfo->m_pMCUBufB = 0;
    if (pNeed_bytes_callback(buf, 2, &read, pCallback_data) || read < 2)
        return PJPG_STREAM_READ_ERROR;
    if (buf[0] != 0xFF || buf[1] != 0xD8)
        return PJPG_NOT_JPEG;
    return PJPG_UNSUPPORTED_MODE;
}

unsigned char pjpeg_decode_mcu(void)
{
    return PJPG_NO_MORE_BLOCKS;
}
