/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

/**************************************************************************************
 * Fixed-point MP3 decoder
 * Jon Recker (jrecker@real.com), Ken Cooke (kenc@real.com)
 * June 2003
 *
 * subband.c - subband transform (synthesis filterbank implemented via 32-point DCT
 *               followed by polyphase filter)
 **************************************************************************************/

#include "coder.h"
#include "amiga_profile_decode.h"
#include "assembly.h"

void FDCT32FastLowrate(int *x, int *d, int offset, int oddBlock, int gb,
	int stride, int phase)
{
#if defined(AMIGA_M68K) && defined(AMIGA_FAST_POLYPHASE)
	if (stride == 2) {
		FDCT32Half(x, d, offset, oddBlock, gb);
		return;
	}
	if (stride == 4 && MP3ExperimentalFDCT32QuarterEnabled()) {
		FDCT32Quarter(x, d, offset, oddBlock, gb, phase, stride);
		return;
	}
#else
	(void)stride;
#endif
	FDCT32(x, d, offset, oddBlock, gb);
}

/**************************************************************************************
 * Function:    Subband
 *
 * Description: do subband transform on all the blocks in one granule, all channels
 *
 * Inputs:      filled MP3DecInfo structure, after calling IMDCT for all channels
 *              vbuf[ch] and vindex[ch] must be preserved between calls
 *
 * Outputs:     decoded PCM data, interleaved LRLRLR... if stereo
 *
 * Return:      0 on success,  -1 if null input pointers
 **************************************************************************************/
int Subband(MP3DecInfo *mp3DecInfo, short *pcmBuf)
{
	int b;
	HuffmanInfo *hi;
	IMDCTInfo *mi;
	SubbandInfo *sbi;
	clock_t amigaProfileStart;
	int vindex;
	int stride;
	int *vbuf;
	int outputMono;

	/* validate pointers */
	if (!mp3DecInfo || !mp3DecInfo->HuffmanInfoPS || !mp3DecInfo->IMDCTInfoPS || !mp3DecInfo->SubbandInfoPS)
		return -1;

	hi = (HuffmanInfo *)mp3DecInfo->HuffmanInfoPS;
	mi = (IMDCTInfo *)(mp3DecInfo->IMDCTInfoPS);
	sbi = (SubbandInfo*)(mp3DecInfo->SubbandInfoPS);
	(void)hi;

	vindex = sbi->vindex;
	vbuf = sbi->vbuf;
	stride = mp3DecInfo->fastLowrateStride;
	outputMono = (mp3DecInfo->outputMono && mp3DecInfo->nChans == 2);
	if (mp3DecInfo->nChans == 2 && !outputMono) {
		/* stereo */
		if (stride > 1) {
			int phase;
			int lowrateOutputSamps;

			phase = mp3DecInfo->fastLowratePhase;
			lowrateOutputSamps = mp3DecInfo->fastLowrateOutputSamps;
			for (b = 0; b < BLOCK_SIZE; b += 2) {
				int produced;
				int *vbase;
				int *fusedIn[MAX_NCHAN];
				int fusedGb[MAX_NCHAN];

				fusedIn[0] = mi->outBuf[0][b];
				fusedIn[1] = mi->outBuf[1][b];
				fusedGb[0] = mi->gb[0];
				fusedGb[1] = mi->gb[1];
				produced = FusedSynthFastLowrate(pcmBuf, fusedIn, sbi, 2, stride,
					&phase, fusedGb, 0);
				if (produced >= 0) {
					lowrateOutputSamps += produced;
					pcmBuf += produced;
					fusedIn[0] = mi->outBuf[0][b + 1];
					fusedIn[1] = mi->outBuf[1][b + 1];
					produced = FusedSynthFastLowrate(pcmBuf, fusedIn, sbi, 2, stride,
						&phase, fusedGb, 1);
					lowrateOutputSamps += produced;
					pcmBuf += produced;
					vindex = (vindex - 1) & 7;
					continue;
				}

				AMIGA_PROFILE_START(amigaProfileStart);
				FDCT32FastLowrate(mi->outBuf[0][b], vbuf + 0*32, vindex, 0,
					mi->gb[0], stride, phase);
				FDCT32FastLowrate(mi->outBuf[1][b], vbuf + 1*32, vindex, 0,
					mi->gb[1], stride, phase);
				AMIGA_PROFILE_STOP(MP3_DECODE_CORE_PROFILE_SUBBAND_DCT32, amigaProfileStart);
				AMIGA_PROFILE_START(amigaProfileStart);
				vbase = vbuf + vindex;
				produced = PolyphaseStereoFastLowrate(pcmBuf, vbase, polyCoef,
					stride, &phase);
				lowrateOutputSamps += produced;
				pcmBuf += produced;
				AMIGA_PROFILE_STOP(MP3_DECODE_CORE_PROFILE_POLYPHASE, amigaProfileStart);

				AMIGA_PROFILE_START(amigaProfileStart);
				FDCT32FastLowrate(mi->outBuf[0][b + 1], vbuf + 0*32, vindex, 1,
					mi->gb[0], stride, phase);
				FDCT32FastLowrate(mi->outBuf[1][b + 1], vbuf + 1*32, vindex, 1,
					mi->gb[1], stride, phase);
				AMIGA_PROFILE_STOP(MP3_DECODE_CORE_PROFILE_SUBBAND_DCT32, amigaProfileStart);
				AMIGA_PROFILE_START(amigaProfileStart);
				vbase = vbuf + vindex + VBUF_LENGTH;
				produced = PolyphaseStereoFastLowrate(pcmBuf, vbase, polyCoef,
					stride, &phase);
				lowrateOutputSamps += produced;
				pcmBuf += produced;
				AMIGA_PROFILE_STOP(MP3_DECODE_CORE_PROFILE_POLYPHASE, amigaProfileStart);
				vindex = (vindex - 1) & 7;
			}
			mp3DecInfo->fastLowratePhase = phase;
			mp3DecInfo->fastLowrateOutputSamps = lowrateOutputSamps;
		} else {
			for (b = 0; b < BLOCK_SIZE; b += 2) {
				int *vbase;

				AMIGA_PROFILE_START(amigaProfileStart);
				FDCT32(mi->outBuf[0][b], vbuf + 0*32, vindex, 0, mi->gb[0]);
				FDCT32(mi->outBuf[1][b], vbuf + 1*32, vindex, 0, mi->gb[1]);
				AMIGA_PROFILE_STOP(MP3_DECODE_CORE_PROFILE_SUBBAND_DCT32, amigaProfileStart);
				AMIGA_PROFILE_START(amigaProfileStart);
				vbase = vbuf + vindex;
				PolyphaseStereo(pcmBuf, vbase, polyCoef);
				pcmBuf += (2 * NBANDS);
				AMIGA_PROFILE_STOP(MP3_DECODE_CORE_PROFILE_POLYPHASE, amigaProfileStart);

				AMIGA_PROFILE_START(amigaProfileStart);
				FDCT32(mi->outBuf[0][b + 1], vbuf + 0*32, vindex, 1, mi->gb[0]);
				FDCT32(mi->outBuf[1][b + 1], vbuf + 1*32, vindex, 1, mi->gb[1]);
				AMIGA_PROFILE_STOP(MP3_DECODE_CORE_PROFILE_SUBBAND_DCT32, amigaProfileStart);
				AMIGA_PROFILE_START(amigaProfileStart);
				vbase = vbuf + vindex + VBUF_LENGTH;
				PolyphaseStereo(pcmBuf, vbase, polyCoef);
				pcmBuf += (2 * NBANDS);
				AMIGA_PROFILE_STOP(MP3_DECODE_CORE_PROFILE_POLYPHASE, amigaProfileStart);
				vindex = (vindex - 1) & 7;
			}
		}
	} else {
		/* mono */
		if (stride > 1) {
			int phase;
			int lowrateOutputSamps;

			phase = mp3DecInfo->fastLowratePhase;
			lowrateOutputSamps = mp3DecInfo->fastLowrateOutputSamps;
			for (b = 0; b < BLOCK_SIZE; b += 2) {
				int produced;
				int *vbase;
				int *fusedIn[MAX_NCHAN];
				int fusedGb[MAX_NCHAN];

				fusedIn[0] = mi->outBuf[0][b];
				fusedGb[0] = mi->gb[0];
				produced = FusedSynthFastLowrate(pcmBuf, fusedIn, sbi, 1, stride,
					&phase, fusedGb, 0);
				if (produced >= 0) {
					lowrateOutputSamps += produced;
					pcmBuf += produced;
					fusedIn[0] = mi->outBuf[0][b + 1];
					produced = FusedSynthFastLowrate(pcmBuf, fusedIn, sbi, 1, stride,
						&phase, fusedGb, 1);
					lowrateOutputSamps += produced;
					pcmBuf += produced;
					vindex = (vindex - 1) & 7;
					continue;
				}

				AMIGA_PROFILE_START(amigaProfileStart);
				FDCT32FastLowrate(mi->outBuf[0][b], vbuf + 0*32, vindex, 0,
					mi->gb[0], stride, phase);
				AMIGA_PROFILE_STOP(MP3_DECODE_CORE_PROFILE_SUBBAND_DCT32, amigaProfileStart);
				AMIGA_PROFILE_START(amigaProfileStart);
				vbase = vbuf + vindex;
				produced = PolyphaseMonoFastLowrate(pcmBuf, vbase, polyCoef,
					stride, &phase);
				lowrateOutputSamps += produced;
				pcmBuf += produced;
				AMIGA_PROFILE_STOP(MP3_DECODE_CORE_PROFILE_POLYPHASE, amigaProfileStart);

				AMIGA_PROFILE_START(amigaProfileStart);
				FDCT32FastLowrate(mi->outBuf[0][b + 1], vbuf + 0*32, vindex, 1,
					mi->gb[0], stride, phase);
				AMIGA_PROFILE_STOP(MP3_DECODE_CORE_PROFILE_SUBBAND_DCT32, amigaProfileStart);
				AMIGA_PROFILE_START(amigaProfileStart);
				vbase = vbuf + vindex + VBUF_LENGTH;
				produced = PolyphaseMonoFastLowrate(pcmBuf, vbase, polyCoef,
					stride, &phase);
				lowrateOutputSamps += produced;
				pcmBuf += produced;
				AMIGA_PROFILE_STOP(MP3_DECODE_CORE_PROFILE_POLYPHASE, amigaProfileStart);
				vindex = (vindex - 1) & 7;
			}
			mp3DecInfo->fastLowratePhase = phase;
			mp3DecInfo->fastLowrateOutputSamps = lowrateOutputSamps;
		} else {
			for (b = 0; b < BLOCK_SIZE; b += 2) {
				int *vbase;

				AMIGA_PROFILE_START(amigaProfileStart);
				FDCT32(mi->outBuf[0][b], vbuf + 0*32, vindex, 0, mi->gb[0]);
				AMIGA_PROFILE_STOP(MP3_DECODE_CORE_PROFILE_SUBBAND_DCT32, amigaProfileStart);
				AMIGA_PROFILE_START(amigaProfileStart);
				vbase = vbuf + vindex;
				PolyphaseMono(pcmBuf, vbase, polyCoef);
				pcmBuf += NBANDS;
				AMIGA_PROFILE_STOP(MP3_DECODE_CORE_PROFILE_POLYPHASE, amigaProfileStart);

				AMIGA_PROFILE_START(amigaProfileStart);
				FDCT32(mi->outBuf[0][b + 1], vbuf + 0*32, vindex, 1, mi->gb[0]);
				AMIGA_PROFILE_STOP(MP3_DECODE_CORE_PROFILE_SUBBAND_DCT32, amigaProfileStart);
				AMIGA_PROFILE_START(amigaProfileStart);
				vbase = vbuf + vindex + VBUF_LENGTH;
				PolyphaseMono(pcmBuf, vbase, polyCoef);
				pcmBuf += NBANDS;
				AMIGA_PROFILE_STOP(MP3_DECODE_CORE_PROFILE_POLYPHASE, amigaProfileStart);
				vindex = (vindex - 1) & 7;
			}
		}
	}

	sbi->vindex = vindex;
	return 0;
}

#if defined(AMIGA_FUSED_SYNTHESIS) && defined(AMIGA_FAST_POLYPHASE)
static int FusedSynthFastLowrateActive(int nChans, int stride)
{
	return MP3ExperimentalFusedSynthesisEnabled() &&
		(nChans == 1 || nChans == 2) && (stride == 2 || stride == 4);
}

int FusedSynthFastLowrate(short *pcm, int *x[MAX_NCHAN], SubbandInfo *sbi,
	int nChans, int stride, int *phase, int gb[MAX_NCHAN], int oddBlock)
{
	int *vbase;
	int produced;

	if (!FusedSynthFastLowrateActive(nChans, stride))
		return -1;

	/* The experimental backend owns fusedVbuf/fusedVindex.  That keeps the
	 * legacy vbuf/vindex untouched so a single binary can selftest the old and
	 * new synthesis paths without FIFO aliasing.  This initial backend keeps the
	 * externally visible sample phase/count identical while the butterfly/window
	 * implementation remains compile-gated behind AMIGA_FUSED_SYNTHESIS.
	 */
	FDCT32FastLowrate(x[0], sbi->fusedVbuf + 0*32, sbi->fusedVindex, oddBlock,
		gb[0], stride, *phase);
	if (nChans == 2)
		FDCT32FastLowrate(x[1], sbi->fusedVbuf + 1*32, sbi->fusedVindex, oddBlock,
			gb[1], stride, *phase);
	vbase = sbi->fusedVbuf + sbi->fusedVindex + (oddBlock ? VBUF_LENGTH : 0);
	if (nChans == 2)
		produced = PolyphaseStereoFastLowrate(pcm, vbase, polyCoef, stride, phase);
	else
		produced = PolyphaseMonoFastLowrate(pcm, vbase, polyCoef, stride, phase);
	if (oddBlock)
		sbi->fusedVindex = (sbi->fusedVindex - 1) & 7;
	return produced;
}
#else
int FusedSynthFastLowrate(short *pcm, int *x[MAX_NCHAN], SubbandInfo *sbi,
	int nChans, int stride, int *phase, int gb[MAX_NCHAN], int oddBlock)
{
	(void)pcm; (void)x; (void)sbi; (void)nChans; (void)stride; (void)phase;
	(void)gb; (void)oddBlock;
	return -1;
}
#endif
