/*
     PLIB - A Suite of Portable Game Libraries
     Copyright (C) 2001  Steve Baker
 
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
 
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
 
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the Free
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 
     For further information visit http://plib.sourceforge.net

     $Id$
*/


#include "puLocal.h"

#define PUSTACK_MAX 100

static int currLiveInterface = -1 ;
static puInterface *liveInterfaceStack [ PUSTACK_MAX ] ;

void puPushLiveInterface ( puInterface *in )
{
  if ( currLiveInterface < PUSTACK_MAX )
    liveInterfaceStack [ ++currLiveInterface ] = in ;
  else
    ulSetError ( UL_WARNING, "PUI: Too many live puInterfaces open at once!\n" ) ;
}

void  puPopLiveInterface ( puInterface *in )
{
  if ( currLiveInterface <= 0 )
  {
    ulSetError ( UL_WARNING, "PUI: Live puInterface stack is empty!\n" ) ;
    return;
  }

  if ( in == NULL || in == liveInterfaceStack [ currLiveInterface ] )
  {
    --currLiveInterface ;
  }
  else
  {
    //Houston, we have a problem...
    for ( int i = currLiveInterface-1 ; i >= 0 ; i-- )
    {
      if ( in == liveInterfaceStack [ i ] )
      {
        //error-- the interface is buried in the stack
        //interface creation/deletion should be nested (LIFO)
        ulSetError ( UL_FATAL, "PUI: interface stack error!\n" ) ;
      }
    }
  }
}

int  puNoLiveInterface ( void )
{
  return currLiveInterface < 0 ;
}

puInterface *puGetUltimateLiveInterface ( void )
{
  if ( currLiveInterface < 0 )
  {
    ulSetError ( UL_WARNING, "PUI: No Live Interface!\n" ) ;
    return NULL ;
  }

  return liveInterfaceStack [ 0 ] ;
}


puInterface *puGetBaseLiveInterface ( void )
{
  if ( currLiveInterface < 0 )
  {
    ulSetError ( UL_WARNING, "PUI: No Live Interface!\n" ) ;
    return NULL ;
  }

  /*
    Work down the interface stack until you
    either get to the bottom or find a block
    in the form of a puDialogBox.
  */

  for ( int i = currLiveInterface ; i > 0 ; i-- )
    if ( liveInterfaceStack [ i ] -> getType () & PUCLASS_DIALOGBOX )
      return liveInterfaceStack [ i ] ; 

  return liveInterfaceStack [ 0 ] ;
}


puInterface::~puInterface ()
{
  puPopLiveInterface ( this ) ;

  for ( puObject *bo = dlist ; bo != NULL ; /* Nothing */ )
  {
    dlist = bo    ;
    bo = bo->next ;
    delete dlist  ;
  }

  if ( this == puActiveWidget () )
    puDeactivateWidget () ;
}
