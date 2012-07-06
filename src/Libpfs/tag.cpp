/**
 * @brief PFS library - PFS Tag Handling
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

#include "tag.h"
#include "frame.h"

#include <stdio.h>
#include <list>
#include <string>

using namespace std;

namespace pfs
{

    TagList::const_iterator TagContainer::tagsBegin() const
    {
        return m_tags.begin();
    }

    TagList::const_iterator TagContainer::tagsEnd() const
    {
        return m_tags.end();
    }

    int TagContainer::getSize() const
    {
        return (int)m_tags.size();
    }

    void TagContainer::appendTagEOL( const char *tagValue )
    {
        assert( tagValue[strlen( tagValue ) -1] == PFSEOLCH );
        m_tags.push_back( string( tagValue, strlen( tagValue ) -1 ) );
    }

    void TagContainer::appendTag( const string &tagValue )
    {
        m_tags.push_back( tagValue );
    }

    TagList::iterator TagContainer::findTag( const char *tagName )
    {
        size_t tagNameLen = strlen( tagName );
        TagList::iterator it;
        for( it = m_tags.begin(); it != m_tags.end(); it++ )
        {
            if ( !memcmp( tagName, it->c_str(), tagNameLen ) ) break; // Found
        }
        return it;
    }

    void TagContainer::setTag( const char *tagName, const char *tagValue )
    {
        string tagVal( tagName );
        tagVal += "=";
        tagVal += tagValue;

        TagList::iterator element = findTag( tagName );
        if ( element == m_tags.end() )
        {
            // Does not exist
            m_tags.push_back( tagVal );
        }
        else
        {
            // Already exist
            *element = tagVal;
        }
    }

    const char *TagContainer::getTag( const char *tagName )
    {
        TagList::iterator element = findTag( tagName );
        if ( element == m_tags.end() ) return NULL;

        std::string::size_type equalSign = element->find( '=' );
        assert( equalSign != string::npos );

        return element->c_str() + equalSign + 1;
    }

    const char* TagContainer::getString( const char *tagName )
    {
        return getTag( tagName );
    }

    void TagContainer::setString( const char *tagName, const char *tagValue )
    {
        setTag( tagName, tagValue );
    }

    void TagContainer::removeTag( const char *tagName )
    {
        TagList::iterator element = findTag( tagName );
        if( element != m_tags.end() ) m_tags.erase( element );
    }

    TagIteratorPtr TagContainer::getIterator() const
    {
        return TagIteratorPtr( new TagIterator( m_tags ) );
    }

    void TagContainer::removeAllTags()
    {
        m_tags.clear();
    }

    void copyTags(const Frame *from, Frame *to)
    {
        copyTags( &from->getTags(), &to->getTags() );

        const ChannelMap& channels = from->getChannels();

        for (ChannelMap::const_iterator it = channels.begin();
             it != channels.end();
             ++it)
        {
            const pfs::Channel *fromCh = it->second;
            pfs::Channel *toCh = to->getChannel( fromCh->getName() );

            // Skip if there is no corresponding channel
            if ( toCh != NULL )
            {
                copyTags( fromCh->getTags(), toCh->getTags() );
            }
        }
    }

    void copyTags(const TagContainer *f, TagContainer *t)
    {
        t->removeAllTags();

        for (TagList::const_iterator it = f->tagsBegin();
             it != f->tagsEnd();
             it++)
        {
            t->appendTag( *it );
        }
    }

    void readTags( TagContainer *tags, FILE *in )
    {
        int readItems;
        int tagCount;
        readItems = fscanf( in, "%d" PFSEOL, &tagCount );
        if ( readItems != 1 || tagCount < 0 || tagCount > 1024 )
        {
            throw Exception( "Corrupted PFS tag section: missing or wrong number of tags" );
        }

        char buf[MAX_TAG_STRING+1];
        for( int i = 0; i < tagCount; i++ )
        {
            char *read = fgets( buf, MAX_TAG_STRING, in );
            if( read == NULL ) throw Exception( "Corrupted PFS tag section: missing tag" );
            char *equalSign = strstr( buf, "=" );
            if( equalSign == NULL ) throw Exception( "Corrupted PFS tag section ('=' sign missing)" );
            tags->appendTagEOL( buf );
        }
    }

    void writeTags( const TagContainer *tags, FILE *out )
    {
        fprintf( out, "%d" PFSEOL, tags->getSize() );
        for (TagList::const_iterator it = tags->tagsBegin(); it != tags->tagsEnd(); it++ )
        {
            fprintf( out, "%s", it->c_str() );
            fprintf( out, PFSEOL );
        }
    }

}





