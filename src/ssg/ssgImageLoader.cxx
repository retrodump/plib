
#include "ssgLocal.h"

/*
  Original source for BMP loader kindly
  donated by "Sean L. Palmer" <spalmer@pobox.com>
*/


/* Some magic constants in the file header. */

#define SGI_IMG_MAGIC           0x01DA
#define SGI_IMG_SWABBED_MAGIC   0xDA01   /* This is how it appears on a PC */
#define SGI_IMG_VERBATIM        0
#define SGI_IMG_RLE             1

static int            total_texels_loaded = 0 ;

static FILE          *curr_image_fd ;
static char           curr_image_fname [ 512 ] ;
static int            isSwapped ;
static unsigned char *rle_temp ;


struct BMPHeader
{
  unsigned short  FileType      ;
  unsigned int    FileSize      ;
  unsigned short  Reserved1     ;
  unsigned short  Reserved2     ;
  unsigned int    OffBits       ;
  unsigned int    Size          ;
  unsigned int    Width         ;
  unsigned int    Height        ;
  unsigned short  Planes        ;
  unsigned short  BitCount      ;
  unsigned int    Compression   ;
  unsigned int    SizeImage     ;
  unsigned int    XPelsPerMeter ;
  unsigned int    YPelsPerMeter ;
  unsigned int    ClrUsed       ;
  unsigned int    ClrImportant  ;
} ;


void ssgSGIHeader::makeConsistant ()
{
  /*
    Sanity checks - and a workaround for buggy RGB files generated by
    the MultiGen Paint program because it will sometimes get confused
    about the way to represent maps with more than one component.

    eg   Y > 1, Number of dimensions == 1
         Z > 1, Number of dimensions == 2
  */

  if ( ysize > 1 && dim < 2 ) dim = 2 ;
  if ( zsize > 1 && dim < 3 ) dim = 3 ;
  if ( dim < 1 ) ysize = 1 ;
  if ( dim < 2 ) zsize = 1 ;
  if ( dim > 3 ) dim   = 3 ;
  if ( zsize < 1 && ysize == 1 ) dim = 1 ;
  if ( zsize < 1 && ysize != 1 ) dim = 2 ;
  if ( zsize >= 1 ) dim = 3 ;

  /*
    A very few SGI image files have 2 bytes per component - this
    library cannot deal with those kinds of files. 
  */

  if ( bpp == 2 )
  {
    fprintf ( stderr,
       "ssgImageLoader: FATAL - Can't work with SGI images with %d bpp\n", bpp ) ;
    exit ( 1 ) ;
  }

  bpp = 1 ;
  min = 0 ;
  max = 255 ;
  magic = SGI_IMG_MAGIC ;
  colormap = 0 ;
}


static void swab_short ( unsigned short *x )
{
  if ( isSwapped )
    *x = (( *x >>  8 ) & 0x00FF ) | 
         (( *x <<  8 ) & 0xFF00 ) ;
}

static void swab_int ( unsigned int *x )
{
  if ( isSwapped )
    *x = (( *x >> 24 ) & 0x000000FF ) | 
         (( *x >>  8 ) & 0x0000FF00 ) | 
         (( *x <<  8 ) & 0x00FF0000 ) | 
         (( *x << 24 ) & 0xFF000000 ) ;
}

static void swab_int_array ( int *x, int leng )
{
  if ( ! isSwapped )
    return ;

  for ( int i = 0 ; i < leng ; i++ )
  {
    *x = (( *x >> 24 ) & 0x000000FF ) | 
         (( *x >>  8 ) & 0x0000FF00 ) | 
         (( *x <<  8 ) & 0x00FF0000 ) | 
         (( *x << 24 ) & 0xFF000000 ) ;
    x++ ;
  }
}


static unsigned char readByte ()
{
  unsigned char x ;
  fread ( & x, sizeof(unsigned char), 1, curr_image_fd ) ;
  return x ;
}

static unsigned short readShort ()
{
  unsigned short x ;
  fread ( & x, sizeof(unsigned short), 1, curr_image_fd ) ;
  swab_short ( & x ) ;
  return x ;
}

static unsigned int readInt ()
{
  unsigned int x ;
  fread ( & x, sizeof(unsigned int), 1, curr_image_fd ) ;
  swab_int ( & x ) ;
  return x ;
}


void ssgSGIHeader::getRow ( unsigned char *buf, int y, int z )
{
  if ( y >= ysize ) y = ysize - 1 ;
  if ( z >= zsize ) z = zsize - 1 ;

  fseek ( curr_image_fd, start [ z * ysize + y ], SEEK_SET ) ;

  if ( type == SGI_IMG_RLE )
  {
    unsigned char *tmpp = rle_temp ;
    unsigned char *bufp = buf ;

    fread ( rle_temp, 1, leng [ z * ysize + y ], curr_image_fd ) ;

    unsigned char pixel, count ;

    while ( TRUE )
    {
      pixel = *tmpp++ ;

      count = ( pixel & 0x7f ) ;

      if ( count == 0 )
	break ;

      if ( pixel & 0x80 )
      {
        while ( count-- )
	  *bufp++ = *tmpp++ ;
      }
      else
      {
        pixel = *tmpp++ ;

	while ( count-- )
          *bufp++ = pixel ;
      }
    }
  }
  else
    fread ( buf, 1, xsize, curr_image_fd ) ;
}


void ssgSGIHeader::getPlane ( unsigned char *buf, int z )
{
  if ( curr_image_fd == NULL )
    return ;

  if ( z >= zsize ) z = zsize - 1 ;

  for ( int y = 0 ; y < ysize ; y++ )
    getRow ( & buf [ y * xsize ], y, z ) ;
}



void ssgSGIHeader::getImage ( unsigned char *buf )
{
  if ( curr_image_fd == NULL )
    return ;

  for ( int y = 0 ; y < ysize ; y++ )
    for ( int z = 0 ; z < zsize ; z++ )
      getRow ( & buf [ ( z * ysize + y ) * xsize ], y, z ) ;
}


ssgSGIHeader::ssgSGIHeader ()
{
  dim   = 0 ;
  start = NULL ;
  leng  = NULL ;
  rle_temp = NULL ;
}


void ssgSGIHeader::readHeader ()
{
  isSwapped = FALSE ;

  magic = readShort () ;

  if ( magic != SGI_IMG_MAGIC && magic != SGI_IMG_SWABBED_MAGIC )
  {
    fprintf ( stderr, "%s: Unrecognised magic number 0x%04x\n",
                                         curr_image_fname, magic ) ;
    exit ( 1 ) ;
  }

  if ( magic == SGI_IMG_SWABBED_MAGIC )
  {
    isSwapped = TRUE ;
    swab_short ( & magic ) ;
  }

  type  = readByte  () ;
  bpp   = readByte  () ;
  dim   = readShort () ;

  /*
    This is a backstop test - if for some reason the magic number isn't swabbed, this
    test will still catch a swabbed file. Of course images with more than 256 dimensions
    are not catered for :-)
  */

  if ( dim > 255 )
  {
    fprintf ( stderr, "%s: Bad swabbing?!?\n", curr_image_fname ) ;
    isSwapped = ! isSwapped ;
    swab_short ( & dim ) ;
    magic = SGI_IMG_MAGIC ;
  }

  xsize = readShort () ;
  ysize = readShort () ;
  zsize = readShort () ;
  min   = readInt   () ;  
  max   = readInt   () ;  
                 readInt   () ;  /* Dummy field */

  int i ;

  for ( i = 0 ; i < 80 ; i++ )
    readByte () ;         /* Name field */

  colormap = readInt () ;

  for ( i = 0 ; i < 404 ; i++ )
    readByte () ;         /* Dummy field */

  makeConsistant () ;

  tablen = ysize * zsize ;
  start = new unsigned int [ tablen ] ;
  leng  = new int [ tablen ] ;
}


struct RGBA
{
  unsigned char r,g,b,a ;
} ;




void ssgImageLoader::make_mip_maps ( GLubyte *image, int xsize,
                                                 int ysize, int zsize )
{
  GLubyte *texels [ 20 ] ;   /* One element per level of MIPmap */

  for ( int l = 0 ; l < 20 ; l++ )
    texels [ 0 ] = NULL ;

  texels [ 0 ] = image ;

  int lev ;

  for ( lev = 0 ; (( xsize >> (lev+1) ) != 0 ||
                   ( ysize >> (lev+1) ) != 0 ) ; lev++ )
  {
    /* Suffix '1' is the higher level map, suffix '2' is the lower level. */

    int l1 = lev   ;
    int l2 = lev+1 ;
    int w1 = xsize >> l1 ;
    int h1 = ysize >> l1 ;
    int w2 = xsize >> l2 ;
    int h2 = ysize >> l2 ;

    if ( w1 <= 0 ) w1 = 1 ;
    if ( h1 <= 0 ) h1 = 1 ;
    if ( w2 <= 0 ) w2 = 1 ;
    if ( h2 <= 0 ) h2 = 1 ;

    texels [ l2 ] = new GLubyte [ w2 * h2 * zsize ] ;

    for ( int x2 = 0 ; x2 < w2 ; x2++ )
      for ( int y2 = 0 ; y2 < h2 ; y2++ )
        for ( int c = 0 ; c < zsize ; c++ )
        {
          int x1   = x2 + x2 ;
          int x1_1 = ( x1 + 1 ) % w1 ;
          int y1   = y2 + y2 ;
          int y1_1 = ( y1 + 1 ) % h1 ;

	  int t1 = texels [ l1 ] [ (y1   * w1 + x1  ) * zsize + c ] ;
	  int t2 = texels [ l1 ] [ (y1_1 * w1 + x1  ) * zsize + c ] ;
	  int t3 = texels [ l1 ] [ (y1   * w1 + x1_1) * zsize + c ] ;
	  int t4 = texels [ l1 ] [ (y1_1 * w1 + x1_1) * zsize + c ] ;

          texels [ l2 ] [ (y2 * w2 + x2) * zsize + c ] =
                                           ( t1 + t2 + t3 + t4 ) / 4 ;
        }
  }

  texels [ lev+1 ] = NULL ;

  if ( ! ((xsize & (xsize-1))==0) ||
       ! ((ysize & (ysize-1))==0) )
  {
    fprintf ( stderr, "%s: Map is not a power-of-two in size!\n", curr_image_fname ) ;
    exit ( 1 ) ;
  }

  glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 ) ;

  int map_level = 0 ;

#ifdef PROXY_TEXTURES_ARE_NOT_BROKEN
  int ww ;

  do
  {
    glTexImage2D  ( GL_PROXY_TEXTURE_2D,
                     map_level, zsize, xsize, ysize, FALSE /* Border */,
                            (zsize==1)?GL_LUMINANCE:
                            (zsize==2)?GL_LUMINANCE_ALPHA:
                            (zsize==3)?GL_RGB:
                                       GL_RGBA,
                            GL_UNSIGNED_BYTE, NULL ) ;

    glGetTexLevelParameteriv ( GL_PROXY_TEXTURE_2D, 0,GL_TEXTURE_WIDTH, &ww ) ;

    if ( ww == 0 )
    {
      delete texels [ 0 ] ;
      xsize >>= 1 ;
      ysize >>= 1 ;

      for ( int l = 0 ; texels [ l ] != NULL ; l++ )
	texels [ l ] = texels [ l+1 ] ;

      if ( xsize < 64 && ysize < 64 )
      {
        fprintf ( stderr,
                "SSG: OpenGL will not accept a downsized version of '%s' ?!?\n",
                curr_image_fname ) ;
        exit ( 1 ) ;
      }
    }
  } while ( ww == 0 ) ;
#endif

  for ( int i = 0 ; texels [ i ] != NULL ; i++ )
  {
    int w = xsize>>i ;
    int h = ysize>>i ;

    if ( w <= 0 ) w = 1 ;
    if ( h <= 0 ) h = 1 ;

    total_texels_loaded += w * h ;

    glTexImage2D  ( GL_TEXTURE_2D,
                     map_level, zsize, w, h, FALSE /* Border */,
                            (zsize==1)?GL_LUMINANCE:
                            (zsize==2)?GL_LUMINANCE_ALPHA:
                            (zsize==3)?GL_RGB:
                                       GL_RGBA,
                            GL_UNSIGNED_BYTE, (GLvoid *) texels[i] ) ;
    map_level++ ;
    delete texels [ i ] ;
  }
}


void ssgImageLoader::loadDummyTexture ()
{
  GLubyte *image = new GLubyte [ 4 * 3 ] ;

  /* Red and white chequerboard */

  image [ 0 ] = 255 ; image [ 1 ] =  0  ; image [ 2 ] =  0  ;
  image [ 3 ] = 255 ; image [ 4 ] = 255 ; image [ 5 ] = 255 ;
  image [ 6 ] = 255 ; image [ 7 ] = 255 ; image [ 8 ] = 255 ;
  image [ 9 ] = 255 ; image [ 10] =  0  ; image [ 11] =  0  ;

  make_mip_maps ( image, 2, 2, 3 ) ;
}


/*
  I'm sick of half the machines on the planet supporting
  strcasecmp and the other half stricmp - so here is my own
  offering:
*/

static int _ssgStrEqual ( char *s1, char *s2 )
{
  int l1 = (s1==NULL) ? 0 : strlen ( s1 ) ;
  int l2 = (s2==NULL) ? 0 : strlen ( s2 ) ;

  if ( l1 != l2 ) return FALSE ;

  for ( int i = 0 ; i < l1 ; i++ )
  {
    char c1 = s1[i] ;
    char c2 = s2[i] ;

    if ( c1 == c2 )
     continue ;

    if ( c1 >= 'a' && c1 <= 'z' )
      c1 = c1 - ('a'-'A') ;

    if ( c2 >= 'a' && c2 <= 'z' )
      c2 = c2 - ('a'-'A') ;

    if ( c1 != c2 )
     return FALSE ;
  }

  return TRUE ;
}


void ssgImageLoader::loadTexture ( char *fname )
{
  char *p = & fname [ strlen ( fname ) ] ;

  while ( p != fname && *p != '.' && *p != '/' && *p != '\\' )
    p-- ;

  if ( *p == '.' )
  {
    if ( _ssgStrEqual ( p, ".bmp"  ) ) { loadTextureBMP ( fname ) ; return ; }
    if ( _ssgStrEqual ( p, ".png"  ) ) { loadTexturePNG ( fname ) ; return ; }
    if ( _ssgStrEqual ( p, ".rgb"  ) ||
         _ssgStrEqual ( p, ".rgba" ) ||
         _ssgStrEqual ( p, ".int"  ) ||
         _ssgStrEqual ( p, ".inta" ) ||
         _ssgStrEqual ( p, ".bw"   ) ) { loadTextureSGI ( fname ) ; return ; }
  }

  fprintf ( stderr, "ssgImageLoader: '%s' - unrecognised file extension.\n",
                                                    fname ) ;
  loadDummyTexture () ;
}

void ssgImageLoader::loadTexturePNG ( char *fname )
{
  fprintf ( stderr,
        "ssgImageLoader: '%s' - PNG format images are not supported (yet)\n",
        fname ) ;
  loadDummyTexture () ;
}


void ssgImageLoader::loadTextureBMP ( char *fname )
{
  int w, h, bpp ;
  RGBA pal [ 256 ] ;

  BMPHeader bmphdr ;

  /* Open file & get size */

  strcpy ( curr_image_fname, fname ) ;

  if ( ( curr_image_fd = fopen ( curr_image_fname, "rb" ) ) == NULL )
  {
    perror ( "ssgImageLoader" ) ;
    fprintf ( stderr, "ssgImageLoader: Failed to open '%s' for reading.\n", curr_image_fname ) ;
    return ;
  }

  /*
    Load the BMP piecemeal to avoid struct packing issues
  */

  isSwapped = FALSE ;
  bmphdr.FileType = readShort () ;

  if ( bmphdr.FileType == ((int)'B' + ((int)'M'<<8)) )
    isSwapped = FALSE ;
  else
  if ( bmphdr.FileType == ((int)'M' + ((int)'B'<<8)) )
    isSwapped = TRUE  ;
  else
  {
    fprintf ( stderr, "%s: Unrecognised magic number 0x%04x\n",
                            curr_image_fname, bmphdr.FileType ) ;
    exit ( 1 ) ;
  }

  bmphdr.FileSize      = readInt   () ;
  bmphdr.Reserved1     = readShort () ;
  bmphdr.Reserved2     = readShort () ;
  bmphdr.OffBits       = readInt   () ;
  bmphdr.Size          = readInt   () ;
  bmphdr.Width         = readInt   () ;
  bmphdr.Height        = readInt   () ;
  bmphdr.Planes        = readShort () ;
  bmphdr.BitCount      = readShort () ;
  bmphdr.Compression   = readInt   () ;
  bmphdr.SizeImage     = readInt   () ;
  bmphdr.XPelsPerMeter = readInt   () ;
  bmphdr.YPelsPerMeter = readInt   () ;
  bmphdr.ClrUsed       = readInt   () ;
  bmphdr.ClrImportant  = readInt   () ;
 
  w   = bmphdr.Width  ;
  h   = bmphdr.Height ;
  bpp = bmphdr.Planes * bmphdr.BitCount ;

#ifdef PRINT_BMP_HEADER_DEBUG
  fprintf(stderr,"Filetype %04x, ",      bmphdr.FileType      ) ;
  fprintf(stderr,"Filesize %08x\n",      bmphdr.FileSize      ) ;
  fprintf(stderr,"R1 %04x, ",            bmphdr.Reserved1     ) ;
  fprintf(stderr,"R2 %04x\n",            bmphdr.Reserved2     ) ;
  fprintf(stderr,"Offbits %08x, ",       bmphdr.OffBits       ) ;
  fprintf(stderr,"Size %08x\n",          bmphdr.Size          ) ;
  fprintf(stderr,"Width %08x, ",         bmphdr.Width         ) ;
  fprintf(stderr,"Height %08x, ",        bmphdr.Height        ) ;
  fprintf(stderr,"Planes %04x\n",        bmphdr.Planes        ) ;
  fprintf(stderr,"Bitcount %04x, ",      bmphdr.BitCount      ) ;
  fprintf(stderr,"Compression %08x\n",   bmphdr.Compression   ) ;
  fprintf(stderr,"SizeImage %08x\n",     bmphdr.SizeImage     ) ;
  fprintf(stderr,"XPelsPerMeter %08x, ", bmphdr.XPelsPerMeter ) ;
  fprintf(stderr,"YPelsPerMeter %08x\n", bmphdr.YPelsPerMeter ) ;
  fprintf(stderr,"ClrUsed %08x, ",       bmphdr.ClrUsed       ) ;
  fprintf(stderr,"ClrImportant %08x\n",  bmphdr.ClrImportant  ) ;
#endif

  int isMonochrome = TRUE ;
  int isOpaque     = TRUE ;

  if ( bpp <= 8 )
  {
    for ( int i = 0 ; i < 256 ; i++ )
    {
      pal[i].b = readByte () ;
      pal[i].g = readByte () ;
      pal[i].r = readByte () ;
      pal[i].a = readByte () ;

      if ( pal[i].a != 255 ) isOpaque = FALSE ;

      if ( pal[i].r != pal[i].g ||
           pal[i].g != pal[i].b ) isMonochrome = FALSE ;
    }
  }

  fseek ( curr_image_fd, bmphdr.OffBits, SEEK_SET ) ;

  if ( bmphdr.SizeImage == 0 )
    bmphdr.SizeImage = w * h * (bpp / 8) ;

  GLubyte *data = new GLubyte [ bmphdr.SizeImage ] ;

  if ( fread ( data, 1, bmphdr.SizeImage, curr_image_fd ) != bmphdr.SizeImage )
  {
    fprintf ( stderr, "Premature EOF in '%s'\n", curr_image_fname ) ;
    return ;
  }

  fclose ( curr_image_fd ) ;

  GLubyte *image ;
  int z ;

  if ( bpp == 8 )
  {
    if ( isMonochrome )
      z = isOpaque ? 1 : 2 ;
    else
      z = isOpaque ? 3 : 4 ;

    image = new GLubyte [ w * h * z ] ;

    for ( int i = 0 ; i < w * h ; i++ )
      switch ( z )
      {
        case 1 : image [ i       ] = pal[data[i]].r ; break ;
        case 2 : image [ i*2     ] = pal[data[i]].r ;
                 image [ i*2 + 1 ] = pal[data[i]].a ; break ;
        case 3 : image [ i*3     ] = pal[data[i]].r ;
                 image [ i*3 + 1 ] = pal[data[i]].g ;
                 image [ i*3 + 2 ] = pal[data[i]].b ; break ;
        case 4 : image [ i*4     ] = pal[data[i]].r ;
                 image [ i*4 + 1 ] = pal[data[i]].g ;
                 image [ i*4 + 2 ] = pal[data[i]].b ;
                 image [ i*4 + 3 ] = pal[data[i]].a ; break ;
        default : break ;
      }

    delete data ;
  }
  else
  if ( bpp == 24 )
  {
    z = 3 ;
    image = data ;

    /* BGR --> RGB */

    for ( int i = 0 ; i < w * h ; i++ )
    {
      GLubyte tmp = image [ 3 * i ] ;
      image [ 3 * i ] = image [ 3 * i + 2 ];
      image [ 3 * i + 2 ] = tmp ;
    }
  }
  else
  if ( bpp == 32 )
  {
    z = 4 ;
    image = data ;

    /* BGRA --> RGBA */

    for ( int i = 0 ; i < w * h ; i++ )
    {
      GLubyte tmp = image [ 4 * i ] ;
      image [ 4 * i ] = image [ 4 * i + 2 ];
      image [ 4 * i + 2 ] = tmp ;
    }
  }
  else
  {
    fprintf ( stderr, "ssgImageLoader: Can't load %d bpp BMP textures.\n", bpp ) ;
    loadDummyTexture () ;
    return ;
  }

  make_mip_maps ( image, w, h, z ) ;
}


void ssgImageLoader::loadTextureSGI ( char *fname )
{
  ssgSGIHeader *sgihdr = new ssgSGIHeader () ;

  strcpy ( curr_image_fname, fname ) ;
  curr_image_fd = fopen ( curr_image_fname, "rb" ) ;

  if ( curr_image_fd == NULL )
  {
    perror ( "ssgImageLoader" ) ;
    fprintf ( stderr, "ssgImageLoader: Failed to open '%s' for reading.\n", curr_image_fname ) ;
    loadDummyTexture () ;
    return ;
  }

  sgihdr -> readHeader () ;

  if ( sgihdr -> type == SGI_IMG_RLE )
  {
    fread ( sgihdr->start, sizeof (unsigned int), sgihdr->tablen, curr_image_fd ) ;
    fread ( sgihdr->leng , sizeof (int), sgihdr->tablen, curr_image_fd ) ;
    swab_int_array ( (int *) sgihdr->start, sgihdr->tablen ) ;
    swab_int_array ( (int *) sgihdr->leng , sgihdr->tablen ) ;

    int maxlen = 0 ;

    for ( int i = 0 ; i < sgihdr->tablen ; i++ )
      if ( sgihdr->leng [ i ] > maxlen )
        maxlen = sgihdr->leng [ i ] ;

    rle_temp = new unsigned char [ maxlen ] ;
  }
  else
  {
    rle_temp = NULL ;

    for ( int i = 0 ; i < sgihdr->zsize ; i++ )
      for ( int j = 0 ; j < sgihdr->ysize ; j++ )
      {
        sgihdr->start [ i * sgihdr->ysize + j ] = sgihdr->xsize * ( i * sgihdr->ysize + j ) + 512 ;
        sgihdr->leng  [ i * sgihdr->ysize + j ] = sgihdr->xsize ;
      }
  }

  if ( sgihdr->zsize <= 0 || sgihdr->zsize > 4 )
  {
    fprintf ( stderr, "ssgImageLoader: '%s' is corrupted.\n", curr_image_fname ) ;
    exit ( 1 ) ;
  }

  GLubyte *image = new GLubyte [ sgihdr->xsize *
                                 sgihdr->ysize *
                                 sgihdr->zsize ] ;

  GLubyte *ptr = image ;

  unsigned char *rbuf =                     new unsigned char [ sgihdr->xsize ] ;
  unsigned char *gbuf = (sgihdr->zsize>1) ? new unsigned char [ sgihdr->xsize ] : (unsigned char *) NULL ;
  unsigned char *bbuf = (sgihdr->zsize>2) ? new unsigned char [ sgihdr->xsize ] : (unsigned char *) NULL ;
  unsigned char *abuf = (sgihdr->zsize>3) ? new unsigned char [ sgihdr->xsize ] : (unsigned char *) NULL ;

  for ( int y = 0 ; y < sgihdr->ysize ; y++ )
  {
    int x ;

    switch ( sgihdr->zsize )
    {
      case 1 :
	sgihdr->getRow ( rbuf, y, 0 ) ;

	for ( x = 0 ; x < sgihdr->xsize ; x++ )
	  *ptr++ = rbuf [ x ] ;

	break ;

      case 2 :
	sgihdr->getRow ( rbuf, y, 0 ) ;
	sgihdr->getRow ( gbuf, y, 1 ) ;

	for ( x = 0 ; x < sgihdr->xsize ; x++ )
	{
	  *ptr++ = rbuf [ x ] ;
	  *ptr++ = gbuf [ x ] ;
	}
	break ;

      case 3 :
        sgihdr->getRow ( rbuf, y, 0 ) ;
	sgihdr->getRow ( gbuf, y, 1 ) ;
	sgihdr->getRow ( bbuf, y, 2 ) ;

	for ( x = 0 ; x < sgihdr->xsize ; x++ )
	{
	  *ptr++ = rbuf [ x ] ;
	  *ptr++ = gbuf [ x ] ;
	  *ptr++ = bbuf [ x ] ;
	}
	break ;

      case 4 :
        sgihdr->getRow ( rbuf, y, 0 ) ;
	sgihdr->getRow ( gbuf, y, 1 ) ;
	sgihdr->getRow ( bbuf, y, 2 ) ;
	sgihdr->getRow ( abuf, y, 3 ) ;

	for ( x = 0 ; x < sgihdr->xsize ; x++ )
	{
	  *ptr++ = rbuf [ x ] ;
	  *ptr++ = gbuf [ x ] ;
	  *ptr++ = bbuf [ x ] ;
	  *ptr++ = abuf [ x ] ;
	}
	break ;
    }
  }

  fclose ( curr_image_fd ) ;

  delete rbuf   ;
  delete gbuf   ;
  delete bbuf   ;
  delete abuf   ;

  make_mip_maps ( image, sgihdr->xsize, sgihdr->ysize, sgihdr->zsize ) ;

  delete sgihdr ;
}


int ssgGetNumTexelsLoaded ()
{
  return total_texels_loaded ;
}


