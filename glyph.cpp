
/* glyph.cpp
 *
 * part of kdvi, a dvi-previewer for the KDE desktop environement
 *
 * written by Stefan Kebekus, originally based on code by Paul Vojta
 * and a large number of co-authors */

#include "dviwin.h"
#include "glyph.h"
#include "oconfig.h"

#include <stdlib.h>

#include <qpainter.h>
#include <qbitmap.h> 
#include <qimage.h> 

glyph::~glyph()
{
  if (bitmap.bits != NULL)
    free(bitmap.bits);
  clearShrunkCharacter();
}

void glyph::clearShrunkCharacter()
{
  if (SmallChar != NULL) {
    delete SmallChar;
    SmallChar = NULL;
  }
}

/** This method returns the SmallChar of the glyph-class, if it
 * exists. If not, it is generated by shrinking the bitmap according
 * to the shrink_factor.  */

QPixmap glyph::shrunkCharacter()
{
  if (SmallChar == NULL) {
    // Rescaling a character is an art that requires some
    // explanation...
    //
    // If we would just divide the size of the character and the
    // coordinates by the shrink factor, then the result would look
    // quite ugly: due to the ineviatable rounding errors in the
    // integer arithmetic, the characters would be displaced by up to
    // a point. That doesn't sound much, but on low-resolution
    // devices, such as a notebook screen, the effect would be a
    // "dancing line" of characters, which looks really bad.
    //
    // The cure is the following procedure:
    //
    // (a) scale the hot point 
    //
    // (b) fit the unshrunken bitmap into a bitmap which is even
    // bigger. Use this to produce extra empty rows and columns at the
    // borders. The proper choice of the border size will ensure that
    // the hot point will fall exactly onto the coordinates which we
    // calculated previously.

    // Here the cheating starts ... on the screen many fonts look very
    // light. We improve on the looks by lowering the shrink factor
    // just when shrinking the characters. The position of the chars
    // on the screen will not be affected, the chars are just slightly
    // larger.
    float sf = shrink_factor * 0.9;

    // Calculate the coordinates of the hot point in the shrunken
    // bitmap
    x2 = (int)(x/sf);
    y2 = (int)(y/sf);

    // Calculate the size of the target bitmap for the
    int shrunk_width  = x2 + (int)((bitmap.w-x) / sf + 0.5) + 1;
    int shrunk_height = y2 + (int)((bitmap.h-y) / sf + 0.5) + 1;

    // Now calculate the size of the white border. This is some sort
    // of black magic. Don't modify unless you know what you are doing.
    int pre_rows = (int)((1.0 + y2)*sf + 0.5) - y - 1;
    if (pre_rows < 0)
      pre_rows = 0;
    int post_rows = (int)(shrunk_height*sf + 0.5) - bitmap.h;
    if (post_rows < 0)
      post_rows = 0;

    int pre_cols = (int)((1.0 + x2)*sf + 0.5) - x - 1;
    if (pre_cols < 0)
      pre_cols = 0;
    int post_cols = (int)(shrunk_width*sf + 0.5) - bitmap.w;
    if (post_cols < 0)
      post_cols = 0;

    // Now shrinking may begin. Produce a QBitmap with the unshrunk
    // character.
    QBitmap bm(bitmap.bytes_wide*8, (int)bitmap.h, (const uchar *)(bitmap.bits) ,TRUE);

    // ... turn it into a Pixmap (highly inefficient, please improve)
    SmallChar = new QPixmap(bitmap.w+pre_cols+post_cols, bitmap.h+pre_rows+post_rows);
    QPainter paint(SmallChar);
    paint.setBackgroundColor(Qt::white);
    paint.setPen( Qt::black );
    paint.fillRect(0,0,bitmap.w+pre_cols+post_cols, bitmap.h+pre_rows+post_rows, Qt::white);
    paint.drawPixmap(pre_cols, pre_rows, bm);
    paint.end();
    
    // Generate an Image and shrink it to the proper size. By the
    // documentation of smoothScale, the resulting Image will be
    // 8-bit.
    QImage im = SmallChar->convertToImage().smoothScale(shrunk_width, shrunk_height);
    // Generate the alpha-channel. This again is highly inefficient.
    // Would anybody please produce a faster routine?
    QImage im32 = im.convertDepth(32);
    im32.setAlphaBuffer(TRUE);
    for(int y=0; y<im.height(); y++) {
      QRgb *imag_scanline = (QRgb *)im32.scanLine(y);
      for(int x=0; x<im.width(); x++) {
	// Make White => Transparent
	if ((0x00ffffff & *imag_scanline) == 0x00ffffff)
	  *imag_scanline &= 0x00ffffff;
	else
	  *imag_scanline |= 0xff000000;
	imag_scanline++; // Disgusting pointer arithmetic. Should be forbidden.
      }
    }
    SmallChar->convertFromImage(im32,0);
    SmallChar->setOptimization(QPixmap::BestOptim);
  }
  return *SmallChar; 
}
