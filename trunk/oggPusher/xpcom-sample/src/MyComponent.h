#ifndef _MY_COMPONENT_H_
#define _MY_COMPONENT_H_

#include "IMyComponent.h"

#define MY_COMPONENT_CONTRACTID "@mydomain.com/XPCOMSample/MyComponent;1"
#define MY_COMPONENT_CLASSNAME "A Simple XPCOM Sample"
#define MY_COMPONENT_CID {0x166ff10a, 0xea3c, 0x490a, {0x8c, 0x39, 0xd4, 0x53, 0x43, 0x1b, 0xce, 0x3a}}
/* Header file */
class MyComponent : public IMyComponent
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMYCOMPONENT

  MyComponent();
  virtual ~MyComponent();
  /* additional members */
};


#endif //_MY_COMPONENT_H_
