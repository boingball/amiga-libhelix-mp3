/*
* This file is part of MPEGA.
* Copyright (C) 1998 Stephane Tavenard
* 
* MPEGA is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* MPEGA is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with MPEGA.  If not, see <http://www.gnu.org/licenses/>.
*
*/
/*------------------------------------------------------------------------------

    File    :   MPEGA.C

    Author  :   Stéphane TAVENARD

    $VER:   MPEGA.C  3.45  (20/06/1998)

    (C) Copyright 1997-1998 Stéphane TAVENARD
        All Rights Reserved

    #Rev|   Date   |                      Comment
    ----|----------|--------------------------------------------------------
    0   |19/02/1997| Initial revision                                     ST
    1   |31/03/1997| First Aminet release (V2.0)                          ST
    1   |31/03/1997| Corrected bug in mixing freq                         ST
    2   |05/04/1997| Aminet release (V2.2)                                ST
    3   |06/04/1997| Added MPEG2.5 (not ISO standard)                     ST
    4   |10/04/1997| Added priority option / -p is no longer for play     ST
    5   |16/04/1997| Added AHI support                                    ST
    6   |23/04/1997| Fixed bug for 48KHz freq                             ST
    7   |24/04/1997| Added Seek in stream                                 ST
    8   |30/04/1997| Added AIFF, auto freq div                            ST
    9   |03/05/1997| Use now file access spec of MPEGDEC                  ST
    10  |08/05/1997| Added time counter                                   ST
    11  |13/05/1997| Use AudioMan V0.2                                    ST
    12  |18/05/1997| Use MPEGDEC lib optimized in SUBBAND synth.          ST
    13  |20/05/1997| Added CPU_VERSION in Version string                  ST
    14  |23/05/1997| All Output is redirected to stdout now               ST
    15  |29/05/1997| Wait for filling audio buffer if wanted              ST
    15  |29/05/1997| User can configure bitstream buffer size             ST
    16  |05/06/1997| FPU optimized versions, don't check mpeg at start    ST
    16  |05/06/1997| Corected bad lock when checking file size            ST
    17  |11/06/1997| Added audio buffer stats (-a option)                 ST
    18  |27/06/1997| INTEL version (arrrg)                                ST
    19  |27/06/1997| Added WAVE output                                    ST
    20  |27/06/1997| Added Decoding parameters output                     ST
    21  |06/07/1997| Use new MPEGDEC_CTRL                                 ST
    22  |14/07/1997| Added config file                                    ST
    23  |19/07/1997| Corrected WAVE format                                ST
    24  |19/07/1997| Alternative input reading with Read()                ST
    25  |19/07/1997| Read config first from ENV:, then S:, then PROGDIR:  ST
    26  |23/07/1997| Added filename display option                        ST
    27  |11/08/1997| New Version string format (STYLEGUIDE compliant)     ST
    28  |14/08/1997| Added Random play for playlist (-R option)           ST
    29  |16/08/1997| Added Tag info                                       ST
    30  |17/08/1997| Changed output display (added bold text)             ST
    31  |08/04/1998| Use optims in overflow MPEGAlib, new email, Hooks    ST
    32  |20/06/1998| Use MPEGA library now + options '-e' '-V'            ST
    33  |21/06/1998| Dynamic boost '-y' option                            ST
    34  |21/06/1998| Scaled output '-k' option (with MPEGA.library >=2.0) ST
    35  |22/06/1998| Scale from comment with '-K'                         ST
    36  |23/07/1998| Fixed bug of pri option that don't work in V3.4      ST
    37  |21/12/1998| Added play length (-L<ms> option)                    ST

    ------------------------------------------------------------------------

    Faaast MPEG Audio decoder for layers I,II and III !
    Original code from MPEGA 1.3, completely rewritten...

------------------------------------------------------------------------------*/

#define MPEGA_LIBRARY // #32

#define TIME_STAT

#ifdef AMIGA
#define MPEGAUD_AUDIO // For audio output
#define ASYNC_IO // #9
#include <dos/dos.h>
#include <proto/dos.h>
#include <pragmas/dos_pragmas.h>
#ifdef MPEGA_LIBRARY // #32 Begin
#include <proto/mpega.h>
#endif // #32 End
#endif

#include "DEFS.h"
#ifdef MPEGA_LIBRARY // #32  Begin
#define MPEGDEC_ACCESS_PARAM MPEGA_ACCESS
#define MPEGDEC_FUNC_OPEN  MPEGA_BSFUNC_OPEN
#define MPEGDEC_FUNC_CLOSE MPEGA_BSFUNC_CLOSE
#define MPEGDEC_FUNC_READ  MPEGA_BSFUNC_READ
#define MPEGDEC_FUNC_SEEK  MPEGA_BSFUNC_SEEK

#define MPEGDEC_PCM_SIZE     MPEGA_PCM_SIZE
#define MPEGDEC_MAX_CHANNELS MPEGA_MAX_CHANNELS
#define MPEGDEC_ERR_NODE     MPEGA_ERR_NONE
#define MPEGDEC_ERR_EOF      MPEGA_ERR_EOF
#define MPEGDEC_ERR_BADFRAME MPEGA_ERR_BADFRAME
#define MPEGDEC_ERR_MEM      MPEGA_ERR_MEM
#define MPEGDEC_ERR_NO_SYNC  MPEGA_ERR_NO_SYNC

#define MPEGDEC_STREAM       MPEGA_STREAM
#define MPEGDEC_CTRL         MPEGA_CTRL
#define MPEGDEC_OUTPUT       MPEGA_OUTPUT
#define MPEGDEC_LAYER        MPEGA_LAYER

#define MPEGDEC_open  MPEGA_open
#define MPEGDEC_close MPEGA_close
#define MPEGDEC_decode_frame  MPEGA_decode_frame
#define MPEGDEC_seek  MPEGA_seek
#define MPEGDEC_time  MPEGA_time
#else
#include "MPEGDEC.h" // #32
#endif // #32 End
#include "mpegtag.h" // #29

#ifdef MPEGAUD_AUDIO
#include "AUDIOMAN.h" // #11
//#include <devices/ahi.h>
#endif

#ifdef ASYNC_IO
#define ASIO_REGARGS
#include <libraries/asyncio.h>
//#define _ARGS // #31
#include <clib/asyncio_protos.h>
#endif

#ifdef AMIGA
#include <dos.h>
#include <sys/stat.h>
#include <exec/exec.h>
#include <clib/exec_protos.h>
#include <pragmas/exec_pragmas.h>
static INT8 old_priority = 0;
static INT8 priority = 0;
#endif

#ifdef TIME_STAT
#include <time.h>
#endif

#include <ctype.h>

#ifdef MPEGA_LIBRARY // #32 Begin
   #define CPU_VERSION "Generic"
#else
#ifdef _M68060
#ifdef _M68881
   #define CPU_VERSION "68040/60+FPU"
#else
   #define CPU_VERSION "68040/60"
#endif
#else
#ifdef _M68040
#ifdef _M68881
   #define CPU_VERSION "68040+FPU"
#else
   #define CPU_VERSION "68040"
#endif

#else
#ifdef _M68020
#ifdef _M68881
   #define CPU_VERSION "68020/30+FPU"
#else
   #define CPU_VERSION "68020/30"
#endif

#else
#ifdef _I486
   #define CPU_VERSION "486"
#else
#ifdef _I586
   #define CPU_VERSION "586"
#else
   #define CPU_VERSION "Unknown_CPU"
#endif
#endif

#endif
#endif
#endif

#endif // #32 End

#define FILENAME_SIZE 256

// #19 Begin

typedef enum { // #19
   IFF_TYPE_NONE,
   IFF_TYPE_AIFF,
   IFF_TYPE_WAVE
} IFF_TYPE;

typedef struct {
   UINT32  id;
   UINT32  size;
} IFF_HEADER;

typedef struct {
   UINT16 channels;
   UINT32 samples;
   UINT16 bits;
   INT8 rate[ 10 ];
} AIFF_CHUNK_COMMON;

typedef struct {
   UINT32 offset;
   UINT32 block_size;
} AIFF_CHUNK_DATA;

typedef struct {
   UINT16 type;
   UINT16 channels;
   UINT32 rate;
   UINT32 bytes_per_sec;
   UINT16 pad;
   UINT16 bits;
} WAVE_CHUNK_FORMAT;

#define SWAP16( w ) ((((UINT16)w>>8) & 0x00FF) |\
                     (((UINT16)w<<8) & 0xFF00))
#define SWAP32( l ) ((((UINT32)l>>24) & 0x000000FF) |\
                     (((UINT32)l>>8)  & 0x0000FF00) |\
                     (((UINT32)l<<8)  & 0x00FF0000) |\
                     (((UINT32)l<<24) & 0xFF000000))

#ifdef LITTLE_ENDIAN
#define LSBF_CPU TRUE
#define MSBF_CPU FALSE
#define MSBF16( v ) SWAP16( v )
#define MSBF32( v ) SWAP32( v )
#define LSBF16( v ) ( v )
#define LSBF32( v ) ( v )
#else
#define LSBF_CPU FALSE
#define MSBF_CPU TRUE
#define MSBF16( v ) ( v )
#define MSBF32( v ) ( v )
#define LSBF16( v ) SWAP16( v )
#define LSBF32( v ) SWAP32( v )
#endif

#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d) ((a<<24)|(b<<16)|(c<<8)|(d))
#endif
#define IFF_ID_FORM MAKE_ID( 'F','O','R','M' )
#define IFF_ID_AIFF MAKE_ID( 'A','I','F','F' )
#define IFF_ID_COMM MAKE_ID( 'C','O','M','M' )
#define IFF_ID_SSND MAKE_ID( 'S','S','N','D' )
#define IFF_ID_RIFF MAKE_ID( 'R','I','F','F' )
#define IFF_ID_WAVE MAKE_ID( 'W','A','V','E' )
#define IFF_ID_fmt  MAKE_ID( 'f','m','t',' ' )
#define IFF_ID_data MAKE_ID( 'd','a','t','a' )


typedef struct {
#ifdef LITTLE_ENDIAN
   UINT32 lo;
   UINT32 hi;
#else
   UINT32 hi;
   UINT32 lo;
#endif
} IEEE_DBL;

typedef struct  SANE_EXT_struct {
   UINT32 l1;
   UINT32 l2;
   UINT16 s1;
} SANE_EXT;

typedef struct {
   INT16 freq_div;
   INT16 quality;
   INT32 freq_max;
   BOOL force_mono;

   INT32 stream_buffer_size;
   BOOL header_only;
   BOOL frame_counter;
   UINT32 ms_seek;
   UINT32 ms_length;  // #37
   BOOL time_counter;
   BOOL decoding_params;
   IFF_TYPE iff_type;
   char in_filename[ FILENAME_SIZE ];
   char out_filename[ FILENAME_SIZE ];
   BOOL play_list; // #22
   char cfg_filename[ FILENAME_SIZE ]; // #22
   BOOL cfg_exclude; // #32
   BOOL display_config; // #22
   BOOL dyn_boost; // #33
   INT32 scale_percent; // #34
   BOOL scale_comment; // #35
   BOOL time_used; // #22
   BOOL display_filename; // #26
   BOOL random_play; // #28
   BOOL tag_info; // #29
#ifdef ASYNC_IO // #24
   BOOL async_io;
#endif
#ifdef MPEGAUD_AUDIO
   BOOL play_audio;
   INT16 filter;
   UINT16 mixing;
   UINT32 buffer_time;
   UINT16 volume;
   BOOL use_ahi;
   UINT32 audio_id;
   BOOL audio_query;
   BOOL audio_wait;
   BOOL audio_stat;
   BOOL volume_comment; // #32
#define VOLUME_NOM 64   // #32
#define VOLUME_MAX 256  // #32
#endif
} MPEGA_CFG;

#ifdef AMIGA // #27
#define PROG_DATE __AMIGADATE__
#else
#define PROG_DATE "("__DATE__")"
#endif

#ifdef AMIGA // #30
#define BOLD "\033[1m"
#define NOBOLD "\033[22m"
#define LINE "\033[4m"
#define NOLINE "\033[24m"
#else
#define BOLD
#define NOBOLD
#define LINE
#define NOLINE
#endif

static const char *Version = "$VER:"BOLD"MPEGA 3.5"NOBOLD" "PROG_DATE" ["CPU_VERSION"] (C)1995-1998 Stephane TAVENARD";

static const char *modes[] = { "stereo", "j-stereo", "dual", "mono" };
static const char *dec_qual[] = { "low", "medium", "high" }; // #20
static const char *dec_mode[] = { "?", "mono", "stereo" }; // #20
static const char *out_type[] = { "PCM", "AIFF", "WAVE" }; // #20
static const char *no_yes[] = { "no", "yes" };
#ifdef MPEGAUD_AUDIO
static const char *filter_type[] = { "no", "yes", "auto" }; // #22
#endif

static MPEGDEC_STREAM *mps = NULL;
static MPEGDEC_CTRL mpa_ctrl; // #21
static FILE *out = NULL;
static UINT32 samples = 0;
static BOOL breaked = TRUE;
static BOOL swap_pcm = FALSE; // #18
static MPEGA_CFG global_cfg; // #22

struct Library *MPEGABase = NULL; // #32
static ULONG MPEGAVersion = MPEGA_VERSION; // #34

static void double_to_extended( double *pd, char ps[ 10 ] )
{
   register UINT32 top2bits;
   register IEEE_DBL *p_dbl;
   register SANE_EXT *p_ext;

   p_dbl = (IEEE_DBL *) pd;
   p_ext = (SANE_EXT *) ps;
   top2bits = p_dbl->hi & 0xc0000000;
   p_ext->l1 = ((p_dbl->hi >> 4) & 0x3ff0000) | top2bits;
   p_ext->l1 |= ((p_dbl->hi >> 5) & 0x7fff) | 0x8000;
   p_ext->l2 = (p_dbl->hi << 27) & 0xf8000000;
   p_ext->l2 |= ((p_dbl->lo >> 5) & 0x07ffffff);
   p_ext->s1 = (UINT16)(p_dbl->lo << 11);

   p_ext->l1 = MSBF32( p_ext->l1 );
   p_ext->l2 = MSBF32( p_ext->l2 );
   p_ext->s1 = MSBF16( p_ext->s1 );
}

static int write_iff_chunk( FILE *file, UINT32 id, UINT32 size, void *chunk, UINT32 chunk_size )
{
   IFF_HEADER h;

   h.id = id;
   h.size = size;

   if( fwrite( &h, sizeof( h ), 1, file ) != 1 ) return -1;
   if( chunk ) {
      if( fwrite( chunk, chunk_size, 1, file ) != 1 ) return -1;
   }
   return 0;
}

static int write_iff_header( FILE *file, IFF_TYPE type, INT16 channels, INT32 freq, INT32 samples )
/*----------------------------------------------------------------------------------------------
   Write iff headers to output file
   Return 0 if ok
   swap_pcm variable is set according to file & cpu
*/
{
   UINT32 ctype;
   UINT32 hsize;
   UINT32 sample_size;

   sample_size = channels * samples * sizeof( INT16 );

   switch( type ) {
      case IFF_TYPE_NONE:
         break;
      case IFF_TYPE_AIFF:
         {
            double rate;
            AIFF_CHUNK_COMMON ccommon;
            AIFF_CHUNK_DATA cdata;
            UINT32 ssize;

            swap_pcm = LSBF_CPU;

            ssize = sizeof( AIFF_CHUNK_DATA ) + sample_size;
            hsize = 4 + (8 + sizeof( AIFF_CHUNK_COMMON )) + (8 + ssize);
            ctype = MSBF32( IFF_ID_AIFF );
            ccommon.channels = MSBF16( channels );
            ccommon.samples = MSBF32( samples );
            ccommon.bits = MSBF16( 16 );
            rate = (double)freq;
            double_to_extended( &rate, ccommon.rate );
            cdata.offset = 0;
            cdata.block_size = 0;

            if( fseek( file, 0, SEEK_SET ) != 0 ) return -1;
            write_iff_chunk( file, MSBF32( IFF_ID_FORM ), MSBF32( hsize ), &ctype, 4 );
            write_iff_chunk( file, MSBF32( IFF_ID_COMM ), MSBF32( sizeof( AIFF_CHUNK_COMMON ) ),
                             &ccommon, sizeof( AIFF_CHUNK_COMMON ) );
            write_iff_chunk( file, MSBF32( IFF_ID_SSND ), MSBF32( ssize ),
                             &cdata, sizeof( AIFF_CHUNK_DATA ) );
         }
         break;
      case IFF_TYPE_WAVE:
         {
            WAVE_CHUNK_FORMAT cformat;
            UINT32 bps;

            swap_pcm = MSBF_CPU;

            hsize = 4 + (8 + sizeof( WAVE_CHUNK_FORMAT )) + (8 + sample_size); // #23
            ctype = MSBF32( IFF_ID_WAVE );

            cformat.type = LSBF16( 0x0001 ); // PCM format
            cformat.channels = LSBF16( channels );
            cformat.rate = LSBF32( freq );
            bps = freq * sizeof( INT16 ) * channels;
            cformat.bytes_per_sec = LSBF32( bps );
            cformat.pad = LSBF16( channels );
            cformat.bits = LSBF16( 16 );

            if( fseek( file, 0, SEEK_SET ) != 0 ) return -1;
            write_iff_chunk( file, MSBF32( IFF_ID_RIFF ), LSBF32( hsize ), &ctype, 4 );
            write_iff_chunk( file, MSBF32( IFF_ID_fmt ), LSBF32( sizeof( WAVE_CHUNK_FORMAT ) ),
                             &cformat, sizeof( WAVE_CHUNK_FORMAT ) );
            write_iff_chunk( file, MSBF32( IFF_ID_data ), LSBF32( sample_size ), NULL, 0 );
         }
         break;
      default:
         return -1;
   }
   return 0;
}

// #19 End


static void close_output( void )
{
   if( out ) {
      if( global_cfg.iff_type != IFF_TYPE_NONE ) {
         (void)write_iff_header( out, global_cfg.iff_type,
                                 mps->dec_channels, mps->dec_frequency, samples );
      }
      fclose( out );
      out = NULL;
   }
}

static int break_cleanup( void )
{
   close_output();

   if( mps ) {
#ifdef MPEGAUD_AUDIO
      AUM_close();
#endif
      MPEGDEC_close( mps );
      mps = NULL;
      if( breaked ) printf( "\n** Stopped **\n" );
   }
#ifdef AMIGA
   if( priority != old_priority ) {
      (void)SetTaskPri( FindTask(NULL), old_priority );
   }
#endif
#ifdef MPEGA_LIBRARY // #32 Begin
   if( MPEGABase ) {
      CloseLibrary( MPEGABase );
      MPEGABase = NULL;
   }
#endif // #32 End

   return 1;
}

static void exit_cleanup( void )
{
   (void)break_cleanup();
}

// #9 Begin

#ifdef ASYNC_IO

#if 1 // #31

typedef struct {
   struct AsyncFile *file;
   INT8 *stream_buffer;
   UINT32 stream_pos;
   UINT32 stream_size;
} STREAM_HANDLE;

static ULONG __saveds __asm bs_access_func( register __a0 struct Hook  *hook,
                                            register __a2 APTR          handle,
                                            register __a1 MPEGDEC_ACCESS_PARAM *access ) {
/*----------------------------------------------------------------------------
*/

   switch( access->func ) {

      case MPEGDEC_FUNC_OPEN: {
         STREAM_HANDLE *stream_handle;
         BPTR lock;
         __aligned struct FileInfoBlock fib;

         lock = Lock( access->data.open.stream_name, ACCESS_READ );
         if( !lock ) return NULL;

         if( Examine( lock, &fib ) ) {
            access->data.open.stream_size = (long)fib.fib_Size;
         }

         UnLock( lock );

         stream_handle = (STREAM_HANDLE *)AllocVec( sizeof(STREAM_HANDLE), MEMF_CLEAR | MEMF_PUBLIC );
         if( !stream_handle ) return NULL;
         stream_handle->stream_size = fib.fib_Size;

//         if( player_config.load_ram ) {
         if( 0 ) {
            stream_handle->stream_buffer = AllocVec( stream_handle->stream_size, MEMF_PUBLIC );
         }
         if( stream_handle->stream_buffer ) {
            BPTR fh = Open( access->data.open.stream_name, MODE_OLDFILE );
            if( fh ) {
               stream_handle->stream_size = Read( fh, stream_handle->stream_buffer,
                                                  stream_handle->stream_size );
               Close( fh );
            }
            else {
               FreeVec( stream_handle );
               stream_handle = NULL;
            }
         }
         else {
            stream_handle->file = OpenAsync( access->data.open.stream_name, MODE_READ, access->data.open.buffer_size*2 );
            if( !stream_handle->file ) {
               FreeVec( stream_handle );
               stream_handle = NULL;
            }
         }
         return (ULONG)stream_handle;
      }

      case MPEGDEC_FUNC_CLOSE:

         if( handle ) {
            STREAM_HANDLE *stream_handle = handle;

            if( stream_handle->file ) CloseAsync( stream_handle->file );
            if( stream_handle->stream_buffer ) FreeVec( stream_handle->stream_buffer );
            FreeVec( stream_handle );
         }
         break;

      case MPEGDEC_FUNC_READ: {
         LONG read_size = 0;

         if( handle ) {
            STREAM_HANDLE *stream_handle = handle;

            if( stream_handle->file ) {
               read_size = ReadAsync( stream_handle->file,
                                      access->data.read.buffer, access->data.read.num_bytes );
            }
            else {
               LONG size = stream_handle->stream_size - stream_handle->stream_pos;
               if( access->data.read.num_bytes < size ) {
                  size = access->data.read.num_bytes;
               }
               if( size > 0 ) {
                  CopyMem( stream_handle->stream_buffer + stream_handle->stream_pos,
                           access->data.read.buffer, size );
                  stream_handle->stream_pos += size;
                  read_size = size;
               }
            }
         }

         return (ULONG)read_size;
      }
      case MPEGDEC_FUNC_SEEK: {

         int err = 0;

         if( handle ) {
            STREAM_HANDLE *stream_handle = handle;

            if( stream_handle->file ) {
               err = SeekAsync( stream_handle->file, access->data.seek.abs_byte_seek_pos, MODE_START );
               if( err > 0 ) err = 0;
            }
            else {
               if( access->data.seek.abs_byte_seek_pos < stream_handle->stream_size ) {
                  stream_handle->stream_pos = access->data.seek.abs_byte_seek_pos;
               }
               else err = 1;
            }
         }

         return (ULONG)err;
      }
   }
   return 0;
}

#else // #31 End

INT32 bs_open( char *stream_name, INT32 buffer_size, INT32 *stream_size )
/*-----------------------------------------------------------------------
*/
{
   struct AsyncFile *file_ptr;
   BPTR lock;
   struct FileInfoBlock fib;

   lock = Lock( stream_name, ACCESS_READ ); // #16: ACCESS_READ
   if( lock ) {
      if( Examine( lock, &fib ) ) {
         *stream_size = (long)fib.fib_Size;
      }
      UnLock( lock );
   }

   file_ptr = OpenAsync( stream_name, MODE_READ, buffer_size*2 );

   if( file_ptr && (!lock) ) {
      *stream_size = SeekAsync( file_ptr, 0, MODE_END );
      (void)SeekAsync( file_ptr, 0, MODE_START );
   }

   return (INT32)file_ptr;
}

void bs_close( INT32 handle )
/*---------------------------
*/
{
   if( handle ) CloseAsync( (struct AsyncFile *)handle );
}

INT32 bs_read( INT32 handle, void *buffer, INT32 num_bytes )
/*----------------------------------------------------------
*/
{
   long read_size = -1;

   if( handle ) {
      read_size = ReadAsync( (struct AsyncFile *)handle,
                             buffer, num_bytes );
   }

   return read_size;
}

int bs_seek( INT32 handle, INT32 abs_byte_seek_pos )
/*--------------------------------------------------
*/
{
   int err = 0;

   if( handle ) {
      err = SeekAsync( (struct AsyncFile *)handle, abs_byte_seek_pos, MODE_START );
      if( err > 0 ) err = 0;
   }

   return err;
}

#endif

#endif

// #9 End

int output_pcm( MPEGDEC_STREAM *mpds,
                INT16 *pcm[ 2 ], INT32 count, FILE *out_file )
/*--------------------------------------------------------------------------
   Ouput the current decoded PCM to a file
   Return 0 if Ok
*/
{
   #define PCM_BUFFER_SIZE (MPEGDEC_MAX_CHANNELS*MPEGDEC_PCM_SIZE)
   static INT16 *pcm_buffer = NULL;
   if( !out_file ) return -1;

   if( !pcm_buffer ) {
      pcm_buffer = (INT16 *)malloc( PCM_BUFFER_SIZE * sizeof(INT16) );
      if( !pcm_buffer ) return -1;
   }

   if( swap_pcm ) { // #18
      if( mpds->dec_channels == 2 ) {
         register INT16 *pcm0, *pcm1, *pcmLR;
         register INT16 i;

         pcm0 = pcm[ 0 ];
         pcm1 = pcm[ 1 ];
         pcmLR = pcm_buffer;
         i = count;
         while( i-- ) {
            *pcmLR++ = ((UINT16)*pcm0 >> 8) | ((UINT16)*pcm0 << 8);
            *pcmLR++ = ((UINT16)*pcm1 >> 8) | ((UINT16)*pcm1 << 8);
            pcm0++; pcm1++;
         }
         fwrite( pcm_buffer, 4, count, out_file );
      }
      else {
         register INT16 *pcm0, *pcmLR;
         register INT16 i;

         pcm0 = pcm[ 0 ];
         pcmLR = pcm_buffer;
         i = count;
         while( i-- ) {
            *pcmLR++ = ((UINT16)*pcm0 >> 8) | ((UINT16)*pcm0 << 8);
            pcm0++;
         }
         fwrite( pcm_buffer, 2, count, out_file );
      }
   }
   else if( mpds->dec_channels == 2 ) {
      register INT16 *pcm0, *pcm1, *pcmLR;
      register INT16 i;

      pcm0 = pcm[ 0 ];
      pcm1 = pcm[ 1 ];
      pcmLR = pcm_buffer;
      i = count;
      while( i-- ) {
         *pcmLR++ = *pcm0++;
         *pcmLR++ = *pcm1++;
      }
      fwrite( pcm_buffer, 4, count, out_file );
   }
   else {
      fwrite( pcm[ 0 ], 2, count, out_file );
   }

   return 0;

} /* output_pcm */

static void parse_options( int argc, char **argv, MPEGA_CFG *cfg )
/*---------------------------------------------------------------
   Parse the input options (argv[1..argc-1]) and fill cfg
*/
{
   char *token, *arg;
   int argi = 0;
   int file_count = 0;

   while( ++argi < argc ) {
      int arg_used;
      char c;

      token = argv[ argi ];
      arg_used = 0;
      if( *token++ == '-' ) {
         while( c = *token++ ) {
            if( *token ) arg = token;
            else arg = argv[ argi+1 ];
            switch( c ) {
               case 'd':
                  cfg->freq_div = (INT16)atoi( arg ); // #21
                  arg_used = 1;
                  break;
               case 'q':
                  cfg->quality = (INT16)atoi( arg ); // #21
                  arg_used = 1;
                  break;
               case 'F': // #8
                  cfg->freq_max = (INT32)atoi( arg ); // #21
                  cfg->freq_div = 0; // #21
                  arg_used = 1;
                  break;
               case 'm':
                  cfg->force_mono = TRUE;
                  break;
               case 'h':
                  cfg->header_only = TRUE;
                  break;
               case 'n':
                  cfg->frame_counter = FALSE;
                  break;
               case 'A': // #8
                  cfg->iff_type = IFF_TYPE_AIFF;
                  break;
               case 'W': // #19
                  cfg->iff_type = IFF_TYPE_WAVE;
                  break;
               case 'T': // #10
                  cfg->frame_counter = FALSE;
                  cfg->time_counter = TRUE;
                  break;
               case 'D': // #20
                  cfg->decoding_params = TRUE;
                  break;
#ifdef AMIGA
               case 'p':
                  priority = atoi( arg );
                  if( priority > 30 ) priority = 30;
                  else if( priority < -30 ) priority = -30;
                  arg_used = 1;
                  break;
#endif
#ifdef MPEGAUD_AUDIO
               case 'I':
                  cfg->use_ahi = TRUE;
                  if( *token ) {
                     sscanf( arg, "%x", &cfg->audio_id );
                     arg_used = 1;
                     cfg->audio_query = FALSE; // #11
                  }
                  break;
               case 's':
                  cfg->play_audio = FALSE;
                  break;
               case 'f':
                  cfg->filter = atoi( arg );
                  arg_used = 1;
                  break;
               case 'x':
                  cfg->mixing = (UINT16)atoi( arg );
                  arg_used = 1;
                  break;
               case 't':
                  cfg->buffer_time = (UINT32)atoi( arg );
                  arg_used = 1;
                  break;
               case 'v':
                  cfg->volume = (UINT16)atoi( arg );
                  if( cfg->volume > VOLUME_MAX ) cfg->volume = VOLUME_MAX; // #32
                  arg_used = 1;
                  break;
               case 'V': // #32
                  cfg->volume_comment = TRUE;
                  break;
               case 'w': // #15
                  cfg->audio_wait = TRUE;
                  break;
               case 'a': // #17
                  cfg->audio_stat = TRUE;
                  break;
#endif
               case 'S': // #7
                  cfg->ms_seek = (UINT32)atoi( arg );
                  arg_used = 1;
                  break;
               case 'L': // #37
                  cfg->ms_length = (UINT32)atoi( arg );
                  arg_used = 1;
                  break;
               case 'b': // #15
                  cfg->stream_buffer_size = (INT32)(1024 * atoi( arg ));
                  arg_used = 1;
                  break;
               case 'l': // #22
                  cfg->play_list = TRUE;
                  break;
               case 'o': // #22
                  strncpy( cfg->out_filename, arg, FILENAME_SIZE );
                  arg_used = 1;
                  break;
               case 'c': // #22
                  strncpy( cfg->cfg_filename, arg, FILENAME_SIZE );
                  arg_used = 1;
                  break;
               case 'C': // #22
                  cfg->display_config = TRUE;
                  break;
               case 'e': // #32
                  cfg->cfg_exclude = TRUE;
                  break;
               case 'y': // #33
                  cfg->dyn_boost = TRUE;
                  break;
               case 'k': // #34
                  cfg->scale_percent = (INT32)(atoi( arg ));
                  arg_used = 1;
                  break;
               case 'K': // #35
                  cfg->scale_comment = TRUE;
                  break;
               case 'u': // #22
                  cfg->time_used = TRUE;
                  break;
#ifdef ASYNC_IO  // #24
               case 'r':
                  cfg->async_io = FALSE;
                  break;
#endif
               case 'N': // #26
                  cfg->display_filename = TRUE;
                  break;
               case 'R': // #28
                  cfg->random_play = TRUE;
                  break;
               case 'i': // #29
                  cfg->tag_info = TRUE;
                  break;
               default:
                  printf( "unknown option '%s'\n", argv[ argi ] );
                  break;
            }
            if( arg_used ) {
               if( arg == token ) token = "";
               else argi++;
               arg_used = 0;
            }
         }
      }
      else {
         token--;
         if( file_count == 0 ) strncpy( cfg->in_filename, token, FILENAME_SIZE );
         else if( file_count == 1 ) strncpy( cfg->out_filename, token, FILENAME_SIZE );
         else {
            printf( "Extra argument '%s' found\n", token );
            exit( 0 );
         }
         file_count++;
      }
   }
}

static void usage( char *prog_name )
{
   printf( "Usage: %s [<options>] <MPEG Audio file> [<out file>]\n", prog_name );
   printf( "Options:\n" );
   printf( "-d <freq_div>   frequency division: 1, 2 or 4 (default=1)\n" );
   printf( "-q <quality>    output quality: 0 (worst), 1 or 2 (best) (default=2)\n" );
   printf( "-h              display header only\n" );
   printf( "-n              no frame counter\n" );
   printf( "-m              mono output (left channel)\n" );
#ifdef AMIGA
   printf( "-p <pri>        process priority (-30..30)\n" );
#endif
#ifdef MPEGAUD_AUDIO
   printf( "-I[<id>]        AHI output, <id> = Audio mode ID in hex (ex: -I20004)\n" );
   printf( "-s              silence (no audio output)\n" );
   printf( "-f 0|1          audio filter off/on (default = auto)\n" );
   printf( "-x <freq>       audio mixing frequency (default = none)\n" );
   printf( "-t <ms>         audio buffer time in ms (default = 1 sec)\n" );
   printf( "-v <vol>        audio volume (0..64 or 65..256 for boost)\n" );
   printf( "-V              read audio volume from file comment (VOL=xx)\n" );
   printf( "-w              wait for audio buffer to be full before starting audio\n" ); // #15
   printf( "-a              audio buffer fill level in secs (use with -T option)\n" ); // #17
#endif
   printf( "-S <ms>         seek to time postion in stream (unit is ms)\n" ); // #7
   printf( "-L <ms>         play length (unit is ms)\n" ); // #37
   printf( "-A              write AIFF header to output file\n" ); // #8
   printf( "-W              write WAVE header to output file\n" ); // #8
   printf( "-F <freq_max>   max decoding frequency (auto freq_div)\n" ); // #8
   printf( "-T              display time counter instead of frame counter\n" ); // #10
   printf( "-b <k_size>     set the input file buffer size in KBytes (default = 16KB)\n" ); // #15
   printf( "-D              display decoding parameters\n" ); // #20
   printf( "-l              input file is a play list (text file)\n" ); // #22
   printf( "-o <out file>   output decoded file\n" ); // #22
   printf( "-c <cfg file>   configuration file (default = MPEGA.CFG)\n" ); // #22
   printf( "-C              display current configuration\n" ); // #22
   printf( "-e              exclude reading of default configuration file\n" ); // #32
   printf( "-y              dynamic boost for audio output (maximize audio dynamic)\n" ); // #33
   printf( "-k <scale_per>  scale decoded output by scale factor in %%\n" ); // #34
   printf( "-K              read scale from file comment (SCA=xx)\n" ); // #35
   printf( "-u              display time used\n" ); // #22
#ifdef ASYNC_IO  // #24
   printf( "-r              don't use Async I/O for input file (no lock)\n" );
#endif
   printf( "-N              display filename\n" ); // #26
   printf( "-R              random play\n" ); // #28
   printf( "-i              display TAG infos\n" ); // #29
   printf( "\nEnjoy fast MPEG :-)\n" );
}


static void print_mpeg_out_cfg( MPEGDEC_OUTPUT *mpa_out )
{
   if( mpa_out->freq_div > 0 ) {
      if( mpa_out->freq_div == 1 ) printf( "   full  " );
      else printf( "    /%1d   ", mpa_out->freq_div );
   }
   else {
      printf( "<=%5dHz", mpa_out->freq_max );
   }
   printf( "  %6s  ", dec_qual[ mpa_out->quality ] );
}

static void print_layer_config( char *pre, MPEGDEC_LAYER *l_cfg )
/*---------------------------------------------------------------
   Print the given layer configuration
*/
{
   printf( "%s          ", pre );
   print_mpeg_out_cfg( &l_cfg->mono );
   printf( "          " );
   print_mpeg_out_cfg( &l_cfg->stereo );
   printf( "     %s\n", no_yes[ l_cfg->force_mono ] );
}

static void display_config( void )
/*--------------------------------
   Display the current configuration
*/
{
// #30
   char line[ 80 ];

   printf( "   Input file is a play list = "BOLD"%-7s"NOBOLD
           "     Input buffer size = "BOLD"%d bytes"NOBOLD"\n",
           no_yes[ global_cfg.play_list ], global_cfg.stream_buffer_size );

   printf( "         Display header only = "BOLD"%-7s"NOBOLD
           " Display frame counter = "BOLD"%-3s"NOBOLD"\n",
           no_yes[ global_cfg.header_only ], no_yes[ global_cfg.frame_counter ] );

   printf( "        Display time counter = "BOLD"%-7s"NOBOLD
           "     Display time used = "BOLD"%-3s"NOBOLD"\n",
           no_yes[ global_cfg.time_counter ], no_yes[ global_cfg.time_used ] );

   printf( " Display decoding parameters = "BOLD"%-7s"NOBOLD
           "      Display filename = "BOLD"%-3s"NOBOLD"\n",
           no_yes[ global_cfg.decoding_params ], no_yes[ global_cfg.display_filename ] ); // #26

   printf( "            Output file type = "BOLD"%-7s"NOBOLD
           "             Seek time = "BOLD"%d ms"NOBOLD"\n",
           out_type[ global_cfg.iff_type ], global_cfg.ms_seek );

   printf( "                 Length time = "BOLD ); // #37
   if( global_cfg.ms_length ) printf( "%d ms"NOBOLD"\n", global_cfg.ms_length ); // #37
   else printf( "full"NOBOLD"\n" ); // #37

   printf( "           Display TAG infos = "BOLD"%-7s"NOBOLD
           "           Random play = "BOLD"%s"NOBOLD"\n",
           no_yes[ global_cfg.tag_info ], no_yes[ global_cfg.random_play ] ); // #28

#ifdef MPEGAUD_AUDIO
   printf( "                  Play audio = "BOLD"%-7s"NOBOLD
           "          Audio filter = "BOLD"%s"NOBOLD"\n",
           no_yes[ global_cfg.play_audio ], filter_type[ global_cfg.filter ] );

   if( global_cfg.mixing > 0 ) sprintf( line, "%5dHz", global_cfg.mixing );
   else strcpy( line, "none" );
   printf( "                Audio mixing = "BOLD"%-7s"NOBOLD
           "     Audio buffer time = "BOLD"%d ms"NOBOLD"\n",
           line, global_cfg.buffer_time );

   printf( "                Audio volume = "BOLD"%-7d"NOBOLD
           "               Use AHI = "BOLD"%s"NOBOLD"\n",
           global_cfg.volume, no_yes[ global_cfg.use_ahi ] );

   printf( "Wait audio buffer to be full = "BOLD"%-7s"NOBOLD
           "         Audio mode ID = "BOLD"0x%08X"NOBOLD"\n",
           no_yes[ global_cfg.audio_wait ], global_cfg.audio_id );

   printf( "     Audio buffer fill level = "BOLD"%-7s"NOBOLD
           "  Audio VOL in comment = "BOLD"%-7s"NOBOLD"\n", // #32
            no_yes[ global_cfg.audio_stat ], no_yes[ global_cfg.volume_comment ] );
   // #35 Begin
   printf( "  Audio dynamic boost volume = "BOLD"%-7s"NOBOLD"\n",
            no_yes[ global_cfg.dyn_boost ] );
   // #35 End
#endif
   // #35 Begin
   printf( "                Scale factor = "BOLD"%-5d %%"NOBOLD
           "      Scale in comment = "BOLD"%-7s"NOBOLD"\n\n",
            global_cfg.scale_percent, no_yes[ global_cfg.scale_comment ] );
   // #35 End

   printf( LINE""BOLD"LAYER(S)"NOBOLD"   MONO: "BOLD"FREQUENCY  QUALITY"NOBOLD
           "   STEREO: "BOLD"FREQUENCY  QUALITY  FORCE MONO"NOBOLD""NOLINE"\n" );
   print_layer_config( " I & II", &mpa_ctrl.layer_1_2 );
   print_layer_config( "  III  ", &mpa_ctrl.layer_3 );
   printf( "\n" );
}

static int play_file( char *filename )
{
   static INT16 *pcm[ MPEGDEC_MAX_CHANNELS ] = { NULL, NULL };
   INT32 pcm_count;
   BOOL again = TRUE; // #17
   INT32 frame = 0;
#ifdef TIME_STAT
   clock_t clk;
#endif
#ifdef MPEGAUD_AUDIO // #32
   UINT16 volume = global_cfg.volume;
#endif
   INT16 sample_min = 0, sample_max = 0; // #33
   INT16 max_boost_vol  = VOLUME_NOM; // #33
   INT16 auto_boost_vol = volume; // #33
   INT32 scale = global_cfg.scale_percent; // #35
   char comment[ 80 ]; // #35
   INT32 pcm_length = 0; // #37

#ifdef MPEGA_LIBRARY // #32 Begin
   if( !MPEGABase ) {
      MPEGABase = OpenLibrary( "mpega.library", MPEGAVersion ); // #34
      if( !MPEGABase ) {
         MPEGAVersion = 0; // #34
         MPEGABase = OpenLibrary( "mpega.library", MPEGAVersion ); // #34
         if( !MPEGABase ) {
            printf( "Can't find mpega.library !\n" );
            exit( 0 );
         }
      }
   }
#endif // #32 End

   // #35 Begin
   {
      BPTR lock;
      __aligned struct FileInfoBlock fib;

      *comment = '\0';
      lock = Lock( filename, ACCESS_READ );
      if( lock ) {
         if( Examine( lock, &fib ) ) {
            strncpy( comment, fib.fib_Comment, 80 );
         }
         UnLock( lock );
      }
   }
   // #35 End

   mps = MPEGDEC_open( filename, &mpa_ctrl );
   if( !mps ) {
      printf( "Unable to open MPEG Audio stream '%s'\n", filename );
      return -1;
   }

   // #35 Begin
   if( global_cfg.scale_comment ) {
      char *s = strstr( comment, "SCA=" );
      if( s ) {
         scale = atoi( s+4 );
//printf( "Scale in comment = %d\n", scale );
      }
   }
   // #35 End


   // #34 Begin
   if( scale > 0 ) {
      if( MPEGAVersion < 2 ) {
         printf( "* Can't scale output, need mpega.library V2.0+ *\n" );
      }
      else if( MPEGA_scale( mps, scale ) ) {
         printf( "* Can't scale output to %d %% *\n", scale );
      }
   }
   // #34 End

   if( global_cfg.display_filename ) { // #26
      printf( "File playing: "BOLD"%s"NOBOLD"\n", filename ); // #30
   }

   if( global_cfg.tag_info ) { // #29
      MPEGTAG tag;
      if( MPEGTAG_get( filename, &tag ) == 0 ) {
// #30 Begin
         if( *tag.title )   printf( "       Title: "BOLD"%s"NOBOLD"\n", tag.title );
         if( *tag.artist )  printf( "      Artist: "BOLD"%s"NOBOLD"\n", tag.artist );
         if( *tag.album )   printf( "       Album: "BOLD"%s"NOBOLD"\n", tag.album );
         if( *tag.year )    printf( "        Year: "BOLD"%s"NOBOLD"\n", tag.year );
         if( *tag.genre )   printf( "       Genre: "BOLD"%s"NOBOLD"\n", tag.genre );
         if( *tag.comment ) printf( "     Comment: "BOLD"%s"NOBOLD"\n", tag.comment );
// #30 Endif
      }
   }

   // #20 Begin
   if( global_cfg.decoding_params ) {
// #30 Begin
      printf( "      Output: "BOLD"%s file, %s quality, %s %dHz"NOBOLD"\n",
              (*global_cfg.out_filename != '\0') ? out_type[ global_cfg.iff_type ] : "no",
              dec_qual[ mps->dec_quality ],
              dec_mode[ mps->dec_channels ],
              mps->dec_frequency );
// #30 End
   }
   // #20 End

   {
      INT32 mins, secs, mils;

      mils = mps->ms_duration;
      mins = mils / 60000;
      mils -= mins * 60000;
      secs = mils / 1000;
// #30 Begin
      printf( "   File Type: "BOLD"MPEG%d-%s %s %dkbps %dHz"NOBOLD"\n",
              mps->norm, (mps->layer == 1)?"I":(mps->layer == 2)?"II":"III",
              modes[ mps->mode ], mps->bitrate, mps->frequency );
      printf( " File length: "BOLD"%02d:%02d"NOBOLD"\n", mins, secs );
// #30 End

   }
   if( global_cfg.header_only ) {
      MPEGDEC_close( mps );
      mps = NULL;
      return 0;
   }

   samples = 0;

   if( *global_cfg.out_filename != '\0' ) {
      out = fopen( global_cfg.out_filename, "wb" );
      if( out ) { // #20
         setvbuf( out, NULL, _IOFBF, 32768 );
      }
      if( (out) && (global_cfg.iff_type != IFF_TYPE_NONE) ) {

         if( write_iff_header( out, global_cfg.iff_type,
                               mps->dec_channels,
                               mps->dec_frequency, samples ) ) {
            printf( "Could not write out headers to \"%s\"\n", global_cfg.out_filename );
         }
      }
   }



#ifdef MPEGAUD_AUDIO
   if( global_cfg.play_audio ) {
      int err;
      AUM_PARAMS aup;

      // #32 Begin
      if( global_cfg.volume_comment ) {
         char *s = strstr( comment, "VOL=" );
         if( s ) {
            volume = atoi( s+4 );
            if( volume > VOLUME_MAX ) volume = VOLUME_MAX;
//printf( "Volume in comment = %d\n", volume );
         }
      }
      // #32 End

      if( AUM_open() ) {
         printf( "Audio init error !\n" );
         exit( 0 );
      }

      aup.mode.device = (global_cfg.use_ahi)?AUM_DEVICE_AHI:AUM_DEVICE_PAULA;
      aup.mode.id = global_cfg.audio_id;
      if( global_cfg.audio_query ) {
         if( AUM_query( &aup.mode ) ) {
            exit( 0 );
         }
         global_cfg.audio_query = FALSE;
         global_cfg.audio_id = aup.mode.id;
      }
      aup.channels = mps->dec_channels;
      aup.filter = global_cfg.filter;
      aup.effects = 0;
      aup.frequency = mps->dec_frequency;
      aup.mixing = global_cfg.mixing;
      aup.volume[ 0 ] = aup.volume[ 1 ] = volume; // #32 global_cfg.volume -> volume
      aup.audio_size = (global_cfg.buffer_time * mps->dec_frequency) / 1000;
      if( aup.audio_size < 8192 ) aup.audio_size = 8192;
      aup.sample_started = NULL;

      if( err = AUM_config( &aup ) ) {
         printf( "Can't open Audio ! [err=%d]\n", err );
         exit( 0 );
      }
      (void)AUM_control( AUM_CTRL_PLAY, TRUE );
   }
#endif


   {
      INT16 i;

      for( i=0; i<MPEGDEC_MAX_CHANNELS; i++ ) {
         if( !pcm[ i ] ) {
            pcm[ i ] = malloc( MPEGDEC_PCM_SIZE * sizeof( INT16 ) );
         }
         if( !pcm[ i ] ) {
            printf( "Memory full !\n" );
            exit( 0 );
         }
      }
   }

   if( global_cfg.ms_seek ) { // #7
      if( MPEGDEC_seek( mps, global_cfg.ms_seek ) ) {
         printf( "Seek failed\n" );
         exit( 0 );
      }
   }

   if( global_cfg.ms_length ) { // #37
      pcm_length = (INT32)((double)mps->dec_frequency * (double)global_cfg.ms_length * 0.001);
   }

#ifdef TIME_STAT
   clk = clock();
#endif

#ifdef MPEGAUD_AUDIO // #15
   if( global_cfg.audio_wait && global_cfg.play_audio ) AUM_control( AUM_CTRL_PLAY, FALSE );
#endif

   while( again ) {

      if( pcm_length < 0 ) { // #37
         pcm_count = MPEGDEC_ERR_EOF;
      }
      else {
         pcm_count = MPEGDEC_decode_frame( mps, pcm );
      }
      if( (pcm_count > 0) && (pcm_length > 0) ) { // #37
         pcm_length -= pcm_count;
      }

      if( pcm_count > 0 ) {
#ifdef MPEGAUD_AUDIO
         if( global_cfg.play_audio ) {
            AUM_SAMPLE aus;

            aus.sample[ 0 ] = pcm[ 0 ];
            aus.sample[ 1 ] = pcm[ 1 ];
            aus.size = pcm_count;

            if( global_cfg.audio_wait ) { // #15
               INT32 free_samples;
               INT32 state;

               AUM_control( AUM_CTRL_STATE, (ULONG)&state );
               if( state != AUM_STA_PLAY ) {
                  if( state == AUM_STA_STOP ) {
                     AUM_control( AUM_CTRL_PLAY, FALSE );
                  }
                  AUM_control( AUM_CTRL_FREE, (ULONG)&free_samples );
                  if( free_samples < aus.size ) {
                     AUM_control( AUM_CTRL_PLAY, TRUE );
                  }
               }
            }
            if( AUM_write( &aus ) ) {
               exit( 0 );
            }
         }
#endif
         if( out ) output_pcm( mps, pcm, pcm_count, out );
         // #33 Begin
         if( global_cfg.dyn_boost ) {
            register INT16 count = pcm_count;
            register INT16 smin = sample_min;
            register INT16 smax = sample_max;
            if( mps->dec_channels == 1 ) { // Mono
               register INT16 *p = pcm[ 0 ];
               register INT16 s;
#if 1
               while( count-=2 ) { // check min for odd samples, max for even
                  s = *p++;
                  if( s > smax ) smax = s;
                  s = *p++;
                  if( s < smin ) smin = s;
               }
#else
               while( count-- ) {
                  s = *p++;
                  if( s > smax ) smax = s;
                  else if( s < smin ) smin = s;
               }
#endif
            }
            else { // Stereo
               register INT16 *pl = pcm[ 0 ];
               register INT16 *pr = pcm[ 1 ];
               register INT16 s;
               while( count-=2 ) { // check min for odd samples, max for even
                  s = *pl++;
                  if( s > smax ) smax = s;
                  s = *pr++;
                  if( s > smax ) smax = s;
                  s = *pl++;
                  if( s < smin ) smin = s;
                  s = *pr++;
                  if( s < smin ) smin = s;
               }
            }
            sample_min = smin;
            sample_max = smax;
            {
               INT32 smax = (INT32)sample_max;
               INT32 smin = -((INT32)sample_min);
               if( smin > smax ) smax = smin;

               if( smax > 0 ) {
                  if( smax < ((32768L*VOLUME_NOM)/VOLUME_MAX) )
                     smax = ((32768L*VOLUME_NOM)/VOLUME_MAX);
                  max_boost_vol = (INT16)( VOLUME_NOM * 32768L / smax );
               }
               if( auto_boost_vol < max_boost_vol ) auto_boost_vol++;
               else if( auto_boost_vol > max_boost_vol ) auto_boost_vol--;
            }
            // Limit the volume sliding update freq (lot of cpu usage)
            if( global_cfg.play_audio && ((frame & 15) == 15) ) {
//printf( "Vol: a=%3d v=%3d\r", max_boost_vol, auto_boost_vol );
//fflush( stdout );
               if( auto_boost_vol != volume ) {
                  volume = auto_boost_vol;
                  AUM_control( AUM_CTRL_LVOL, volume );
                  AUM_control( AUM_CTRL_RVOL, volume );
               }
            }
         }
         // #33 End
         samples += pcm_count;
      }

      if( pcm_count >= 0 ) frame++;

//      if( global_cfg.verbose ) {
         if( global_cfg.time_counter ) { // #10
            UINT32 ms_time;
#ifdef MPEGAUD_AUDIO // #17
            INT32 remaining_samples;
            static INT32 ms_audio = 0, old_ms_audio = 0;
#endif
            INT32 mins, secs;
            static INT32 old_mins = -1, old_secs = -1;

            MPEGDEC_time( mps, &ms_time );
#ifdef MPEGAUD_AUDIO // #17
            if( global_cfg.play_audio ) {
               AUM_control( AUM_CTRL_REMAIN, (ULONG)&remaining_samples );
               if( remaining_samples > (0x7FFFFFFF/1000) ) {
                  ms_audio = (remaining_samples * 100) / (mps->dec_frequency / 10);
               }
               else {
                  ms_audio = (remaining_samples * 1000) / mps->dec_frequency;
               }
               if( ms_time > ms_audio ) ms_time -= ms_audio;
            }
#endif
            mins = ms_time / 60000;
            ms_time -= mins * 60000;
            secs = ms_time / 1000;
            ms_time -= secs * 1000;
#ifdef MPEGAUD_AUDIO // #17
            if( global_cfg.play_audio && global_cfg.audio_stat ) {
               INT32 ms_audio_diff = ms_audio - old_ms_audio;
               if( ms_audio_diff < 0 ) ms_audio_diff = -ms_audio_diff;

               if( (old_mins != mins) || (old_secs != secs) || (ms_audio_diff >= 1000 ) ) {
                  printf( "\r%02d:%02d / %02d", mins, secs, (ms_audio+500) / 1000 );
                  fflush( stdout );
                  old_mins = mins;
                  old_secs = secs;
                  old_ms_audio = ms_audio;
               }
            }
            else
#endif
            if( (old_mins != mins) || (old_secs != secs) ) {
               printf( "\rElapsed time: "BOLD"%02d:%02d"NOBOLD, mins, secs ); // #30
               fflush( stdout );
               old_mins = mins;
               old_secs = secs;
            }
         }
//         else if( (frame & 15) == 0 ) {
         else if( !(frame & 15) && global_cfg.frame_counter ) {
            printf( "\r{%04d}", frame ); fflush( stdout );
         }
//      }

#ifdef MPEGAUD_AUDIO // #17
      if( pcm_count == MPEGDEC_ERR_EOF ) {
         if( global_cfg.play_audio ) {
            INT32 state;

            AUM_control( AUM_CTRL_STATE, (ULONG)&state );
            if( state != AUM_STA_PLAY ) {
               AUM_control( AUM_CTRL_PLAY, TRUE ); // Leave pause mode (with -w option)
               AUM_control( AUM_CTRL_STATE, (ULONG)&state );
            }
            if( state == AUM_STA_PLAY ) Delay( 1 );
            else again = FALSE;
         }
         else again = FALSE;
      }
#else
      if( pcm_count == MPEGDEC_ERR_EOF ) again = FALSE;
#endif

#ifdef AMIGA
      if( SetSignal( 0, SIGBREAKF_CTRL_C ) & SIGBREAKF_CTRL_C ) {
         break_cleanup();
         exit( 0 );
      }
#endif
   }

   if( global_cfg.time_counter ) { // #10
      printf( "\n" );
   }
   else if( global_cfg.frame_counter ) {
      printf( "\r{%04d}\n", frame );
   }

#ifdef MPEGAUD_AUDIO
   if( global_cfg.play_audio ) {
      AUM_close();
   }
#endif

#ifdef TIME_STAT
   clk = clock() - clk;
#endif

   close_output();

   if( mps ) {
      MPEGDEC_close( mps );
      mps = NULL;
   }

#ifdef TIME_STAT
#ifndef CLK_TCK
#define CLK_TCK CLOCKS_PER_SEC
#endif
   if( global_cfg.time_used ) printf( "%5.2f sec(s)\n", (double)clk / (double)CLK_TCK );
#endif

   // #33 Begin
#ifdef MPEGAUD_AUDIO
   if( (!global_cfg.play_audio) && global_cfg.dyn_boost ) {
#else
   if( global_cfg.dyn_boost ) {
#endif
      INT32 smax = (INT32)sample_max;
      INT32 smin = -((INT32)sample_min);
      if( smin > smax ) smax = smin;

      if( smax > 0 ) {
         int vol;
         double boost = 3276800.0 / (double)smax;

         vol = (int)(0.64 * boost);
         if( vol > VOLUME_MAX ) vol = VOLUME_MAX;
         printf( "Samples in [%d..%d] -> boost up to %d %% (VOL=%d)\n",
                  sample_min, sample_max, (int)boost, vol );
      }
      else {
         printf( "All Samples are 0 !, no boost info available\n" );
      }
   }
   // #33 End

   return 0;
}

#define LINE_SIZE  256
#define MAX_TOKENS 32

static void init_config( MPEGA_CFG *cfg )
{
   memset( cfg, 0, sizeof( global_cfg ) );
   cfg->freq_div = -1;
   cfg->quality = -1;
   cfg->freq_max = -1;
   cfg->stream_buffer_size = 16384;

   cfg->frame_counter = TRUE;
#ifdef MPEGAUD_AUDIO
   cfg->play_audio = TRUE;
   cfg->filter = 2;
   cfg->buffer_time = 1000;
   cfg->volume = VOLUME_NOM; // #32
   cfg->audio_query = TRUE;
#endif
#ifdef ASYNC_IO // #24
   cfg->async_io = TRUE;
#endif
}

static int check_line( char *line )
/*---------------------------------
   Check if line is a text line
   Replace '\n' by '\0'
   Return -1 if not text
   Return length of line if text
*/
{
   int len = 0;
   unsigned char *ptr = line;
   BOOL end = FALSE;
   BOOL text = TRUE;

   while( *ptr ) {
      if( (*ptr == '\n') || (*ptr == '\r') ) {
         *ptr = '\0';
         end = TRUE;
      }
      else if( *ptr < 32 ) text = FALSE;
      else if( (!end) && ((!isspace( *ptr )) || (len > 0)) ) len++;
      ptr++;
   }
   if( text ) return len;
   return -1;
}

static int read_config_file( char *filename )
/*--------------------------------------------
   Read a configuration file and set
   global options according to.
   Return 0 if ok
          -1 if config file not found
*/
{
   FILE *file;
   char line[ LINE_SIZE ];
   char *argv[ MAX_TOKENS ];
   int argc;
   int option_line =0;
   MPEGA_CFG local_cfg;

   if( !*filename ) return 0;
   file = fopen( filename, "r" );
   if( !file ) return -1;

   while( fgets( line, LINE_SIZE, file ) ) {
      int len;

      len = check_line( line );
      if( len < 0 ) {
         printf( "'%s' is not a text file !\n", filename );
         exit( 0 );
      }
      if( (len == 0) || (*line == '#') ) {
         // Comment line
      }
      else {
         char *token;
         int i;
         argc = 1;

//printf( "%d '%s'\n", option_line, line );
         token = strtok( line, " " );
         while( (token) && (argc < (MAX_TOKENS-1)) ) {
            argv[ argc ] = malloc( strlen( token ) + 1 );
            if( argv[ argc ] ) {
               strcpy( argv[ argc ], token );
            }
            argc++;
            token = strtok( NULL, " " );
         }

         if( option_line < 4 ) {
            MPEGDEC_OUTPUT *mpa_out;

            init_config( &local_cfg );
            parse_options( argc, argv, &local_cfg );
            switch( option_line ) {
               case 0: mpa_out = &mpa_ctrl.layer_1_2.mono; break;
               case 1: mpa_out = &mpa_ctrl.layer_3.mono; break;
               case 2: mpa_out = &mpa_ctrl.layer_1_2.stereo; break;
               default: mpa_out = &mpa_ctrl.layer_3.stereo; break;
            }
            if( local_cfg.freq_div >= 0 ) mpa_out->freq_div = local_cfg.freq_div;
            if( local_cfg.quality >= 0 ) mpa_out->quality = local_cfg.quality;
            if( local_cfg.freq_max > 0 ) mpa_out->freq_max = local_cfg.freq_max;
            if( local_cfg.force_mono ) {
               if( option_line == 2 ) mpa_ctrl.layer_1_2.force_mono = local_cfg.force_mono;
               if( option_line == 3 ) mpa_ctrl.layer_3.force_mono = local_cfg.force_mono;
            }
         }
         else {
            parse_options( argc, argv, &global_cfg );
         }

         for( i=1; i<argc; i++ ) {
            if( argv[ argc ] ) {
               free( argv[ argc ] );
               argv[ argc ] = NULL;
            }
         }
         option_line++;
      }
   }

   fclose( file );

   if( option_line < 4 ) {
      printf( "* WARNING missing %d options line(s) in '%s' *\n",
               4 - option_line, filename );
   }
   return 0;
}

// #28 Begin
static struct {
   char **array;
   int count;
   int size;
   int current;
} playlist = {
   NULL,
   0, 0, 0
};

static int playlist_add( char *filename )
{
   int len = strlen( filename );

   if( len > 0 ) {
      if( playlist.size <= playlist.count ) {
         char **new_array;
         new_array = (char **)realloc( playlist.array, (playlist.size + 50) * sizeof(char *) );
         if( !new_array ) return -1;
         playlist.array = new_array;
         playlist.size += 50;
      }
      len++;
      playlist.array[ playlist.count ] = (char *)malloc( len );
      if( !playlist.array[ playlist.count ] ) return -1;
      strncpy( playlist.array[ playlist.count ], filename, len );
      playlist.count++;
   }
   return 0;
}

static void playlist_randomize( void )
{
   int i;
   int i1, i2;
   char *temp;

   if( playlist.count <= 0 ) return;

   srand( (unsigned int)time( NULL ) );
   for( i=0; i<playlist.count; i++ ) {
      i1 = rand() % playlist.count;
      i2 = rand() % playlist.count;
      temp = playlist.array[ i1 ];
      playlist.array[ i1 ] = playlist.array[ i2 ];
      playlist.array[ i2 ] = temp;
   }
}

static char *playlist_next( void )
{
   char *name = NULL;

   if( playlist.current < playlist.count ) {
      name = playlist.array[ playlist.current ];
      playlist.current++;
   }
   return name;
}

static char *playlist_first( void )
{
   playlist.current = 0;
   return playlist_next();
}

// #28 End

int main( int argc, char **argv )
{
#ifdef ASYNC_IO
//   MPEGDEC_ACCESS bs_access = { bs_open, bs_close, bs_read, bs_seek };
   static struct Hook bs_access = { { NULL, NULL }, bs_access_func, NULL, NULL }; // #31
#endif
   MPEGDEC_OUTPUT mpa_out = {
      1, 2, 48000
   };

#ifdef AMIGA // #32 moved
   onbreak( break_cleanup );
#endif
   atexit( exit_cleanup );

   init_config( &global_cfg );

   // #21 Begin
   mpa_ctrl.check_mpeg = 0;

   mpa_ctrl.layer_1_2.force_mono = FALSE;
   mpa_ctrl.layer_1_2.mono = mpa_out;
   mpa_ctrl.layer_1_2.stereo = mpa_out;
   mpa_ctrl.layer_3 = mpa_ctrl.layer_1_2;
   // #21 End

   printf( "%s\n", &Version[ 5 ] );
//   printf( "Release date: "BOLD""__DATE__""NOBOLD"\n" );
   printf( "Email: "BOLD"Stephane.Tavenard@wanadoo.fr"NOBOLD"\n" ); // #31
   printf( "Giftware MPEG Audio decoder for Layers I,II and III\n\n" );
   if( argc < 2 ) {
      usage( argv[ 0 ] );
      exit( 0 );
   }


   parse_options( argc, argv, &global_cfg );
   // #32 Begin
   if( !global_cfg.cfg_exclude ) {
#ifdef AMIGA
      static const char *cfg_names[] = { // #25
         "ENV:MPEGA.CFG", "S:MPEGA.CFG", "PROGDIR:MPEGA.CFG", "MPEGA.CFG", NULL
      };
      int i = 0;

      while( cfg_names[ i ] ) {
         if( read_config_file( (char *)cfg_names[ i ] ) == 0 ) break;
         i++;
      }
#else
      read_config_file( "MPEGA.CFG" );
#endif
   }
   // #32 End
   if( *global_cfg.cfg_filename ) {
      if( read_config_file( global_cfg.cfg_filename ) == -1 ) {
         printf( "Config file '%s' not found\n", global_cfg.cfg_filename );
         exit( 0 );
      }
   }

#ifdef ASYNC_IO // #24
   if( global_cfg.async_io ) mpa_ctrl.bs_access = &bs_access;
   else mpa_ctrl.bs_access = NULL;
#else // #21
   mpa_ctrl.bs_access = NULL;
#endif

   if( global_cfg.freq_div >= 0 ) {
      mpa_ctrl.layer_1_2.mono.freq_div =
      mpa_ctrl.layer_1_2.stereo.freq_div =
      mpa_ctrl.layer_3.mono.freq_div =
      mpa_ctrl.layer_3.stereo.freq_div = global_cfg.freq_div;
   }
   if( global_cfg.quality >= 0 ) {
      mpa_ctrl.layer_1_2.mono.quality =
      mpa_ctrl.layer_1_2.stereo.quality =
      mpa_ctrl.layer_3.mono.quality =
      mpa_ctrl.layer_3.stereo.quality = global_cfg.quality;
   }
   if( global_cfg.freq_max >= 0 ) {
      mpa_ctrl.layer_1_2.mono.freq_max =
      mpa_ctrl.layer_1_2.stereo.freq_max =
      mpa_ctrl.layer_3.mono.freq_max =
      mpa_ctrl.layer_3.stereo.freq_max = global_cfg.freq_max;
   }
   if( global_cfg.force_mono ) {
      mpa_ctrl.layer_1_2.force_mono =
      mpa_ctrl.layer_3.force_mono = global_cfg.force_mono;
   }
   mpa_ctrl.stream_buffer_size = global_cfg.stream_buffer_size;

   if( global_cfg.display_config ) {
      display_config();
   }

   if( !*global_cfg.in_filename ) exit( 0 );

#ifdef AMIGA // #36
   if( priority != old_priority ) {
      old_priority = SetTaskPri( FindTask(NULL), priority );
   }
#endif

   if( global_cfg.play_list ) {
      char filename[ FILENAME_SIZE ];
      char *name; // #28
      FILE *play_list = fopen( global_cfg.in_filename, "r" );

      if( !play_list ) {
         printf( "Can't open play list '%s'\n", global_cfg.in_filename );
         exit( 0 );
      }
      while( fgets( filename, FILENAME_SIZE, play_list ) ) {
         int len;

         len = check_line( filename );

         if( len < 0 ) {
            printf( "'%s' is not a valid play list (not a text file)\n", global_cfg.in_filename );
            exit( 0 );
         }
         playlist_add( filename ); // #28
      }
      fclose( play_list );

      // #28 Begin
      if( global_cfg.random_play ) playlist_randomize();
      name = playlist_first();
      while( name ) {
         (void)play_file( name );
         name = playlist_next();
      }
      // #28 End

   }
   else {
//            printf( "%s\n", filename ); // #26: removed
      play_file( global_cfg.in_filename );
   }

   breaked = FALSE;

   return 0;
}

