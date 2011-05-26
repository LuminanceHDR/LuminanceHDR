/**
 * @brief PFS library - DOM I/O
 *
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 * Copyright (C) 2011 Davide Anastasia
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 *
 * @author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 */

#include <stdio.h>
#include <list>

#include "domio.h"
#include "frame.h"

using namespace std;

namespace pfs
{
    const char *PFSFILEID="PFS1\x0a";

    DOMIO::DOMIO() { };
    DOMIO::~DOMIO() { };

    Frame* DOMIO::readFrame( FILE *inputStream )
    {
        assert( inputStream != NULL );

#ifdef HAVE_SETMODE
        // Needed under MS windows (text translation IO for stdin/out)
        int old_mode = setmode( fileno( inputStream ), _O_BINARY );
#endif

        size_t read;

        char buf[5];
        read = fread( buf, 1, 5, inputStream );
        if( read == 0 ) return NULL; // EOF

        if( memcmp( buf, PFSFILEID, 5 ) ) throw Exception( "Incorrect PFS file header" );

        int width, height, channelCount;
        read = fscanf( inputStream, "%d %d" PFSEOL, &width, &height );
        if( read != 2 || width <= 0 || width > MAX_RES || height <= 0 || height > MAX_RES )
            throw Exception( "Corrupted PFS file: missing or wrong 'width', 'height' tags" );
        read = fscanf( inputStream, "%d" PFSEOL, &channelCount );
        if( read != 1 || channelCount < 0 || channelCount > MAX_CHANNEL_COUNT )
            throw Exception( "Corrupted PFS file: missing or wrong 'channelCount' tag" );

        FrameImpl *frame = (FrameImpl*)createFrame( width, height );

        readTags( frame->tags, inputStream );

        //Read channel IDs and tags
        //       FrameImpl::ChannelID *channelID = new FrameImpl::ChannelID[channelCount];
        std::list<ChannelImpl*> orderedChannel;
        for( int i = 0; i < channelCount; i++ )
        {
            char channelName[MAX_CHANNEL_NAME+1], *rs;
            rs = fgets( channelName, MAX_CHANNEL_NAME, inputStream );
            if( rs == NULL )
                throw Exception( "Corrupted PFS file: missing channel name" );
            size_t len = strlen( channelName );
            //      fprintf( stderr, "s = '%s' len = %d\n", channelName, len );
            if( len < 1 || channelName[len-1] != PFSEOLCH )
                throw Exception( "Corrupted PFS file: bad channel name" );
            channelName[len-1] = 0;
            ChannelImpl *ch = (ChannelImpl*)frame->createChannel( channelName );
            readTags( ch->tags, inputStream );
            orderedChannel.push_back( ch );
        }

        read = fread( buf, 1, 4, inputStream );
        if( read == 0 || memcmp( buf, "ENDH", 4 ) )
            throw Exception( "Corrupted PFS file: missing end of header (ENDH) token" );


        //Read channels
        std::list<ChannelImpl*>::iterator it;
        for ( it = orderedChannel.begin(); it != orderedChannel.end(); it++ )
        {
            ChannelImpl *ch = *it;
            unsigned int size = frame->getWidth()*frame->getHeight();
            read = fread( ch->getRawData(), sizeof( float ), size, inputStream );
            if ( read != size )
                throw Exception( "Corrupted PFS file: missing channel data" );
        }
#ifdef HAVE_SETMODE
        setmode( fileno( inputStream ), old_mode );
#endif
        return frame;
    }


    Frame* DOMIO::createFrame( int width, int height )
    {
        /*    if( lastFrame != NULL && lastFrame->width() == width && lastFrame->height() == height ) {
    // Reuse last frame
    return lastFrame;
    } else
    delete lastFrame;*/

        Frame *frame = new FrameImpl( width, height );
        if ( frame == NULL ) throw Exception( "Out of memory" );
        return frame;
    }


    void DOMIO::writeFrame( Frame *frame, FILE *outputStream )
    {
        assert( outputStream != NULL );
        assert( frame != NULL );
#ifdef HAVE_SETMODE
        // Needed under MS windows (text translation IO for stdin/out)
        int old_mode = setmode( fileno( outputStream ), _O_BINARY );
#endif

        FrameImpl *frameImpl = (FrameImpl*)frame;

        fwrite( PFSFILEID, 1, 5, outputStream ); // Write header ID

        fprintf( outputStream, "%d %d" PFSEOL, frame->getWidth(), frame->getHeight() );
        //fprintf( outputStream, "%d" PFSEOL, frameImpl->channel.size() );
        fprintf( outputStream, "%zd" PFSEOL, frameImpl->channel.size() );

        writeTags( frameImpl->tags, outputStream );

        //Write channel IDs and tags
        for( ChannelMap::iterator it = frameImpl->channel.begin(); it != frameImpl->channel.end(); it++ )
        {
            fprintf( outputStream, "%s" PFSEOL, it->second->getName().c_str() );
            writeTags( it->second->tags, outputStream );
        }

        fprintf( outputStream, "ENDH");

        //Write channels
        {
            for( ChannelMap::iterator it = frameImpl->channel.begin(); it != frameImpl->channel.end(); it++ )
            {
                int size = frame->getWidth()*frame->getHeight();
                fwrite( it->second->getRawData(), sizeof( float ), size, outputStream );
            }
        }

        //Very important for pfsoutavi !!!
        fflush(outputStream);
#ifdef HAVE_SETMODE
        setmode( fileno( outputStream ), old_mode );
#endif
    }

    void DOMIO::freeFrame( Frame *frame )
    {
        delete frame;
        frame = NULL;
    }

}

