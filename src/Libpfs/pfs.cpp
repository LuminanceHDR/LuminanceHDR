/**
 * @brief PFS library - core API interfaces
 *
 * Classes for reading and writing a stream of PFS frames.
 * 
 * This file is a part of Qtpfsgui package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
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
 *
 * $Id: pfs.cpp,v 1.9 2006/10/26 14:57:54 gkrawczyk Exp $
 */

#include <string.h>
#include <assert.h>
#include <string>
#include <list>
#include <map>
#include <stdlib.h>

#include "pfs.h"

#define PFSEOL "\x0a"
#define PFSEOLCH '\x0a'

using namespace std;

namespace pfs
{

const char *PFSFILEID="PFS1\x0a";


//------------------------------------------------------------------------------
// TagContainer implementation  
//------------------------------------------------------------------------------

typedef list<string> TagList;

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
    int equalSign = tag.find( '=' );
    assert( equalSign != -1 );
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

class TagContainerImpl : public TagContainer
{
public:
private:

  TagList tagList;

public:

  TagList::const_iterator tagsBegin() const
  {
    return tagList.begin();
  }

  TagList::const_iterator tagsEnd() const
  {
    return tagList.end();
  }

  int getSize() const
  {
    return (int)tagList.size();
  }

  void appendTagEOL( const char *tagValue )
  {
    assert( tagValue[strlen( tagValue ) -1] == PFSEOLCH );
    tagList.push_back( string( tagValue, strlen( tagValue ) -1 ) );
  }

  void appendTag( const string &tagValue )
  {
    tagList.push_back( tagValue );
  }

  TagList::iterator findTag( const char *tagName )
  {
    size_t tagNameLen = strlen( tagName );
    TagList::iterator it;
    for( it = tagList.begin(); it != tagList.end(); it++ ) {
      if( !memcmp( tagName, it->c_str(), tagNameLen ) ) break; // Found
    }
    return it;
  }

  void setTag( const char *tagName, const char *tagValue )
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

  const char *getTag( const char *tagName )
  {
    TagList::iterator element = findTag( tagName );
    if( element == tagList.end() ) return NULL;

    string::size_type equalSign = element->find( '=' );
    assert( equalSign != string::npos );

    return element->c_str() + equalSign + 1;
  }


  //Implementation of TagContainer
  const char* getString( const char *tagName )
  {
    return getTag( tagName );
  }

  void setString( const char *tagName, const char *tagValue )
  {
    setTag( tagName, tagValue );
  }

  void removeTag( const char *tagName )
  {
    TagList::iterator element = findTag( tagName );
    if( element != tagList.end() ) tagList.erase( element );
  }

  TagIteratorPtr getIterator() const
  {
    return TagIteratorPtr( new TagIteratorImpl( tagList ) );
  }

  void removeAllTags()
  {
    tagList.clear();
  }


}; //TagContainerImpl



void copyTags( const TagContainer *from, TagContainer *to ) 
{
	TagContainerImpl *f = (TagContainerImpl*)from;
	TagContainerImpl *t = (TagContainerImpl*)to;
	
	t->removeAllTags();
	
	TagList::const_iterator it;
	for( it = f->tagsBegin(); it != f->tagsEnd(); it++ )
		t->appendTag( *it );
}

void copyTags( Frame *from, Frame *to ) {
	copyTags( from->getTags(), to->getTags() );
	pfs::ChannelIterator *it = from->getChannels();
	while( it->hasNext() ) {
		pfs::Channel *fromCh = it->getNext();
		pfs::Channel *toCh = to->getChannel( fromCh->getName() );
		if( toCh == NULL ) // Skip if there is no corresponding channel
		continue;
		copyTags( fromCh->getTags(), toCh->getTags() );
	}
}


//------------------------------------------------------------------------------
// Channel implementation  
//------------------------------------------------------------------------------
Channel::~Channel() {}

class DOMIOImpl;

class ChannelImpl: public Channel {
	int width, height;
	float *data;
	const char *name;

protected:
	friend class DOMIOImpl;
	
	TagContainerImpl *tags;

public:
ChannelImpl( int width, int height, const char *n_name ) : width(width), height(height) {
// 	fprintf(stderr,"constr Chan\n");
#ifndef _WIN32
	data = (float*)mmap(0, width*height*4, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON, -1, 0);
#else
	data = (float*)VirtualAlloc(NULL,width*height*4,MEM_COMMIT,PAGE_READWRITE);
#endif
//     data = new float[width*height];
	tags = new TagContainerImpl();
	name = strdup( n_name );
}

virtual ~ChannelImpl() {
	if (tags) delete tags;
#ifndef _WIN32
	if (data) munmap(data, width*height*4);
#else
	if (data) VirtualFree(data,0,MEM_RELEASE);
#endif
	//     if (data) delete[] data;
// 	fprintf(stderr,"free Chan\n");
	free( (void*)name );
}

// Channel implementation
TagContainer *getTags() {
	return tags;
}

float *getRawData() {
	return data;
}

//Array2D implementation

virtual int getCols() const {
	return width;
}

virtual int getRows() const {
	return height;
}

virtual const char *getName() const {
	return name;
}

virtual void setName(const char* newname) {
	name=strdup(newname);
}

inline float& operator()( int x, int y ) {
	assert( x >= 0 && x < width );
	assert( y >= 0 && y < height );
	return data[ x+y*width ];
}

inline const float& operator()( int x, int y ) const {
	assert( x >= 0 && x < width );
	assert( y >= 0 && y < height );
	return data[ x+y*width ];
}

inline float& operator()( int rowMajorIndex ) {
	assert( rowMajorIndex < width*height );
	assert( rowMajorIndex >= 0 );
	return data[ rowMajorIndex ];
}

inline const float& operator()( int rowMajorIndex ) const {
	assert( rowMajorIndex < width*height );
	assert( rowMajorIndex >= 0 );
	return data[ rowMajorIndex ];
}

};//ChannelImpl


//------------------------------------------------------------------------------
// Map of channels
//------------------------------------------------------------------------------

struct str_cmp: public std::binary_function<const char*,const char*,bool>
{
  bool operator()(const char* s1, const char* s2) const
  {
    return strcmp(s1, s2) < 0;
  }
};
typedef std::map<const char*, ChannelImpl*, str_cmp> ChannelMap;

//------------------------------------------------------------------------------
// Channel Iterator implementation
//-----------------------------------------------------------------------------

class ChannelIteratorImpl: public ChannelIterator
{
  ChannelMap::iterator it;
  ChannelMap *cm;
public:
  ChannelIteratorImpl( ChannelMap *cm ) : cm(cm)
  {
    reset();
  }

  void reset()
  {
    it = cm->begin();    
  }
  
  Channel *getNext() 
  {
    if( !hasNext() ) return NULL;
    return (it++)->second;
  }
    
  bool hasNext() const
  {
    return it != cm->end();
  }
    
};

//------------------------------------------------------------------------------
// Frame implementation  
//------------------------------------------------------------------------------

//A pure virtual destructor
Frame::~Frame() {}

class FrameImpl: public Frame {
  int width, height;

protected:
  friend class DOMIOImpl;

  TagContainerImpl *tags;
  ChannelMap channel;
  ChannelIteratorImpl channelIterator;

public:

  FrameImpl( int width, int height ): width( width ), height( height ), channelIterator( &channel ) 
  {
    tags = new TagContainerImpl();
  }

  ~FrameImpl()
  {
// 	fprintf(stderr, "begin ~FrameImpl\n");
    if (tags) delete tags;
    ChannelMap::iterator it;
    for( it = channel.begin(); it != channel.end(); ) {
//       fprintf(stderr,"deleting chan %s",it->first);
      Channel *ch = it->second;
      ChannelMap::iterator itToDelete = it; // Nasty trick because hashmap
                                            // elements point to string that is
                                            // freed by the channel 
      it++;
      channel.erase( itToDelete );
      if (ch) delete ch;
    }
// 	fprintf(stderr, "end ~FrameImpl\n");
  }

  virtual int getWidth() const
  {
    return width;
  }

  virtual int getHeight() const
  {
    return height;
  }

virtual void renameRGBChannelsToXYZ() {
	ChannelMap::iterator itR=channel.find("R");
	ChannelMap::iterator itG=channel.find("G");
	ChannelMap::iterator itB=channel.find("B");
	if( itR != channel.end() &&
	itG != channel.end() &&
	itB != channel.end() ) {
	//take pointers to fat greasy data
	ChannelImpl *chR=itR->second;
	ChannelImpl *chG=itG->second;
	ChannelImpl *chB=itB->second;
	//Set old channels' name to "X","Y","Z"
	chR->setName("X");
	chG->setName("Y");
	chB->setName("Z");
// 	fprintf(stderr,"ren::endsetname\n");
	//STL's erase, should be lightweight
	channel.erase(itR);
	channel.erase(itG);
	channel.erase(itB);
	channel.insert( pair<const char*, ChannelImpl*>(chR->getName(), chR) );
	channel.insert( pair<const char*, ChannelImpl*>(chG->getName(), chG) );
	channel.insert( pair<const char*, ChannelImpl*>(chB->getName(), chB) );
	}
// 	fprintf(stderr,"ren::end");
}

  virtual void renameXYZChannelsToRGB() {
	ChannelMap::iterator itX=channel.find("X");
	ChannelMap::iterator itY=channel.find("Y");
	ChannelMap::iterator itZ=channel.find("Z");
	if( itX != channel.end() &&
	itY != channel.end() &&
	itZ != channel.end() ) {
	//take pointers to fat greasy data
	ChannelImpl *chX=itX->second;
	ChannelImpl *chY=itY->second;
	ChannelImpl *chZ=itZ->second;
	//Set old channels' name to "X","Y","Z"
	chX->setName("R");
	chY->setName("G");
	chZ->setName("B");
	//STL's erase, should be lightweight
	channel.erase(itX);
	channel.erase(itY);
	channel.erase(itZ);
	channel.insert( pair<const char*, ChannelImpl*>(chX->getName(), chX) );
	channel.insert( pair<const char*, ChannelImpl*>(chY->getName(), chY) );
	channel.insert( pair<const char*, ChannelImpl*>(chZ->getName(), chZ) );
	}
  }

  virtual void convertRGBChannelsToXYZ () {
    if( channel.find("R") != channel.end() ||
      channel.find("G") != channel.end() ||
      channel.find("B") != channel.end() ) {
	//RGB->XYZ
	transformColorSpace(CS_RGB,channel["R"],channel["G"],channel["B"],CS_XYZ,channel["R"],channel["G"],channel["B"]);
	renameRGBChannelsToXYZ();
    }
  }
  virtual void convertXYZChannelsToRGB () {
    if( channel.find("X") != channel.end() ||
      channel.find("Y") != channel.end() ||
      channel.find("Z") != channel.end() ) {
	//XYZ->RGB
	transformColorSpace(CS_XYZ,channel["X"],channel["Y"],channel["Z"],CS_RGB,channel["X"],channel["Y"],channel["Z"]);
	renameXYZChannelsToRGB();
    }
  }

  virtual void getXYZChannels( Channel* &X, Channel* &Y, Channel* &Z ) {

    if( channel.find("X") == channel.end() ||
      channel.find("Y") == channel.end() ||
      channel.find("Z") == channel.end() ) {
      X = Y = Z = NULL;
    } else {
      X = channel["X"];
      Y = channel["Y"];
      Z = channel["Z"];
    }
  }
  virtual void getRGBChannels( Channel* &R, Channel* &G, Channel* &B ) {
    if( channel.find("R") == channel.end() ||
      channel.find("G") == channel.end() ||
      channel.find("B") == channel.end() ) {
      R = G = B = NULL;
    } else {
      R = channel["R"];
      G = channel["G"];
      B = channel["B"];
    }
  }

  virtual void createXYZChannels( Channel* &X, Channel* &Y, Channel* &Z )
  {
    X = createChannel("X");
    Y = createChannel("Y");
    Z = createChannel("Z");
  }
  virtual void createRGBChannels( Channel* &R, Channel* &G, Channel* &B )
  {
    R = createChannel("R");
    G = createChannel("G");
    B = createChannel("B");
  }
  
  Channel* getChannel( const char *name )
  {
    ChannelMap::iterator it = channel.find(name);
    if( it == channel.end() )
      return NULL;
    else
      return it->second;
    
  }
  
  Channel *createChannel( const char *name )
  {
    ChannelImpl *ch;
    if( channel.find(name) == channel.end() ) {
      ch = new ChannelImpl( width, height, name );
      channel.insert( pair<const char*, ChannelImpl*>(ch->getName(), ch) );
    } else
      ch = channel[name];
    
    return ch;
  }

  void removeChannel( Channel *ch )
  {
    assert( ch != NULL );
    ChannelMap::iterator it = channel.find( ch->getName() );
    assert( it != channel.end() && it->second == ch );
    
    channel.erase( it );
    delete ch;
  }
  
  ChannelIterator *getChannels()
  {
    channelIterator.reset();
    return &channelIterator;
  }

  ChannelIteratorPtr getChannelIterator()
  {
    return ChannelIteratorPtr( new ChannelIteratorImpl( &channel ) );
  }

  TagContainer *getTags()
  {
    return tags;
  }


};

static void readTags( TagContainerImpl *tags, FILE *in )
{
  int readItems;
  int tagCount;
  readItems = fscanf( in, "%d" PFSEOL, &tagCount );
  if( readItems != 1 || tagCount < 0 || tagCount > 1024 )
    throw Exception( "Corrupted PFS tag section: missing or wrong number of tags" );

  char buf[1024];
  for( int i = 0; i < tagCount; i++ ) {
    char *read = fgets( buf, 1024, in );
    if( read == NULL ) throw Exception( "Corrupted PFS tag section: missing tag" );
    char *equalSign = strstr( buf, "=" );
    if( equalSign == NULL ) throw Exception( "Corrupted PFS tag section ('=' sign missing)" );
    tags->appendTagEOL( buf );
  }
}

static void writeTags( const TagContainerImpl *tags, FILE *out )
{
  TagList::const_iterator it;
  fprintf( out, "%d" PFSEOL, tags->getSize() );
  for( it = tags->tagsBegin(); it != tags->tagsEnd(); it++ ) {
    fprintf( out, it->c_str() );
    fprintf( out, PFSEOL );
  }
}




//------------------------------------------------------------------------------
// pfs IO
//------------------------------------------------------------------------------

class DOMIOImpl {
public:

Frame *readFrame( const char* filename ) {
	assert( strlen(filename)!=0 );
	FILE * inputStream=fopen(filename,"rb");
    size_t read;

    char buf[5];
    read = fread( buf, 1, 5, inputStream );
    if( read == 0 ) return NULL; // EOF

    if( memcmp( buf, PFSFILEID, 5 ) ) throw Exception( "Incorrect PFS file header" );

    int width, height, channelCount;
    read = fscanf( inputStream, "%d %d" PFSEOL, &width, &height );
    if( read != 2 || width <= 0 || height <= 0 )
      throw Exception( "Corrupted PFS file: missing or wrong 'width', 'height' tags" );
    read = fscanf( inputStream, "%d" PFSEOL, &channelCount );
    if( read != 1 || channelCount < 0 || channelCount > 1024 )
      throw Exception( "Corrupted PFS file: missing or wrong 'channelCount' tag" );

    FrameImpl *frame = (FrameImpl*)createFrame( width, height );

    readTags( frame->tags, inputStream );

    //Read channel IDs and tags
    //       FrameImpl::ChannelID *channelID = new FrameImpl::ChannelID[channelCount];
    list<ChannelImpl*> orderedChannel;
    for( int i = 0; i < channelCount; i++ ) {
      char channelName[10], *rs;
      rs = fgets( channelName, 10, inputStream );
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
    list<ChannelImpl*>::iterator it;
    for( it = orderedChannel.begin(); it != orderedChannel.end(); it++ ) {
      ChannelImpl *ch = *it;
      unsigned int size = frame->getWidth()*frame->getHeight();
      read = fread( ch->getRawData(), sizeof( float ), size, inputStream );
      if( read != size )
        throw Exception( "Corrupted PFS file: missing channel data" );
    }
	fclose(inputStream);
	return frame;
}


Frame *createFrame( int width, int height ) {
	Frame *frame = new FrameImpl( width, height );
	if( frame == NULL ) throw Exception( "Out of memory" );
	return frame;
}


void writeFrame( Frame *frame, const char* outputFileName ) {
	assert(strlen(outputFileName)!=0);
	assert( frame != NULL );
	FILE *outputStream=fopen(outputFileName,"wb");
	FrameImpl *frameImpl = (FrameImpl*)frame;

    fwrite( PFSFILEID, 1, 5, outputStream ); // Write header ID
    
    fprintf( outputStream, "%d %d" PFSEOL, frame->getWidth(), frame->getHeight() );
    fprintf( outputStream, "%d" PFSEOL, frameImpl->channel.size() );

    writeTags( frameImpl->tags, outputStream );

    //Write channel IDs and tags
    for( ChannelMap::iterator it = frameImpl->channel.begin(); it != frameImpl->channel.end(); it++ ) {
      fprintf( outputStream, "%s" PFSEOL, it->second->getName() );
      writeTags( it->second->tags, outputStream );
    }

    fprintf( outputStream, "ENDH");
    
    //Write channels
    for( ChannelMap::iterator it = frameImpl->channel.begin(); it != frameImpl->channel.end(); it++ ) {
        int size = frame->getWidth()*frame->getHeight();
        fwrite( it->second->getRawData(), sizeof( float ), size, outputStream );
    }

	fclose(outputStream);
}

void freeFrame( Frame *frame ) {
	delete frame;
}

}; //DOMIOImpl


DOMIO::DOMIO() {
	impl = new DOMIOImpl();
}

DOMIO::~DOMIO() {
	delete impl;
}

Frame *DOMIO::createFrame( int width, int height ) {
	return impl->createFrame( width, height );
}

Frame *DOMIO::readFrame( const char *filepath ) {
	return impl->readFrame( filepath );
}

void DOMIO::writeFrame( Frame *frame, const char * filepath ) {
	impl->writeFrame( frame, filepath );
}

void DOMIO::freeFrame( Frame *frame ) {
	impl->freeFrame( frame );
}

}; //namespace pfs
