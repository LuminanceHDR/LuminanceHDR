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

void copyTags( const TagContainer *from, TagContainer *to ) {
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

static void readTags( TagContainerImpl *tags, QDataStream &in )
{
	int tagCount;
	char singlechar[1];
	bool conversionok;
	QString tempstr;
	do {
		in.readRawData(singlechar,1);
		tempstr+=*singlechar;
	} while (memcmp(singlechar,PFSEOL,1));
	tagCount=tempstr.toInt(&conversionok);
	tempstr.clear();
	if( !conversionok )
		throw Exception( "Corrupted PFS tag section: missing or wrong number of tags" );

	for( int i = 0; i < tagCount; i++ ) {
		do {
			in.readRawData(singlechar,1);
			tempstr+=*singlechar;
		} while (memcmp(singlechar,PFSEOL,1));
		if( tempstr.isEmpty() ) throw Exception( "Corrupted PFS tag section: missing tag" );
		if( tempstr.indexOf("=") == -1 ) throw Exception( "Corrupted PFS tag section ('=' sign missing)" );
		tags->appendTagEOL( tempstr.toUtf8().constData() );
		tempstr.clear();
	}
}

static void writeTags( const TagContainerImpl *tags, QDataStream &out ) {
	TagList::const_iterator it;
	QString tagsize=QString("%1" PFSEOL).arg(tags->getSize());
	out.writeRawData(tagsize.toUtf8().constData(),tagsize.size());
	for( it = tags->tagsBegin(); it != tags->tagsEnd(); it++ ) {
		QString name=QString(it->c_str());
		out.writeRawData(name.toUtf8().constData(),name.size());
		out.writeRawData(PFSEOL,1);
	}
}



//------------------------------------------------------------------------------
// pfs IO
//------------------------------------------------------------------------------

class DOMIOImpl {
public:

Frame *readFrame( QString filepath ) {
	assert( !filepath.isEmpty() );
	int read; //number of bytes that have been read

	QFile file(filepath);
	file.open(QIODevice::ReadOnly);
	QDataStream in(&file);

	char buf[5];
	read = in.readRawData(buf,5);

	if( memcmp( buf, PFSFILEID, 5 ) ) throw Exception( "Incorrect PFS file header" );

	int width, height, channelCount;
	char singlechar[1];
	bool conversionok;
	QString tempstr;

	do {
		in.readRawData(singlechar,1);
		tempstr+=*singlechar;
	} while (memcmp(singlechar," ",1));
	width=tempstr.toInt(&conversionok);
	if (!conversionok) 
		throw Exception( "Corrupted PFS file: missing or wrong 'width'");
	tempstr.clear();

	do {
		in.readRawData(singlechar,1);
		tempstr+=*singlechar;
	} while (memcmp(singlechar,PFSEOL,1));
	height=tempstr.toInt(&conversionok);
	if (!conversionok) 
		throw Exception( "Corrupted PFS file: missing or wrong 'height'");
	tempstr.clear();

	do {
		in.readRawData(singlechar,1);
		tempstr+=*singlechar;
	} while (memcmp(singlechar,PFSEOL,1));
	channelCount=tempstr.toInt(&conversionok);
	if (!conversionok)
		throw Exception( "Corrupted PFS file: missing or wrong 'channelCount' tag");
	tempstr.clear();

	FrameImpl *frame = (FrameImpl*)createFrame( width, height );

	readTags( frame->tags, in );

	//Read channel IDs and tags
	list<ChannelImpl*> orderedChannel;
	for( int i = 0; i < channelCount; i++ ) {
		do {
			in.readRawData(singlechar,1);
			tempstr+=*singlechar;
		} while (memcmp(singlechar,PFSEOL,1));

		if( tempstr.isEmpty() ) {
			throw Exception( "Corrupted PFS file: missing channel name" );
		}
		tempstr=tempstr.trimmed();
		ChannelImpl *ch = (ChannelImpl*)frame->createChannel( tempstr.toUtf8().constData() );
		readTags( ch->tags, in );
		orderedChannel.push_back( ch );
		tempstr.clear();
	}

	char buf2[4];
	read = in.readRawData(buf2,4);
	if( read != 4 || memcmp( buf2, "ENDH", 4 ) ) 
		throw Exception( "Corrupted PFS file: missing end of header (ENDH) token" );
	//Read channels
	list<ChannelImpl*>::iterator it;
	for( it = orderedChannel.begin(); it != orderedChannel.end(); it++ ) {
		ChannelImpl *ch = *it;
		int size = frame->getWidth()*frame->getHeight();
		read = in.readRawData(reinterpret_cast<char*>(ch->getRawData()),4*size);
		if( read != 4*size )
			throw Exception( "Corrupted PFS file: missing channel data" );
	}
	file.close();
	return frame;
}


Frame *createFrame( int width, int height ) {
	Frame *frame = new FrameImpl( width, height );
	if( frame == NULL ) throw Exception( "Out of memory" );
	return frame;
}


void writeFrame( Frame *frame, QString filepath ) {
	assert( !filepath.isEmpty() );
	assert( frame != NULL );
	QFile file(filepath);
	file.open(QIODevice::WriteOnly);
	QDataStream out(&file);
	FrameImpl *frameImpl = (FrameImpl*)frame;
	out.writeRawData(PFSFILEID,5);// Write header ID
	QString width_and_height=QString("%1 %2" PFSEOL).arg(frame->getWidth()).arg(frame->getHeight());
	out.writeRawData(width_and_height.toUtf8().constData(),width_and_height.size());
	QString chansize=QString("%1" PFSEOL).arg(frameImpl->channel.size());
	out.writeRawData(chansize.toUtf8().constData(),chansize.size());

	writeTags( frameImpl->tags, out );

	//Write channel IDs and tags
	for( ChannelMap::iterator it = frameImpl->channel.begin(); it != frameImpl->channel.end(); it++ ) {
		QString name=QString("%1" PFSEOL).arg(it->second->getName());
		out.writeRawData(name.toUtf8().constData(),name.size());
		writeTags( it->second->tags, out );
	}

	out.writeRawData("ENDH",4);
	//Write channels
	for( ChannelMap::iterator it = frameImpl->channel.begin(); it != frameImpl->channel.end(); it++ ) {
		int size = frame->getWidth()*frame->getHeight();
		out.writeRawData(reinterpret_cast<const char*>(it->second->getRawData()),4*size);
	}
	file.close();
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

Frame *DOMIO::readFrame( QString filepath ) {
	return impl->readFrame( filepath );
}

void DOMIO::writeFrame( Frame *frame, QString filepath ) {
	impl->writeFrame( frame, filepath );
}

void DOMIO::freeFrame( Frame *frame ) {
	impl->freeFrame( frame );
}

}; //namespace pfs
