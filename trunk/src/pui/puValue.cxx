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
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

     For further information visit http://plib.sourceforge.net

     $Id$
*/


#include "puLocal.h"

void puValue::re_eval ( void )
{
  if ( res_integer != NULL )
    setValue ( *res_integer )  ;
  else if ( res_floater != NULL )
    setValue ( *res_floater ) ;
  else if ( res_string  != NULL )
    setValue ( res_string ) ;
}

void puValue::update_res ( void ) const
{
  if ( res_integer != NULL )
    *res_integer = integer ;
  else if ( res_floater != NULL )
    *res_floater = floater ;
  else if ( res_string  != NULL )
  {
    /* Work around ANSI strncpy's null-fill behaviour */

    res_string[0] = '\0' ;
    strncat ( res_string, string, res_string_sz-1 ) ;
  }
}

void puValue::copy_stringval ( const char *str )
{
  int str_len = strlen ( str ) ;
  int new_size = string_size ;

  while ( new_size < str_len + 1 )
  /* While our array is too small, double it. */
    new_size += new_size ;

  while ( ( new_size > 4 * str_len + 1 ) && ( new_size > PUSTRING_INITIAL ) )
  /* While our array is too big, halve it */
    new_size /= 2 ;

  if ( new_size != string_size )
  /* If the array size has changed, allocate a new array */
  {
    delete string ;
    string = new char [ new_size ] ;
    string_size = new_size ;
  }

  memcpy ( string, str, str_len + 1 ) ;
}


const char *puValue::getTypeString ( void ) const
{
  int i = getType () ;

  if ( i & PUCLASS_SELECTBOX    ) return "puSelectBox" ;
  if ( i & PUCLASS_COMBOBOX     ) return "puComboBox" ;
  if ( i & PUCLASS_LARGEINPUT   ) return "puLargeInput" ;
  if ( i & PUCLASS_VERTMENU     ) return "puVerticalMenu" ;
  if ( i & PUCLASS_TRISLIDER    ) return "puTriSlider" ;
  if ( i & PUCLASS_BISLIDER     ) return "puBiSlider" ;
  if ( i & PUCLASS_FILESELECTOR ) return "puFileSelector" ;
  if ( i & PUCLASS_DIAL         ) return "puDial" ;
  if ( i & PUCLASS_LISTBOX      ) return "puListBox" ;
  if ( i & PUCLASS_ARROW        ) return "puArrowButton" ;
  if ( i & PUCLASS_DIALOGBOX    ) return "puDialogBox" ;
  if ( i & PUCLASS_SLIDER       ) return "puSlider" ;
  if ( i & PUCLASS_BUTTONBOX    ) return "puButtonBox" ;
  if ( i & PUCLASS_INPUT        ) return "puInput" ;
  if ( i & PUCLASS_MENUBAR      ) return "puMenuBar" ;
  if ( i & PUCLASS_POPUPMENU    ) return "puPopupMenu" ;
  if ( i & PUCLASS_POPUP        ) return "puPopup" ;
  if ( i & PUCLASS_ONESHOT      ) return "puOneShot" ;
  if ( i & PUCLASS_BUTTON       ) return "puButton" ;
  if ( i & PUCLASS_TEXT         ) return "puText" ;
  if ( i & PUCLASS_FRAME        ) return "puFrame" ;
  if ( i & PUCLASS_INTERFACE    ) return "puInterface" ;
  if ( i & PUCLASS_GROUP        ) return "puGroup" ;
  if ( i & PUCLASS_OBJECT       ) return "puObject" ;
  if ( i & PUCLASS_VALUE        ) return "puValue" ;

  return "Unknown Object type." ;
}

