/*
     P-GUIDE - PUI-based Graphical User Interface Designer
     Copyright (C) 2002  John F. Fay

     This program is free software; you can redistribute it and/or
     modify it under the terms of the GNU General Public License as
     published by the Free Software Foundation; either version 2 of
     the License, or (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

// Data Structure Definition

#ifndef WIDGET_LIST_H
#define WIDGET_LIST_H

#include <plib/pu.h>

// PUI Widget List for Main Window

struct WidgetList
{
  puObject *obj ;
  char *object_type_name ;
  int object_type ;
  char *label_text ;
  char *legend_text ;
  short callbacks ;
  char object_name [ PUSTRING_MAX ] ;
  bool visible ;
  int layer ;  // GUI layer:  0 - in back, positive nubmers - greater in front of lesser
  WidgetList *next ;
} ;


#endif  // WIDGET_LIST_H
