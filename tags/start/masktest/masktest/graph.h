/*  Copyright 2000 - 2001 by Robert Voigt <robert.voigt@gmx.de>  */

#ifndef GRAPH_H
#define GRAPH_H

#include <qwidget.h>
#include <qpainter.h>
#include "data.h"

/*  this class holds the coordinate system and paints the curves... */


class Graph : public QWidget
{
  Q_OBJECT

 public:
  Graph(QWidget *parent=0, const char *name=0);
  ~Graph();
  void resetpoints();
  void setmaskerdraw();
  bool finished;

 public slots:
  void addpoint(int, float);
  void newrow(int, int);

 protected:
  void  paintEvent( QPaintEvent * );

 private:
  float points[EHMER_MAX];
  int maskerfreqindex;
  int maskerampindex;
  bool maskerdraw;
  void paintgrid(QPainter *);
  void paintmasker(QPainter *);
  void paintbullets(QPainter *);

};


#endif
