// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the LIBTEMPORALURI_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// LIBTEMPORALURI_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef LIBTEMPORALURI_EXPORTS
#define LIBTEMPORALURI_API __declspec(dllexport)
#else
#define LIBTEMPORALURI_API __declspec(dllimport)
#endif

// This class is exported from the libTemporalURI.dll
class LIBTEMPORALURI_API ClibTemporalURI {
public:
	ClibTemporalURI(void);
	// TODO: add your methods here.
};

extern LIBTEMPORALURI_API int nlibTemporalURI;

LIBTEMPORALURI_API int fnlibTemporalURI(void);
