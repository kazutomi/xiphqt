/*  Copyright 2000 - 2001 by Robert Voigt <robert.voigt@gmx.de>  */

#include "data.h"


Data::Data(QString path)
{
  datafile.setName(path);
  datafile.open(IO_ReadWrite);
  datastream.setDevice(&datafile);

  //is this necessary?
  for(int i = 0; i < EHMER_MAX; i++)
    Zs[i] = 0.;

  counter = 0;
}

Data::~Data()
{

}

void Data::save(float ampvalue)
{
//   this was for the case we want to store the data in arrays, could
//   be useful for saving and restoring sessions:
//    switch(kindoftest)
//      {
//      case 0 : athtone[freq] = ampvalue;
//      case 1 : athnoise[freq] = ampvalue;
//      case 2 : tonetone[maskerfreq][maskeramp][freq] = ampvalue;
//      case 3 : tonenoise[maskerfreq][maskeramp][freq] = ampvalue;
//      case 4 : noisetone[maskerfreq][maskeramp][freq] = ampvalue;
//      case 5 : noisenoise[maskerfreq][maskeramp][freq] = ampvalue;
//      default : break;
//      }

  datastream << ampvalue;
}

void Data::save(QString thestring)
{
//    saves the text information the user entered:
  datastream << thestring;
}

void Data::addZ(float z)
{
  Zs[counter] = z;
  counter++;
}

float Data::getZaverage()
{
  float average;

  for(int i = 0; i < counter; i ++)
    {
      average += Zs[counter];
    }

  return (average / (float) (counter));
}

QString Data::getpath()
{
  return datafile.name();
}
