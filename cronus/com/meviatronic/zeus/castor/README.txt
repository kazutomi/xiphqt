	Castor, a fast Vorbis decoder created by Michael Scheerer.
	Castor decoder (c) 2009 Michael Scheerer www.meviatronic.com

	Many thanks to
	Monty <monty@xiph.org> and
	The XIPHOPHORUS Company http://www.xiph.org/ .
	
	Castor's IMDCT based on:
	The use of multirate filter banks for coding of high quality digital audio,
	6th European Signal Processing Conference (EUSIPCO), Amsterdam, June 1992, Vol.1, pages 211-214Th,
	Sporer Kh.Brandenburg B.Edler
	
	Features:
 	- 9 loops and one threefold loop.
	
	Features of Castor's enhanced IMDCT:
	- inplacing inside the performance critical part.
 	- 3 loops and one threefold nested loop.
 	- includes windowing.
 	- includes dot product.
	- 2 loops plus a windowing loop less than the IMDCT of the current reference decoder.
	
	Castor's CRC32 based on (but here used inside the Ogg Framing):
	public domain code by
	Ross Williams (ross@guest.adelaide.edu.au).
	A 32 bit CRC value (direct algorithm, initial val and final XOR = 0, generator polynomial=0x04c11db7) is used.
	The value is computed over the entire header (with the CRC field in the header set to zero)
	and then continued over the page. The CRC field is then filled with the computed value.

	Castor's huffman decoding based on:
 	Self developed algorithm (deflated binary tree approach -
	a kind of a 2 pow n compression), therefore there's no reference.
	Dr. m. m. Timothy B. Terriberry pointed out, that this kind of approach is at least since 1993 a
	reinvention:
	R. Hashemian, "High Speed Search and Memory Efficient
    	Huffman Coding," 1993.
	
	Castor's bitstream decoding based on:
 	Self developed algorithm, therefore there's no reference.

	Summary:
	
	Advantages of Castor over the current reference decoder (libVorbis 1.2.3):
	- in some technical aspects better IMDCT.
	- drastical reduced binary or bytecode size
 	  (from 50 Kbyte Java bytecode ripped of floor 0 and encoder stuff to only 35 Kbyte including Helena).
 	- support of VQ lookup type 0 (the spec describes type 0 ambigiously as
 	  indirectly allowed and forbidden).
 	- support of all specified truncated packet operations.
 	- support of codebooks with a single used entry.
 	
 	Advantages of Castor over an older Java version of the reference decoder:
	- memory consumption is lower (under Java according to the taskmanager).
	- performance consumption is lower (under Java according to the taskmanager).
 	
 	The decoder itself supports full gapless playerback and positive time offset (broadcaststream).
 	
 	The code size and/or memory consumption may be of interest for
 	firmware developers of hardware players.
 	
 	A firmware port of Castor should replace the recursions during the
 	Huffman decoder initialization. Under Java, this recursion based initialization
 	is faster than the non recursion version.
 	
 	Todo:
 	
 	Exact performance tests against the actual reference decoder.
 	
 	Because of the small codesize of Castor, the decoder may
 	be the right one for e.g. firmware/HDL/ASIC developers.
 	
 	A C++ version of this decoder should be tested with the huffman decoding
 	approach of the reference decoder.
 		
 	Floor 0 decoding may be implemented, if neccessary.
 	According to
 	
	http://www.mp3-tech.org/programmer/docs/embedded_vorbis_thesis.pdf
	
	is this floor type not used anymore, due to poor performance and high decoder
 	complexity compared to floor 1. Floor 0 was replaced in an early beta, but is still part
 	of the standard end hence needs to be supported by a compliant decoder.
 	
 	Since the current Ogg/Vorbis standard includes features that are not currently used,
 	some very unlikely to ever be used (e.g. window lengths of up to 8192 samples), and
 	others that are simply outdated (e.g. floor type 0), it is possible to remove support
 	for these features from the decoder while still being able to play back almost every
 	available Ogg/Vorbis stream. Of course the decoder can no longer be considered fully
 	compliant with the standard.
 	
 	Appreciations:
 	
 	Very thanks to Dr. m. m. Timothy B. Terriberry for reviewing
 	the huffman decoding.
