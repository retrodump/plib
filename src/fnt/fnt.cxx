
#include "fntLocal.h"

fntFont:: fntFont () {}
fntFont::~fntFont () {}

int fntTexFont::load ( const char *fname, GLenum mag, GLenum min )
{
  const char *p ;

  for ( p = & fname [ strlen ( fname ) -1 ] ;
        p != fname && *p != '.' && *p != '/' ; p-- )
    /* Do nothing */ ;

  if ( strcmp ( p, ".txf" ) == 0 ) {
    return loadTXF ( fname, mag, min ) ;
  }
  else
  {
    fprintf ( stderr, "fnt::load: Error - Unrecognised file format for '%s'\n",
                                        fname ) ;
    return FNT_FALSE ;
  }
}



float fntTexFont::low_putch ( sgVec3 curpos, float pointsize,
                               float italic, char c )
{
  unsigned int cc = (unsigned char) c ;

  /* Auto case-convert if character is absent from font. */

  if ( ! exists [ cc ] )
  {
    if ( cc >= 'A' && cc <= 'Z' )
      cc = cc - 'A' + 'a' ;
    else
    if ( cc >= 'a' && cc <= 'z' )
      cc = cc - 'a' + 'A' ;

    if ( cc == ' ' )
    {
      curpos [ 0 ] += pointsize / 2.0f ;
      return pointsize / 2.0f ;
    }
  }

  /*
    We might want to consider making some absent characters from
    others (if they exist): lowercase 'l' could be made into digit '1'
    or letter 'O' into digit '0'...or vice versa. We could also
    make 'b', 'd', 'p' and 'q' by mirror-imaging - this would
    save a little more texture memory in some fonts.
  */

  if ( ! exists [ cc ] )
    return 0.0f ;

  glBegin ( GL_TRIANGLE_STRIP ) ;
    glTexCoord2f ( t_left [cc], t_bot[cc] ) ;
    glVertex3f   ( curpos[0] +          v_left [cc] * pointsize,
		   curpos[1] +          v_bot  [cc] * pointsize,
		   curpos[2] ) ;

    glTexCoord2f ( t_left [cc], t_top[cc] ) ;
    glVertex3f   ( curpos[0] + italic + v_left [cc] * pointsize,
		   curpos[1] +          v_top  [cc] * pointsize,
		   curpos[2] ) ;

    glTexCoord2f ( t_right[cc], t_bot[cc] ) ;
    glVertex3f   ( curpos[0] +          v_right[cc] * pointsize,
		   curpos[1] +          v_bot  [cc] * pointsize,
		   curpos[2] ) ;

    glTexCoord2f ( t_right[cc], t_top[cc] ) ;
    glVertex3f   ( curpos[0] + italic + v_right[cc] * pointsize,
		   curpos[1] +          v_top  [cc] * pointsize,
		   curpos[2] ) ;
  glEnd () ;

  float ww = ( gap + ( fixed_pitch ? width : v_right[cc] ) ) * pointsize ;
  curpos[0] += ww ;
  return ww ;
}



void fntTexFont::setGlyph ( char c,
		float tex_left, float tex_right,
		float tex_bot , float tex_top  ,
		float vtx_left, float vtx_right,
		float vtx_bot , float vtx_top  )
{
  unsigned int cc = (unsigned char) c ;

  exists[cc] = FNT_TRUE ;

  t_left[cc] = tex_left ; t_right[cc] = tex_right ;
  t_bot [cc] = tex_bot  ; t_top  [cc] = tex_top   ;

  v_left[cc] = vtx_left ; v_right[cc] = vtx_right ;
  v_bot [cc] = vtx_bot  ; v_top  [cc] = vtx_top   ;
}


int fntTexFont::getGlyph ( char c,
		float *tex_left, float *tex_right,
		float *tex_bot , float *tex_top  ,
		float *vtx_left, float *vtx_right,
		float *vtx_bot , float *vtx_top  )
{
  unsigned int cc = (unsigned char) c ;

  if ( ! exists[cc] ) return FNT_FALSE ;

  if ( tex_left  != NULL ) *tex_left  = t_left [cc] ;
  if ( tex_right != NULL ) *tex_right = t_right[cc] ;
  if ( tex_bot   != NULL ) *tex_bot   = t_bot  [cc] ;
  if ( tex_top   != NULL ) *tex_top   = t_top  [cc] ;

  if ( vtx_left  != NULL ) *vtx_left  = v_left [cc] ;
  if ( vtx_right != NULL ) *vtx_right = v_right[cc] ;
  if ( vtx_bot   != NULL ) *vtx_bot   = v_bot  [cc] ;
  if ( vtx_top   != NULL ) *vtx_top   = v_top  [cc] ;

  return FNT_TRUE ;
}


void fntTexFont::getBBox ( const char *s,
                           float pointsize, float italic,
                           float *left, float *right,
                           float *bot , float *top  )
{
  float h_pos = 0.0f ;
  float v_pos = 0.0f ;
  float l, r, b, t ;
  float max_r, max_b ;

  l = r = max_r = b = max_b = t = 0.0f ;

  while ( *s != '\0' )
  {
    if ( *s == '\n' )
    {
      r = h_pos = 0.0f ;
      v_pos -= 1.333f ;
      s++ ;
      continue ;
    }

    unsigned int cc = (unsigned char) *(s++) ;

    if ( ! exists [ cc ] )
    {
      if ( cc >= 'A' && cc <= 'Z' )
        cc = cc - 'A' + 'a' ;
      else
      if ( cc >= 'a' && cc <= 'z' )
        cc = cc - 'a' + 'A' ;
  
      if ( cc == ' ' )
      {
        r += 0.5f ;
        h_pos += 0.5f ;

        if ( max_r < r ) max_r = r ;

        continue ;
      }
    }

    if ( ! exists [ cc ] )
      continue ;

    if ( italic >= 0 )
    {
      if ( l >       h_pos + v_left [cc]        ) l =       h_pos + v_left [cc]          ;
      if ( r < gap + h_pos + v_right[cc]+italic ) r = gap + h_pos + v_right[cc] + italic ;
    }
    else
    {
      if ( l >       h_pos + v_left [cc]+italic ) l =       h_pos + v_left [cc] + italic ;
      if ( r < gap + h_pos + v_right[cc]+italic ) r = gap + h_pos + v_right[cc]          ;
    }

    if ( b > v_pos + v_bot [cc] ) b = v_pos + v_bot [cc] ;
    if ( t < v_pos + v_top [cc] ) t = v_pos + v_top [cc] ;

    if ( max_b > b ) max_b = b ;
    if ( max_r < r ) max_r = r ;

    h_pos += gap + ( fixed_pitch ? width : v_right[cc] ) ;
  }

  if ( left  != NULL ) *left  = l * pointsize ;
  if ( right != NULL ) *right = r * pointsize ;
  if ( top   != NULL ) *top   = t * pointsize ;
  if ( bot   != NULL ) *bot   = b * pointsize ;
}


void fntTexFont::puts ( sgVec3 curpos, float pointsize, float italic, const char *s )
{
  SGfloat origx = curpos[0] ;
	
  if ( ! bound )
    bind_texture () ;

  while ( *s != '\0' )
  {
    if (*s == '\n')
    {
      curpos[0]  = origx ;
      curpos[1] -= pointsize * 1.333f ;
    }
    else
      low_putch ( curpos, pointsize, italic, *s ) ;

    s++ ;
  }
}


