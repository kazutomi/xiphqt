
#include <stdio.h>
#include <stdlib.h>
//#include <qapplication.h>
#include <qfile.h>
#include <qdatastream.h>
#include <qstring.h>


int main()
{
  QString path = getenv("HOME");
  path += "/maskingdata";

  QFile datafile(path);
  datafile.open(IO_ReadOnly);

  QDataStream datastream(&datafile);

  //currently the QStrings at the beginning of the file
  //are just being read, not written to stdout
  QString str;
  for(int i = 0; i < 3; i++)
    datastream >> str;


  float val;
  int counter = 0;

  while( ! datastream.atEnd() )
    {
      counter++;
      datastream >> val;
      printf("%d:  %f \n", counter, val);
    }

  return 0;
}
