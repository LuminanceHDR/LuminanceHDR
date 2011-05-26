/**
 * @file
 * @brief PFS library - core API interfaces
 *
 * Classes for reading and writing a stream of PFS frames.
 *
 * Note on the design of pfs library API: pfs library API makes
 * extensive usage of interfaces - classes that have only virtual
 * methods. This way no private fields are visible for the client
 * programs. Everything that is private is hidden in .cpp file rather
 * than the header .h. For example, pfs library uses STL to store some
 * internal data, but no STL class can be found the header file
 * pfs.h. Such design should hopefully give less problems when
 * extending and updating the library.
 * 
 * This file is a part of PFSTOOLS package.
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
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#ifndef PFS_H
#define PFS_H

#include <string.h>

#define PFSEOL "\x0a"
#define PFSEOLCH '\x0a'

#define MAX_RES 65535
#define MAX_CHANNEL_NAME 32
#define MAX_TAG_STRING 1024
#define MAX_CHANNEL_COUNT 1024

/**
 * All classes and function from PFS library reside in pfs namespace. 
 */
namespace pfs
{

  /**
   * Utility class that keeps pointer and deletes pointed object
   * when the class is deleted.
   *
   * Note that it is not a full implementation of the smart pointer
   * and memory management is not fool proof. You should never store
   * this object as a global variable or a field of a class. These
   * objects should be used only as local variables.
   */
  template<class T>
    class SelfDestructPtr
    {
      T *ptr;
      mutable bool itsOwn;
    public:
      explicit SelfDestructPtr( T *ptr = 0 ) /*: ptr(ptr) , itsOwn(ptr!=0) */
        {
          this->ptr = ptr;
          if (this->ptr != 0) itsOwn = true;
        }

      SelfDestructPtr( const SelfDestructPtr& r ) 
      /*  : itsOwn(r.itsOwn), ptr(r.release()) */
      {
        itsOwn = r.itsOwn;
        ptr = r.release();
      }
      
      SelfDestructPtr& operator=( const SelfDestructPtr& r ) {
        if (&r != this) {
            if (ptr != r.ptr) {
              if( itsOwn ) delete ptr;
              itsOwn = r.itsOwn;
            }
            else if( r.itsOwn ) itsOwn = true;
            ptr = r.release();
        }
        return *this;
      }
      
      ~SelfDestructPtr()
        {
          if( itsOwn ) 
            delete ptr;
        }  

      bool operator==( const SelfDestructPtr &x ) const {
        return *(ptr) == *(x.ptr);
      }

      bool operator!=( const SelfDestructPtr &x ) const {
        return *(ptr) != *(x.ptr);
      }
      
      T& operator*()  const            {return *ptr;}
      T* operator->() const            {return ptr;}
      T* get()        const            {return ptr;}
      T* release()    const  {itsOwn = false; return ptr;}
      
    };

  /**
   * A pair of a file name and file handler, returned from
   * FrameFileIterator.
   */
//  struct FrameFile
//  {
//    FrameFile( FILE *fh, const char* fileName ): fh(fh), fileName( fileName )
//    {
//    }

//    /**
//     * File handler.
//     */
//    FILE *fh;

//    /**
//     * File name.
//     */
//    const char *fileName;
//  };

//class FrameFileIteratorImpl;


///**
// * Utility class that can be used to iterate over file names
// * specified as command line arguments. It can handle patterns,
// * like frame%04d.hdr, where %04d is replaced with specified
// * range of frame numbers.
// *
// */

//  class FrameFileIterator
//    {
//      FrameFileIteratorImpl *impl;
//    public:
//      /**
//       * Creates new iterator over frame files. Command line
//       * arguments are parsed and all recognized arguments are
//       * removed.
//       *
//       * @param argc argument count passed to program's main function.
//       * @param argv argument values passed to program's main function.
//       * @param fopenMode mode used to fopen frame files, usually "rb" or "wb"
//       * @param fileNamePrefix each frame pattern must be preceded
//       * with this string (for example "-i'). If NULL, every argument that
//       * does not start with "-" is treated as a frame pattern.
//       * @param stdinout if set, treat '-' file name specially and instead
//       * of opening a named file, use filedescriptor passed as this parameter.
//       * It should be used to get or write data to stdin / stdout.
//       * @param optstring parameter string passed to getopt()
//       * function. When optstring != NULL, FrameFileIterator will skip
//       * all parameters and their required arguments. Optional
//       * arguments are not handled.
//       * @param getopt_long parameter structure passed to getopt_long()
//       * function. When getopt_long != NULL, FrameFileIterator will skip
//       * all parameters and their required arguments. Optional
//       * arguments are not handled.
//       * @throws CommandLineException on bad syntax of command line options
//       */
//      FrameFileIterator( int &argc, char* argv[], const char *fopenMode,
//        const char *fileNamePrefix = NULL, FILE *stdinout = NULL,
//        const char *optstring = NULL, const struct option *getopt_long = NULL );
//      ~FrameFileIterator();

//      /**
//       * Get the file handle FILE* and file name for the next
//       * frame. Note that fileName string is valid until next
//       * call to getNextFrameFile or closeFrameFile.
//       *
//       * When file handle is no longer needed, closeFileFile
//       * should be called.
//       *
//       * @return file handle FILE* and file name of the next frame.
//       * Returns file handle == NULL if there are no more frames.
//       *
//       * @throws Exception if the file is not found
//       */
//      FrameFile getNextFrameFile( );

//      /**
//       * Close file openned with getNextFrameFile.
//       *
//       * @param frameFile FrameFile object returned from getNextFrameFile
//       */
//      void closeFrameFile( FrameFile &frameFile );

//      static void printUsage( FILE *out, const char *progName );

//    };

/**
 * General exception class used to throw exceptions from pfs library.
 */
  class Exception
    {
      char msg[1024];
    public:
      /**
       * Creates a new exception.
       *
       * @param message description of the cause for the
       * exception. The copy of the message string is made, so it can
       * be freed after creating Exception.
       */
      Exception( const char* const message )
	{
          strcpy( msg, message );
	}
			
      ~Exception() {}

      /**
       * Returns the description of the problem.
       *
       * @return text description of the cause for the exception
       */
      const char* getMessage()
        {
          return msg;
	}
    };


/**
 * Exception class used to throw exceptions from FileFileIterator class.
 */
//  class CommandLineException: public Exception
//    {
//    public:
//      CommandLineException( const char* const message ): Exception( message )
//	{
//	}
//    };


}



#endif
