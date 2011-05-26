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

#include <list>
#include <string>

using namespace std;

namespace pfs
{

//   TagContainerImpl::~TagContainerImpl()
//   {
//     tagList.clear();
//   }

    TagList::const_iterator TagContainerImpl::tagsBegin() const
  {
    return tagList.begin();
  }

    TagList::const_iterator TagContainerImpl::tagsEnd() const
  {
    return tagList.end();
  }

  int TagContainerImpl::getSize() const
  {
    return (int)tagList.size();
  }

  void TagContainerImpl::appendTagEOL( const char *tagValue )
  {
    assert( tagValue[strlen( tagValue ) -1] == PFSEOLCH );
    tagList.push_back( string( tagValue, strlen( tagValue ) -1 ) );
  }

  void TagContainerImpl::appendTag( const string &tagValue )
  {
    tagList.push_back( tagValue );
  }

  TagList::iterator TagContainerImpl::findTag( const char *tagName )
  {
    size_t tagNameLen = strlen( tagName );
    TagList::iterator it;
    for( it = tagList.begin(); it != tagList.end(); it++ )
    {
      if( !memcmp( tagName, it->c_str(), tagNameLen ) ) break; // Found
    }
    return it;
  }

  void TagContainerImpl::setTag( const char *tagName, const char *tagValue )
  {
    string tagVal( tagName );
    tagVal += "=";
    tagVal += tagValue;

    TagList::iterator element = findTag( tagName );
    if( element == tagList.end() ) { // Does not exist
      tagList.push_back( tagVal );
    } else {                // Already exist
      *element = tagVal;
    }
  }

  const char *TagContainerImpl::getTag( const char *tagName )
  {
    TagList::iterator element = findTag( tagName );
    if( element == tagList.end() ) return NULL;

    std::string::size_type equalSign = element->find( '=' );
    assert( equalSign != string::npos );

    return element->c_str() + equalSign + 1;
  }


  //Implementation of TagContainer
  const char* TagContainerImpl::getString( const char *tagName )
  {
    return getTag( tagName );
  }

  void TagContainerImpl::setString( const char *tagName, const char *tagValue )
  {
    setTag( tagName, tagValue );
  }

  void TagContainerImpl::removeTag( const char *tagName )
  {
    TagList::iterator element = findTag( tagName );
    if( element != tagList.end() ) tagList.erase( element );
  }

  TagIteratorPtr TagContainerImpl::getIterator() const
  {
    return TagIteratorPtr( new TagIteratorImpl( tagList ) );
  }

  void TagContainerImpl::removeAllTags()
  {
    tagList.clear();
  }

  void copyTags( Frame *from, Frame *to )
  {
      copyTags( from->getTags(), to->getTags() );
      pfs::ChannelIterator *it = from->getChannels();
      while ( it->hasNext() )
      {
          pfs::Channel *fromCh = it->getNext();
          pfs::Channel *toCh = to->getChannel( fromCh->getName() );
          if ( toCh == NULL ) // Skip if there is no corresponding channel
              continue;
          copyTags( fromCh->getTags(), toCh->getTags() );
      }

  }

  void copyTags( const TagContainer *from, TagContainer *to )
  {
      TagContainerImpl *f = (TagContainerImpl*)from;
      TagContainerImpl *t = (TagContainerImpl*)to;

      t->removeAllTags();

      TagList::const_iterator it;
      for( it = f->tagsBegin(); it != f->tagsEnd(); it++ ) {
          t->appendTag( *it );
      }
  }

  void readTags( TagContainerImpl *tags, FILE *in )
  {
      int readItems;
      int tagCount;
      readItems = fscanf( in, "%d" PFSEOL, &tagCount );
      if( readItems != 1 || tagCount < 0 || tagCount > 1024 )
          throw Exception( "Corrupted PFS tag section: missing or wrong number of tags" );

      char buf[MAX_TAG_STRING+1];
      for( int i = 0; i < tagCount; i++ ) {
          char *read = fgets( buf, MAX_TAG_STRING, in );
          if( read == NULL ) throw Exception( "Corrupted PFS tag section: missing tag" );
          char *equalSign = strstr( buf, "=" );
          if( equalSign == NULL ) throw Exception( "Corrupted PFS tag section ('=' sign missing)" );
          tags->appendTagEOL( buf );
      }
  }

  void writeTags( const TagContainerImpl *tags, FILE *out )
  {
      TagList::const_iterator it;
      fprintf( out, "%d" PFSEOL, tags->getSize() );
      for( it = tags->tagsBegin(); it != tags->tagsEnd(); it++ ) {
          fprintf( out, "%s", it->c_str() );
          fprintf( out, PFSEOL );
      }
  }

}





