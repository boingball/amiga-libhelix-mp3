/*
* This file is part of MPEGALibrary.
* Copyright (C) 1999 Stephane Tavenard
* 
* MPEGALibrary is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* MPEGALibrary is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with MPEGALibrary.  If not, see <http://www.gnu.org/licenses/>.
*
*/
/*------------------------------------------------------------------------------

    File    :   CMP_PCM.C

    Author  :   Stéphane TAVENARD

    $VER:   CMP_PCM.C  0.0  (13/07/1997)

    (C) Copyright 1997-1997 Stéphane TAVENARD
        All Rights Reserved

    #Rev|   Date   |                      Comment
    ----|----------|--------------------------------------------------------
    0   |13/07/1997| Initial revision                                     ST

    ------------------------------------------------------------------------

    Compare 2 pcm files

------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main( int argc, char **argv )
{
   FILE *f1, *f2;
   short p1, p2;
   int diffs = 0;
   int diff;
   int index = 0;
   int max_diff = 0;
   int max_index = 0;

   if( argc != 3 ) {
      printf( "Usage: %s <pcm file 1> <pcm file 2>\n", argv[ 0 ] );
      exit( 0 );
   }

   f1 = fopen( argv[ 1 ], "rb" );
   if( !f1 ) {
      printf( "Unable to open file '%s'\n", argv[ 1 ] );
      exit( 0 );
   }
   f2 = fopen( argv[ 2 ], "rb" );
   if( !f2 ) {
      fclose( f1 );
      printf( "Unable to open file '%s'\n", argv[ 2 ] );
      exit( 0 );
   }

   while( 1 ) {
      if( fread( &p1, 2, 1, f1 ) != 1 ) break;
      if( fread( &p2, 2, 1, f2 ) != 1 ) break;
      diff = p1 - p2;
      if( diff ) {
         diffs++;
         if( diff < 0 ) diff = -diff;
         if( diff > max_diff ) {
            max_diff = diff;
            max_index = index;
         }
      }
      index++;
   }

   fclose( f2 );
   fclose( f1 );

   printf( "diffs = %d\n", diffs );
   if( diffs ) {
      printf( "max diff = %d (0x%04X) at %d (0x%08X)\n",
               max_diff, (unsigned short)max_diff, max_index, max_index );
   }
}
