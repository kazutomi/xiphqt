/*  Copyright 2000 - 2001 by Robert Voigt <robert.voigt@gmx.de>  */

#include <math.h>
#include "graph.h"

#define LOFREQ 62.5
#define HIFREQ 16000.
#define XSTRETCH 60.
#define YSTRETCH -2.
#define XADD 50.
#define YADD 230.
#define YEND 100.


Graph::Graph( QWidget *parent, const char *name )
        : QWidget( parent, name )
{
  setPalette( QPalette( QColor( 250, 250, 200) ) );
  //don't draw the masker until told to do so after the ATH tests:
  maskerdraw = FALSE;
  newrow(0, 0);
}

Graph::~Graph()
{
}

void Graph::addpoint(int freq, float amp)
{
  points[freq] = amp;
  update();
}

void Graph::newrow(int mfi, int mai)
{
  resetpoints();
  maskerfreqindex = mfi;
  maskerampindex = mai;
  update();
}

void Graph::resetpoints()
{
  for(int i = 0; i < EHMER_MAX; i++)
    {
      points[i] = 0;
    }
}

void Graph::setmaskerdraw()
{
  //now paint the masker:
  maskerdraw = TRUE;
}

//  Qt calls this method whenever the widget needs to be repainted:
void Graph::paintEvent(QPaintEvent *)
{
  QPainter painter(this);

  paintgrid(&painter);

  if(maskerdraw == TRUE)
    paintmasker(&painter);

  paintbullets(&painter);
}

void Graph::paintgrid(QPainter *painter)
{
  QString caption;

  painter->setPen(blue);

  // draw the horizontal lines and the dB scale
  int num;
  const int xend = (int) rint((log(HIFREQ / LOFREQ) / log(2.)) * XSTRETCH + XADD);
  for(num = 0; num <= YEND; num += 20)
    {
      painter->drawLine((int) (0. + XADD), (int) (((float) (num)) * YSTRETCH + YADD),
			xend    , (int) (((float) (num)) * YSTRETCH + YADD));

      caption.setNum(num);
      painter->drawText((int) (-30. + XADD), (int) (((float) (num)) * YSTRETCH + YADD), caption);
    }

  // draw the vertical lines and the frequency scale
  float freq = LOFREQ;
  while(freq <= HIFREQ)
    {
      painter->drawLine( (int) rint((log(freq / LOFREQ) / log(2)) * XSTRETCH + XADD), (int) (0.              + YADD),
			 (int) rint((log(freq / LOFREQ) / log(2)) * XSTRETCH + XADD), (int) (YEND * YSTRETCH + YADD));

      caption.setNum(freq);
      painter->drawText( (int) rint((log(freq / LOFREQ) / log(2)) * XSTRETCH + XADD), (int) (-10. * YSTRETCH + YADD), caption);
      freq *= 2;
    }
}

void  Graph::paintmasker(QPainter *painter)
{
  painter->setPen(green);

  int xpos = (int) rint((log(maskerfreqarray[maskerfreqindex] / LOFREQ) / log(2)) * XSTRETCH + XADD);

  painter->drawLine(xpos -1, (int) (0. + YADD), xpos -1, (int) rint(maskeramparray[maskerampindex] * YSTRETCH + YADD));
  painter->drawLine(xpos   , (int) (0. + YADD), xpos   , (int) rint(maskeramparray[maskerampindex] * YSTRETCH + YADD));
  painter->drawLine(xpos +1, (int) (0. + YADD), xpos +1, (int) rint(maskeramparray[maskerampindex] * YSTRETCH + YADD));
}

void Graph::paintbullets(QPainter *painter)
{
  painter->setPen(NoPen);
  painter->setBrush(black);

  for(int i = 0; i < EHMER_MAX; i++)
    {
      if(points[i] > 0)
	{
	  painter->drawEllipse( (int) rint((log(freqarray[i] / LOFREQ) / log(2)) * XSTRETCH + XADD -2),
	                        (int) rint(points[i] * YSTRETCH + YADD -2),
			        5, 5);
	}
    }
}

