/*  Copyright 2000 - 2001 by Robert Voigt <robert.voigt@gmx.de>  */

#ifndef TESTWINDOW_H
#define TESTWINDOW_H

#include <qwidget.h>
#include <qlabel.h>
#include "data.h"
#include "audio.h"
#include "graph.h"

//this is the main class, it represents the application window 
//and does all the flow control

class Testwindow : public QWidget
{
  Q_OBJECT

 public:
  Testwindow(QWidget *parent=0, const char *name=0);
  ~Testwindow();
  void maskerloop();
  void save();
  Audio *audio;
  Data *data;

 signals:
  void disablebuttons(bool);
  void soundmade(bool);
  void testfinished();

 public slots:
  void decision_yes();
  void decision_no();

 private slots:
  void dosomething();
  void whatsnext();
  void changetext();

 private:
  void welcome();
  void enterdriver();
  void enterfile();
  void entersoundcard();
  void enterheadphones();
  void entermisc();
  void goodbye();
  //the flow control variables:
  int kindoftest; 
  int maskerfreq; //not really a frequency, just an index of a lookup array
  int maskeramp; //not really an amplitude, just an index of a lookup array
  int freq;
  float amp;
  float prev;
  float tolerance;
  Graph *graph;
  QLabel *text;
};


#endif
