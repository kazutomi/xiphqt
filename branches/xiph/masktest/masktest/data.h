/*  Copyright 2000 - 2001 by Robert Voigt <robert.voigt@gmx.de>  */

#ifndef DATA_H
#define DATA_H

#include <qobject.h>
#include <qdatastream.h>
#include <qfile.h>
#include <qvaluelist.h>
#include "audio.h"

/*  this class saves the masking data and holds some values from the  */
/*  ATH tone test, from which 0 dB is calculated later */


class Data : public QObject
{
 public:
  Data(QString);
  ~Data();
  void save(float);
  void save(QString);
  float getZaverage();
  void addZ(float);
  QString getpath();

 private:
  QFile datafile;
  QDataStream datastream;

/*  holds some values from the ATH tone test */
/*  we don't need that many elements, but they can't be more than EHMER_MAX: */
  float Zs[EHMER_MAX];
  int counter;
  
};


#endif
