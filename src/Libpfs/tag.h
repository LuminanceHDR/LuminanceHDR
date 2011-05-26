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
 
#ifndef __PFS_TAG_H__
#define __PFS_TAG_H__

#include <list>
#include <string>
#include <assert.h>

#include "pfs.h"

using namespace std;

namespace pfs
{
    class Frame;

    typedef list<string> TagList;

    /**
 * Iterator that allows to get the list of available tags in a
 * TagContainer.
 */
    class TagIterator
    {
    public:
        /**
     * Get next item on the list.
     *
     * @return name of the tag
     */
        virtual const char *getNext() = 0;
        /**
     * Returns true if there is still an item left on the list.
     */
        virtual bool hasNext() const = 0;
    };

    //------------------------------------------------------------------------------
    // TagContainer implementation
    //------------------------------------------------------------------------------
    class TagIteratorImpl: public TagIterator
    {
        TagList::const_iterator it;
        const TagList &tagList;
        string tagName;

    public:
        TagIteratorImpl( const TagList &tagList ) : tagList( tagList )
        {
            it = tagList.begin();
        }

        /**
       * Get next item on the list.
       */
        const char *getNext()
        {
            const string &tag = *(it++);
            size_t equalSign = tag.find( '=' );
            //assert( equalSign != -1 );
            assert( equalSign != string::npos );
            tagName = string( tag, 0, equalSign );
            return tagName.c_str();
        }

        /**
       * Returns true if there is still an item left on the list.
       */
        bool hasNext() const
        {
            return it != tagList.end();
        }

    };

    typedef SelfDestructPtr<TagIterator> TagIteratorPtr;

    //------------------------------------------------------------------------------
    // TagContainer interface allows to read and modify tags. A tag is "name"="value" pair.
    // ------------------------------------------------------------------------------
    class TagContainer
    {
    public:
        /**
     * Get a string tag of the name tagName from the TagContainer.
     * @param tagName name of the tag to retrieve
     * @return tag value or NULL if tag was not found
     */
        virtual const char* getString( const char *tagName ) = 0;

        /**
     * Set or add a string tag of the name tagName.
     * @param tagName name of the tag to add or set
     * @param tagValue value of the tag
     */
        virtual void setString( const char *tagName, const char *tagValue ) = 0;

        /**
     * Removes (if exists) a tag of the name tagName from the TagContainer.
     * @param tagName name of the tag to remove
     */
        virtual void removeTag( const char *tagName ) = 0;

        /**
     * Use TagIterator to iterate over all tags in the TagContainer.
     * TagIteratorPtr is a smart pointer, which destructs
     * TagIterator when TagIteratorPtr is destructed. Use ->
     * operator to access TagIterator members from a TagIteratorPtr
     * object.
     *
     * To iterate over all tags, use the following code:
     * <code>
     * pfs::TagIteratorPtr it( frame->getTags()->getIterator() );
     * while( it->hasNext() ) {
     *   const char *tagName = it->getNext();
     *   //Do something
     * }
     * </code>
     */
        virtual TagIteratorPtr getIterator() const = 0;
    };

    class TagContainerImpl: public TagContainer
    {
    public:
    private:

      TagList tagList;

    public:

    //   ~TagContainerImpl()
    //   {
    //     tagList.clear();
    //   }

      TagList::const_iterator tagsBegin() const;

      TagList::const_iterator tagsEnd() const;

      int getSize() const;

      void appendTagEOL( const char *tagValue );

      void appendTag( const string &tagValue );

      TagList::iterator findTag( const char *tagName );

      void setTag( const char *tagName, const char *tagValue );

      const char *getTag( const char *tagName );

      //Implementation of TagContainer
      const char* getString( const char *tagName );

      void setString( const char *tagName, const char *tagValue );

      void removeTag( const char *tagName );

      TagIteratorPtr getIterator() const;

      void removeAllTags();
    };



    /**
 * Copy all tags from both the frame and its channels to the
 * destination frame. If there is no corresponding destination
 * channel for a source channel, the tags from that source channel
 * will not be copied. Note, that all tags in the destination
 * channel will be removed before copying. Therefore after this
 * operation, the destination will contain exactly the same tags as
 * the source.
 */
    void copyTags( Frame *from, Frame *to );

    /**
 * Copy all tags from one container into another. Note, that all
 * tags in the destination channel will be removed before
 * copying. Therefore after this operation, the destination will
 * contain exactly the same tags as the source.
 */
    void copyTags( const TagContainer *from, TagContainer *to );

    void writeTags( const TagContainerImpl *tags, FILE *out );
    void readTags( TagContainerImpl *tags, FILE *in );

}

#endif // __PFS_TAG_H__


