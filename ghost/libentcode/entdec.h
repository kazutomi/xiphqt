#if !defined(_entdec_H)
# define _entdec_H (1)
# include "entcode.h"



typedef struct ec_dec ec_dec;



/*The entropy decoder.*/
struct ec_dec{
   /*The buffer to decode.*/
   ec_byte_buffer *buf;
   /*The remainder of a buffered input symbol.*/
   int             rem;
   /*The number of values in the current range.*/
   ec_uint32       rng;
   /*The difference between the input value and the lowest value in the current
      range.*/
   ec_uint32       dif;
   /*Normalization factor.*/
   ec_uint32       nrm;
};


/*Initializes the decoder.
  _buf: The input buffer to use.
  Return: 0 on success, or a negative value on error.*/
void ec_dec_init(ec_dec *_this,ec_byte_buffer *_buf);
/*Calculates the cumulative frequency for the next symbol.
  This can then be fed into the probability model to determine what that
   symbol is, and the additional frequency information required to advance to
   the next symbol.
  This function cannot be called more than once without a corresponding call to
   ec_dec_update(), or decoding will not proceed correctly.
  _ft: The total frequency of the symbols in the alphabet the next symbol was
        encoded with.
  Return: A cumulative frequency representing the encoded symbol.
          If the cumulative frequency of all the symbols before the one that
           was encoded was fl, and the cumulative frequency of all the symbols
           up to and including the one encoded is fh, then the returned value
           will fall in the range [fl,fh).*/
unsigned ec_decode(ec_dec *_this,unsigned _ft);
/*Advance the decoder past the next symbol using the frequency information the
   symbol was encoded with.
  Exactly one call to ec_decode() must have been made so that all necessary
   intermediate calculations are performed.
  _fl:  The cumulative frequency of all symbols that come before the symbol
         decoded.
  _fh:  The cumulative frequency of all symbols up to and including the symbol
        Together with _fl, this defines the range [_fl,_fh) in which the value
         returned above must fall.
  _ft:  The total frequency of the symbols in the alphabet the symbol decoded
         was encoded in.
        This must be the same as passed to the preceding call to ec_decode().*/
void ec_dec_update(ec_dec *_this,unsigned _fl,unsigned _fh,
 unsigned _ft);
/*Extracts a sequence of raw bits from the stream.
  The bits must have been encoded with ec_enc_bits().
  No call to ec_dec_update() is necessary after this call.
  _ftb: The number of bits to extract.
        This must be at least one, and no more than 32.
  Return: The decoded bits.*/
ec_uint32 ec_dec_bits(ec_dec *_this,int _ftb);
/*Extracts a raw unsigned integer with a non-power-of-2 range from the stream.
  The bits must have been encoded with ec_enc_uint().
  No call to ec_dec_update() is necessary after this call.
  _ft: The number of integers that can be decoded (one more than the max).
       This must be at least one, and no more than 2**32-1.
  Return: The decoded bits.*/
ec_uint32 ec_dec_uint(ec_dec *_this,ec_uint32 _ft);

#endif