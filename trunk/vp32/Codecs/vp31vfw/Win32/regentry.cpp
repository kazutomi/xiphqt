#include "regentry.h"
//#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
 
int Registry_GetEntry(void *data, REGISTRY_TYPE r, 
					  unsigned long *sizeItem, char *nameItem, RegistryAccess regAccess)
{
	HKEY hKey;
	DWORD vLen( 255);  
	DWORD sh;
	char buffer[1024];

	// initialize everything
	int ret=0;
	*sizeItem=0;
	*((char *) buffer)=0;

	// Open the key 
	if( ret==0 && RegOpenKeyEx( HKEY_LOCAL_MACHINE,
		(const char *) regAccess,0,KEY_QUERY_VALUE,&hKey) != ERROR_SUCCESS)
        ret =  -2;

	// Get the size of the value of the item in the key
    if( ret==0 && RegQueryValueEx( hKey,nameItem,NULL,&sh,
		NULL, sizeItem)  != ERROR_SUCCESS)
		ret =  -1;

	// Get the value of the item 
    if( ret==0 && RegQueryValueEx( hKey,nameItem,NULL,&sh,
        (unsigned char *)buffer, sizeItem)  != ERROR_SUCCESS)
        ret =  -1;

	if (ret>-2)
		RegCloseKey( hKey);              

	switch (r)
	{
		case REG_INTEGER:
			sscanf(buffer,"%ld",(long *) data);break;
		case REG_DOUBLE:
			sscanf(buffer,"%lf",(double *) data);break;
		default:
			*((unsigned char *) data)=0;
			memcpy(data,buffer,*sizeItem);break;
	}

	return ret;

}
int Registry_SetEntry(void *data, REGISTRY_TYPE r, 
					  unsigned long sizeItem, char *nameItem, RegistryAccess regAccess)
{
    HKEY hKey; 
	char outdata[1024];
	switch (r)
	{
		case REG_INTEGER:
			sprintf(outdata,"%d", * (long *) data);break;
		case REG_DOUBLE:
			sprintf(outdata,"%g",* (double *) data);break;
		default:
			*((unsigned char *) outdata)=0;
			memcpy(outdata,data,sizeItem+1);break;
	}
	
	unsigned long OpenOrCreate;
/*    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,(const char *) regAccess ,0,
        KEY_ALL_ACCESS,&hKey) != ERROR_SUCCESS)
		*/
    if (RegCreateKeyEx(
		HKEY_LOCAL_MACHINE,
		(const char *) regAccess,
		0,           
		0,           
		REG_OPTION_NON_VOLATILE,  
		KEY_ALL_ACCESS,        
		0,
		
		&hKey,          
		&OpenOrCreate   
		) != ERROR_SUCCESS)

	{ 
		return -1;
	}
	else
	{		
		if ( RegSetValueEx( 
			hKey,
			nameItem,   
			0,
			REG_SZ,
			(const unsigned char *) outdata,
			strlen(outdata))  != ERROR_SUCCESS) 
		{
			
			RegCloseKey( hKey);             
			return -1;
			
		}
		RegCloseKey( hKey);
		return 0;             
	}
}


int Registry_Open(RegistryAccess regAccess, char *mode)
{
	return 0;
}

int Registry_Close(RegistryAccess regAccess)
{
	return 0;
}







