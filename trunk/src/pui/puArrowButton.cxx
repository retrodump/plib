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

void puArrowButton::draw ( int dx, int dy )
{
  if ( !visible || ( window != puGetWindow () ) ) return ;

  /*
    If arrow button is pushed or highlighted -
    use inverse style for button itself
  */

  int tempStyle;

  if ( parent && ( ( parent->getType() & PUCLASS_POPUPMENU ) ||
                   ( parent->getType() & PUCLASS_MENUBAR   ) ) )
    tempStyle = ( getValue() ^ highlighted ) ? PUSTYLE_SMALL_SHADED : style ;
  else
    tempStyle = ( getValue() ^ highlighted ) ? -style : style ;

  abox . draw ( dx, dy, tempStyle, colour, isReturnDefault() ) ;

  /*
    If greyed out then halve the opacity when drawing
    the label and legend
  */

  if ( active )
    glColor4fv ( colour [ PUCOL_LABEL ] ) ;
  else
    glColor4f ( colour [ PUCOL_LABEL ][0],
                colour [ PUCOL_LABEL ][1],
                colour [ PUCOL_LABEL ][2],
                colour [ PUCOL_LABEL ][3] / 2.0f ) ; /* 50% more transparent */ 

  if ( r_cb )
    r_cb ( this, dx, dy, render_data ) ;
  else
  {
    int size_x = abox.max[0] - abox.min[0] ;
    int size_y = abox.max[1] - abox.min[1] ;

    int pos_x = dx + ( abox.max[0] + abox.min[0] ) / 2 ;
    int pos_y = dy + ( abox.max[1] + abox.min[1] ) / 2 ;

    switch ( arrow_type )
    {
    case PUARROW_UP :
      glBegin    ( GL_TRIANGLES ) ;
		glVertex2i ( pos_x - size_x/4, pos_y - size_y/4 ) ;
		glVertex2i ( pos_x           , pos_y + size_y/4 ) ;
		glVertex2i ( pos_x + size_x/4, pos_y - size_y/4 ) ;
      glEnd      () ;
      break;

    case PUARROW_DOWN :
      glBegin    ( GL_TRIANGLES ) ;
		glVertex2i ( pos_x - size_x/4, pos_y + size_y/4 ) ;
		glVertex2i ( pos_x           , pos_y - size_y/4 ) ;
		glVertex2i ( pos_x + size_x/4, pos_y + size_y/4 ) ;
      glEnd      () ;
      break;

    case PUARROW_FASTUP :
      glBegin    ( GL_TRIANGLES ) ;
		glVertex2i ( pos_x - size_x/4, pos_y - size_y/4 ) ;
		glVertex2i ( pos_x           , pos_y            ) ;
		glVertex2i ( pos_x + size_x/4, pos_y - size_y/4 ) ;

		glVertex2i ( pos_x - size_x/4, pos_y            ) ;
		glVertex2i ( pos_x           , pos_y + size_y/4 ) ;
		glVertex2i ( pos_x + size_x/4, pos_y            ) ;
      glEnd      () ;
      break;

    case PUARROW_FASTDOWN :
      glBegin    ( GL_TRIANGLES ) ;
		glVertex2i ( pos_x - size_x/4, pos_y + size_y/4 ) ;
		glVertex2i ( pos_x           , pos_y            ) ;
		glVertex2i ( pos_x + size_x/4, pos_y + size_y/4 ) ;

		glVertex2i ( pos_x - size_x/4, pos_y            ) ;
		glVertex2i ( pos_x           , pos_y - size_y/4 ) ;
		glVertex2i ( pos_x + size_x/4, pos_y            ) ;
      glEnd      () ;
      break;

    case PUARROW_LEFT :
      glBegin    ( GL_TRIANGLES ) ;
		glVertex2i ( pos_x + size_x/4, pos_y - size_y/4 ) ;
		glVertex2i ( pos_x - size_x/4, pos_y            ) ;
		glVertex2i ( pos_x + size_x/4, pos_y + size_y/4 ) ;
      glEnd      () ;
      break;

    case PUARROW_RIGHT :
      glBegin    ( GL_TRIANGLES ) ;
		glVertex2i ( pos_x - size_x/4, pos_y - size_y/4 ) ;
		glVertex2i ( pos_x + size_x/4, pos_y            ) ;
		glVertex2i ( pos_x - size_x/4, pos_y + size_y/4 ) ;
      glEnd      () ;
      break;

    case PUARROW_FASTLEFT :
      glBegin    ( GL_TRIANGLES ) ;
		glVertex2i ( pos_x + size_x/4, pos_y - size_y/4 ) ;
		glVertex2i ( pos_x           , pos_y            ) ;
		glVertex2i ( pos_x + size_x/4, pos_y + size_y/4 ) ;

		glVertex2i ( pos_x           , pos_y - size_y/4 ) ;
		glVertex2i ( pos_x - size_x/4, pos_y            ) ;
		glVertex2i ( pos_x           , pos_y + size_y/4 ) ;
      glEnd      () ;
      break;

    case PUARROW_FASTRIGHT :
      glBegin    ( GL_TRIANGLES ) ;
		glVertex2i ( pos_x - size_x/4, pos_y - size_y/4 ) ;
		glVertex2i ( pos_x           , pos_y            ) ;
		glVertex2i ( pos_x - size_x/4, pos_y + size_y/4 ) ;

		glVertex2i ( pos_x           , pos_y - size_y/4 ) ;
		glVertex2i ( pos_x + size_x/4, pos_y            ) ;
		glVertex2i ( pos_x           , pos_y + size_y/4 ) ;
      glEnd      () ;
      break;

    default : break ;
    }
  }

  draw_label ( dx, dy ) ;
}

