/*
   Copyright (C) 2002, 2003, 2004 Zentaro Kavanagh
   
   Copyright (C) 2003, 2004 Commonwealth Scientific and Industrial Research
   Organisation (CSIRO) Australia

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of Zentaro Kavanagh nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#pragma once
using namespace System::Runtime::InteropServices;
using namespace System;


//This is a small subset of the full wrappers marshalling class. It only includes the string
// marshalling functions.
namespace illiminable {
namespace libiWrapper {
	public __gc class Wrappers
	{
		public:
			Wrappers(void);
			~Wrappers(void);

			/// Converts a .NET String to a C String. Must call releaseCStr when done.
			static char* netStrToCStr(String* inNetString);
			
			/// To be called when you are done with the CString so it can be deleted internally.
			static void releaseCStr(char* inCStr);

			/// Converts an ANSI C String to a .NET String.
			static String* CStrToNetStr(const char* inCStr);

			/// Converts a wide (2 byte) string to a .NET string.
			static String* WStrToNetStr(const wchar_t* inWStr);

			/// Converts a .NET string to a wide (2 byte) string.  Must call releaseWStr when done.
			static wchar_t* netStrToWStr(String* inNetString);

			/// To be called when you are done with the C String so it can be internall deleted.
			static void releaseWStr(wchar_t* inWStr);
	};
}
}