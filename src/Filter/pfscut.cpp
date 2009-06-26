/**
 * @brief Cut a rectangle out of images in PFS stream
 *
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 *
 * @author Dorota Zdrojewska, <dzdrojewska@wi.ps.pl>
 * adapted by Franco Comida <fcomida@sourceforge.net> for qtpfsgui
 *
 */

#include "pfscut.h"
#include <climits>
#include <cstdlib>

#define UNSP INT_MAX
#define MIN 0
#define MAX 1
#define SIZE 2

void calcBorders(int min, int max, int size, int inSize, int flag, int* out)
{

if (min!=UNSP && max!=UNSP && size==UNSP) {
    out[MIN]=min;
    out[MAX]=inSize-max-1;
    out[SIZE]=inSize-min-max;
    }

if (min!=UNSP && max==UNSP && size==UNSP) {
    out[MIN]=min;
    out[MAX]=inSize-1;
    out[SIZE]=inSize-min;
    }

if (min==UNSP && max!=UNSP && size==UNSP) {
    out[MIN]=0;
    out[MAX]=inSize-max-1;
    out[SIZE]=inSize-max;
    }

if (min!=UNSP && max==UNSP && size!=UNSP) {
    out[MIN]=min;
    out[MAX]=size+min-1;
    out[SIZE]=size;
    }

if (min==UNSP && max!=UNSP && size!=UNSP) {
    out[MIN]=inSize-max-size;
    out[MAX]=inSize-max-1;
    out[SIZE]=size;
    }

if (min==UNSP && max==UNSP && size!=UNSP) {
    int diff=inSize-size;
    out[MIN]=diff/2;
    out[MAX]=inSize-(diff/2)-1;
    if(diff%2) out[MAX]--;
    out[SIZE]=size;
    }

if (min==UNSP && max==UNSP && size==UNSP) {
    out[MIN]=0;
    out[MAX]=inSize-1;
    out[SIZE]=inSize;
    }

}

pfs::Frame *pfscut(pfs::Frame *inFrame, int x_ul, int y_ul, int x_br, int y_br)
{
  pfs::DOMIO pfsio;

  //numbers of pixels to cut from each border of an image 
  int left=UNSP, right=UNSP, top=UNSP, bottom=UNSP; 
  int width=UNSP, height=UNSP; //size of an output image
  //int x_ul=UNSP, y_ul=UNSP, x_br=UNSP, y_br=UNSP;
    
  int optionIndex=0;

  while (1) {

    int inWidth=inFrame->getWidth();
    int inHeight=inFrame->getHeight();

    int leftRight[3], topBottom[3];
    if( x_ul != UNSP ) {
      leftRight[MIN] = x_ul;
      leftRight[MAX] = x_br;
      leftRight[SIZE] = x_br - x_ul + 1;
      topBottom[MIN] = y_ul;
      topBottom[MAX] = y_br;
      topBottom[SIZE] = y_br - y_ul + 1;
    } else {
      //calculate edge columns and rows of an input image to be in an output image  
      calcBorders(left, right, width, inWidth, 1, leftRight);
      calcBorders(top, bottom, height, inHeight, 0, topBottom);
    }
    
    int lCol=leftRight[MIN];
    int rCol=leftRight[MAX];
    int tRow=topBottom[MIN];
    int bRow=topBottom[MAX];

    int outWidth=leftRight[SIZE];
    int outHeight=topBottom[SIZE];
  
    pfs::Frame *outFrame = pfsio.createFrame(outWidth, outHeight);

    pfs::ChannelIterator *it = inFrame->getChannels();
    
    while (it->hasNext()) {
	
	pfs::Channel *inCh = it->getNext();
	pfs::Channel *outCh = outFrame->createChannel(inCh->getName());
	
	for (int i=tRow; i<=bRow; i++)
	    for (int j=lCol; j<=rCol; j++) 
		(*outCh)(j-lCol, i-tRow)=(*inCh)(j,i);
	
    }
    
    pfs::copyTags(inFrame, outFrame);
    return outFrame;
  }

}

pfs::Frame *pfscopy(pfs::Frame *inFrame) {
  int w = inFrame->getWidth();
  int h = inFrame->getHeight();
  return pfscut(inFrame,0,0,w-1,h-1);
}
	
