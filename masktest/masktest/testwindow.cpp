/*  Copyright 2000 - 2001 by Robert Voigt <robert.voigt@gmx.de>  */

#include <qapplication.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qhbuttongroup.h>
#include <qvbox.h>
#include <qmessagebox.h>
#include <qinputdialog.h>
#include <ao/ao.h>
#include <math.h>
#include "testwindow.h"

#define INITIAL0dB .00001


Testwindow::Testwindow( QWidget *parent, const char *name )
        : QWidget( parent, name )
{  
  // some dialog boxes first:
  welcome();
  enterdriver();
  enterfile();
  entersoundcard();
  enterheadphones();
  entermisc();

  // here starts the setup of the main widget:
  setMinimumSize(600, 400);

  QGridLayout *grid = new QGridLayout( this, 3, 1, 10);

  // the coordinate system:
  QVBox *graphbox = new QVBox( this, "graphframe" );
  graphbox->setFrameStyle( QFrame::WinPanel | QFrame::Sunken );
  graph = new Graph(graphbox, "graph");
 
  // the user guide text:
  text = new QLabel("Press Start to begin.", this, "text");
  text->setAlignment(AlignHCenter | AlignVCenter);

  // the buttons:
  QHButtonGroup *bottombox = new QHButtonGroup(this);

  QPushButton *quit = new QPushButton( "&Quit", bottombox, "quit" );
  quit->setMaximumWidth(100);
  connect( quit, SIGNAL(clicked()), qApp, SLOT(quit()) );

  QPushButton *start = new QPushButton( "&Start", bottombox, "start" );
  start->setMaximumWidth(100);
  connect( start, SIGNAL(clicked()), this, SLOT(dosomething()));

  QPushButton *yes = new QPushButton( "&Yes", bottombox, "yes" );
  yes->setMaximumWidth(100);
  connect( yes, SIGNAL(clicked()), this, SLOT(decision_yes()) );
  yes->setDisabled(TRUE);

  QPushButton *no = new QPushButton( "&No", bottombox, "no" );
  no->setMaximumWidth(100);
  connect( no, SIGNAL(clicked()), this, SLOT(decision_no()) );
  no->setDisabled(TRUE);

  // assemble the window:
  grid->addWidget(graphbox, 0, 0);
  grid->addWidget(text, 1, 0);
  grid->addWidget(bottombox, 2, 0);

  grid->setRowStretch(0, 1);
  grid->addRowSpacing(1, 20);
  grid->addRowSpacing(2, 20);

  // a bunch of connections:
  connect(this, SIGNAL(disablebuttons(bool)), yes, SLOT(setDisabled(bool)));
  connect(this, SIGNAL(disablebuttons(bool)), no, SLOT(setDisabled(bool)));
  connect(this, SIGNAL(soundmade(bool)), yes, SLOT(setEnabled(bool)));
  connect(this, SIGNAL(soundmade(bool)), no, SLOT(setEnabled(bool)));
  connect(this, SIGNAL(soundmade(bool)), this, SLOT(changetext()));
  connect(this, SIGNAL(testfinished()), qApp, SLOT(quit()) );

  // some initializations:
  audio->setzerodB(INITIAL0dB); // a 0 dB value used for the ATH tests
  kindoftest = 0; // 0 means start with ATH tone
  freq = 0; 
  //set amp to half of the dynamic range we can use:
  amp = .5 * 20 * log10(1 / audio->getzerodB());
  maskerfreq = 0;
  maskeramp = 0;
  prev = 0.0;
 //the remaining error in dB, determines the number of iterations:
  tolerance = 2.;

  ao_initialize();;//is it ok to do it here?

}
    
Testwindow::~Testwindow()
{
  ao_shutdown();//is it ok to do it here?
  delete audio;
  delete data;
}

void Testwindow::welcome()
{
  QMessageBox::information(this, "mask",
			   "Welcome to the Ogg Vorbis masking test.\n"
			   "Before you start, please answer the following questions.");
}

void Testwindow::enterdriver()
{ 
  bool ok = FALSE;
  QStringList lst;
  lst << "OSS" << "Alsa";
  QString driver = QInputDialog::getItem("mask", "Please select the sounddriver you would like to be used during the test.",
					 lst, 0, TRUE, &ok, this );
  if ( ok ) // user selected an item and pressed ok
  audio = new Audio("OSS"); //(driver);
  else // user pressed cancel
    QMessageBox::information(this, "mask",
			     "You can't take the test without answering this question.\n");
}

void Testwindow::enterfile()
{
  bool ok = FALSE;
  QString filepath = getenv("HOME");
  filepath += "/maskingdata";
  filepath = QInputDialog::getText("mask", "Please enter the path to the file the collected data will be stored in.\n" 
				   "Note: You must have read and write access to that directory.", // ? for ppl new to Unix
				   filepath, &ok, this);
  if ( ok )
    data = new Data(filepath);
  else
    QMessageBox::information(this, "mask",
			     "You can't take the test without answering this question.\n");
}

void Testwindow::entersoundcard()
{
  bool ok = FALSE;
  QString soundcard = QInputDialog::getText("mask", "Please enter the make and model of your soundcard.\n" 
					    "This information is not required, but helpful.",
					    QString::null, &ok, this);
  if ( ok )
    data->save(soundcard);
  else
    data->save("no soundcard");
}

void Testwindow::enterheadphones()
{
  bool ok = FALSE;
  QString headphones = QInputDialog::getText("mask", "Please enter the make and model of your headphones.\n" 
					    "This information is not required, but helpful.",
					    QString::null, &ok, this);
  if ( ok )
    data->save(headphones);
  else
    data->save("no headphones");
}

void Testwindow::entermisc()
{
  bool ok = FALSE;
  QString misc = QInputDialog::getText("mask", "Would you like to tell the Ogg Vorbis developers \n"
				       "anything else about your equipment or hearing, perhaps your age?\n" 
				       "Please enter the text below.",
				       QString::null, &ok, this);
  if ( ok )
    data->save(misc);
  else
    data->save("no misc");
}

void Testwindow::dosomething()
{
  if(freq == EHMER_MAX)
    {
      text->setText("Next test.");
      //this doesn't belong here, it doesn't work properly this way
      graph->newrow(maskerfreq, maskeramp);
      if(kindoftest > 0)
	{
	  audio->setzerodB(pow(10., (data->getZaverage() / 20 + log10(INITIAL0dB))));
	  graph->setmaskerdraw();
	}
      whatsnext();
    }
  else
    {
      if(fabs(amp - prev) > tolerance)
	{    
	  audio->sound(kindoftest, maskerfreq, maskeramp, freq, amp);
	  emit(soundmade(TRUE));
	}
      else
	{
	  data->save(amp);
	  if(kindoftest == 0 && (freqarray[freq] >= 500. || freqarray[freq] <= 4000.))
	    data->addZ(amp);
	  graph->addpoint(freq, amp);
	  text->setNum(amp);
	  whatsnext();
	}
    }
}

//  The previous slot and the following method must be replaced
//  by something real. They are just here because I couln't
//  think of a better way to do it.

void Testwindow::whatsnext()
{
  freq++;
  // set amp to half of the dynamic range we can use
  amp = .5 * 20 * log10(1. / audio->getzerodB());
  prev = 0.;
  if(freq > EHMER_MAX)
    {
      freq = 0;
      if(kindoftest < 2)
	{
	  kindoftest++;
	}
      else
	{
	  maskeramp++;
	  if(maskeramp >= P_LEVELS)
	    {
	      maskeramp = 0;
	      maskerfreq++;
	      if(maskerfreq >= P_BANDS)
		{
		  maskerfreq = 0;
		  kindoftest++;
		  if(kindoftest >= NUMTESTS)
		    {
		      goodbye();
		    }
		}
	    }
	}
    }
}

void Testwindow::decision_yes()
{
  float saveamp = amp;
  amp -= (.5 * fabs(amp - prev));
  prev = saveamp;
  emit(disablebuttons(TRUE));
  text->clear();
}

void Testwindow::decision_no()
{
  float saveamp = amp;
  amp += (.5 * fabs(amp - prev));
  prev = saveamp;
  emit(disablebuttons(TRUE));
  text->clear();
}

void Testwindow::changetext()
{
  if(kindoftest == 0 || kindoftest == 1) text->setText("Could you hear the sound?");
  if(kindoftest > 1) text->setText("Could you hear any changes in the sound?");
}

void Testwindow::goodbye()
{
  QString text = "You have completet the test. Thank you very much!\n Now please send the file ";
  text += data->getpath();
  text += " to monty@xiph.org."; // might be a different email address
  QMessageBox::information(this, "mask", text);
  emit(testfinished());
}
