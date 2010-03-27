	Pollux, a fast Theora decoder created by Michael Scheerer.
	Pollux decoder (c) 2009 Michael Scheerer www.meviatronic.com

	Many thanks to
	Monty <monty@xiph.org> and
	The XIPHOPHORUS Company http://www.xiph.org/ .
	
	Pollux's IDCT based on:
	The official Theora specification.
	
	Features of Pollux's enhanced IDCT:
	- inplacing.
 	- higher degree of content adaptive calculation.
 	  Dr. m. m. Timothy B. Terriberry pointed out, that this approach is only helpful in case
 	  of systems and compilers, which don't have or exploit
 	  the SIMD (Single Instruction Multiple Data) computer architecture.
 	
 	Pollux's colorspace transformation based on:
	The official Theora specification.
	
	Pollux's CRC32 based on (but here used inside the Ogg Framing):
	public domain code by
	Ross Williams (ross@guest.adelaide.edu.au).
	A 32 bit CRC value (direct algorithm, initial val and final XOR = 0, generator polynomial=0x04c11db7) is used.
	The value is computed over the entire header (with the CRC field in the header set to zero)
	and then continued over the page. The CRC field is then filled with the computed value.

	Pollux's huffman decoding based on:
 	Self developed algorithm (deflated binary tree approach -
	a kind of a 2 pow n compression), therefore there's no reference.
	Dr. m. m. Timothy B. Terriberry pointed out, that this kind of approach is at least since 1993 a
	reinvention:
	R. Hashemian, "High Speed Search and Memory Efficient
    Huffman Coding," 1993.

	Pollux's bitstream decoding based on:
 	Self developed algorithm, therefore there's no reference.
 	
	Summary:
	
	Advantages of Pollux over the current reference decoder (libTheora 1.1.1):
	- content adaptive color space transformation.
 	- content adaptive block copying.
 	- content adaptive motion prediction (sparseDC mode).
 	- higher degree of content adaptive IDCT on non SIMD architectures.
 	- higher degree of content adaptive edge detection and deblock filtering.
 	- sparseDC mode.
 	  Dr. m. m. Timothy B. Terriberry pointed out, that there is
 	  a separation of the MC from the DCT in developing for a future reference decoder version,
 	  which even avoids such a special sparseDC mode with the same amount of
 	  reducing the calculation amount.
 	- avoiding a two pass coefficient decoding.
 	
 	Advantages of Pollux over an older Java version (called JTheora) of the reference decoder:
 	- drastical reduced binary or bytecode size
 	  (from 80 Kbyte Java bytecode ripped of encoder stuff to only 50 Kbyte).
 	- support of the specified truncated packet operation.
 	- better huffman decoding.
	- memory consumption is lower (under Java according to the taskmanager).
	- performance consumption is lower (under Java according to the taskmanager).
 	- "feeled" performance improvement of full scaled video playback is nearly 80%
 	  on a single core Pentium M 1.4 GHZ machine.
 	- content adaptive color space transformation.
 	- content adaptive motion prediction (sparseDC mode).
 	- higher degree of content adaptive IDCT.
 	- higher degree of content adaptive edge detection and deblocking.
 	- color space transformation is working in a separate thread - yielding a
 	  better load balancing of the CPU utilization on multicore/multithread machines
 	  or in case of CPU loads of over 100%.
 	- sparseDC mode.
 	- support of the pixel formats 4:4:4 and 4:2:2.
 	- elimination of the "64 * number of blocks"-fold performance bottleneck if-condition
 	  inside the coefficient decoding.
 	
 	The code size and/or memory consumption may be of interest for
 	firmware developers of hardware players.
 	
 	A firmware port of Pollux should replace the recursions during the
 	Huffman decoder initialization. Under Java, this recursion based initialization
 	is faster than the non recursion version.
 	
 	Todo:
 	
 	Exact performance tests against the actual reference decoder.
 	
 	Because of the small codesize of Pollux, the decoder may
 	be the right one for e.g. firmware/HDL/ASIC developers.
 	
 	
 	Appreciations:
 	
 	Very thanks to Dr. m. m. Timothy B. Terriberry for reviewing and error correcting
 	the README and Pollux decoder.
