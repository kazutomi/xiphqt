library std;
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
--use ieee.std_logic_arith.all;
--use ieee.std_logic_unsigned.all;
--use ieee.std_logic_signed.all;



entity IDctSlow is
  port (Clk,
        Reset_n : in std_logic;

        in_request : out std_logic;
        in_valid : in std_logic;
        in_data : in signed(15 downto 0);
        in_quantmat : in signed(15 downto 0);

        out_requested : in std_logic;
        out_valid : out std_logic;
        out_data : out signed(15 downto 0)

        );

end entity IDctSlow;





architecture rtl of IDctSlow is

  subtype ogg_int_16_t is signed(15 downto 0);
  subtype ogg_int_32_t is signed(31 downto 0);

  type mem64_t is array (0 to 63) of ogg_int_16_t;
  type mem64_32bits_t is array (0 to 63) of ogg_int_32_t;

  signal InData : mem64_t;
  signal QuantMat : mem64_t;

  signal IntermediateData : mem64_t;
  alias ip : mem64_t is  IntermediateData;
  alias op : mem64_t is  InData;


  signal s_A, s_B, s_C, s_D, s_Ad, s_Bd, s_Cd, s_Dd, s_E, s_F, s_G, s_H : ogg_int_16_t;
  signal s_Ed, s_Gd, s_Add, s_Bdd, s_Fd, s_Hd : ogg_int_16_t;





  
-- FSMs
  type state_t is (readIn,dequant,idct_row,idct_col,proc,writeOut);
  signal state : state_t;


  type idct_row_state_t is (idct_row_st1, idct_row_st2, idct_row_st3, idct_row_st4);
  signal idct_row_state : idct_row_state_t;

  type idct_col_state_t is (idct_col_st1, idct_col_st2, idct_col_st3, idct_col_st4);
  signal idct_col_state : idct_col_state_t;

-- Handshake
--  signal count : std_logic_vector(5 downto 0);
  subtype tiny_int is integer range 0 to 63;
  signal count : tiny_int;
  signal s_in_request : std_logic;
  signal s_out_valid : std_logic;





  
  type dezigzag_t is array (0 to 63) of unsigned(5 downto 0);
  constant dezigzag_index : dezigzag_t := (
    "000000", "000001", "001000", "010000", "001001",
    "000010", "000011", "001010", "010001", "011000",
    "100000", "011001", "010010", "001011", "000100",
    "000101", "001100", "010011", "011010", "100001",
    "101000", "110000", "101001", "100010", "011011",
    "010100", "001101", "000110", "000111", "001110",
    "010101", "011100", "100011", "101010", "110001",
    "111000", "111001", "110010", "101011", "100100",
    "011101", "010110", "001111", "010111", "011110",
    "100101", "101100", "110011", "111010", "111011",
    "110100", "101101", "100110", "011111", "100111",
    "101110", "110101", "111100", "111101", "110110",
    "101111", "110111", "111110", "111111" );




   constant xC1S7 : ogg_int_32_t := "00000000000000001111101100010101";
   constant xC2S6 : ogg_int_32_t := "00000000000000001110110010000011";
   constant xC3S5 : ogg_int_32_t := "00000000000000001101010011011011";
   constant xC4S4 : ogg_int_32_t := "00000000000000001011010100000101";
   constant xC5S3 : ogg_int_32_t := "00000000000000001000111000111010";
   constant xC6S2 : ogg_int_32_t := "00000000000000000110000111111000";
   constant xC7S1 : ogg_int_32_t := "00000000000000000011000111110001";

--    constant xC1S7 : ogg_int_16_t := "1111101100010101";
--    constant xC2S6 : ogg_int_16_t := "1110110010000011";
--    constant xC3S5 : ogg_int_16_t := "1101010011011011";
--    constant xC4S4 : ogg_int_16_t := "1011010100000101";
--    constant xC5S3 : ogg_int_16_t := "1000111000111010";
--    constant xC6S2 : ogg_int_16_t := "0110000111111000";
--    constant xC7S1 : ogg_int_16_t := "0011000111110001";

  
begin

  in_request <= s_in_request;
  out_valid <= s_out_valid;

  process(clk)
    
    procedure ReadIn is
    begin
      s_out_valid <= '0';            -- came from WriteOut, out_valid must be 0
      s_in_request <= '1';

      if( s_in_request = '1' and in_valid = '1' )then

        InData( count ) <= in_data;
        QuantMat( count ) <= in_quantmat;
        
        if( count = 63 )then
          state <= dequant;
          s_in_request <= '0';
          count <= 0;
        else
          count <= count + 1;
        end if;


      end if;
    end procedure ReadIn;
                     

    procedure WriteOut is
    begin
      out_data <= op( count );
      s_out_valid <= '1';
      
      if( out_requested = '1' )then
        if( count = 63 )then
          state <= readIn;          -- on readIn state must set out_valid to 0
          count <= 0;
        else
          count <= count + 1;      
        end if;
      end if;
      

    end procedure WriteOut;










    procedure Dequant_slow is
    begin
      IntermediateData( to_integer(dezigzag_index( count )) ) <=
        "*"(InData(count), QuantMat(count))(15 downto 0);

      if( count = 63 )then
        state <= idct_row;
        count <= 0;
      else
        count <= count + 1;
      end if;


    end procedure Dequant_slow;



    



    -- Inverse DCT on the rows now
    procedure Idct_row is
    begin
      case idct_row_state is
        when idct_row_st1 =>
          s_A <= "*"(xC1S7, ip(1 + count))(31 downto 16) +
                 "*"(xC7S1, ip(7 + count))(31 downto 16);
          
          s_B <= "*"(xC7S1, ip(1 + count))(31 downto 16) -
                 "*"(xC1S7, ip(7 + count))(31 downto 16);

          s_C <= "*"(xC3S5, ip(3 + count))(31 downto 16) +
                 "*"(xC5S3, ip(5 + count))(31 downto 16);

          s_D <= "*"(xC3S5, ip(5 + count))(31 downto 16) -
                 "*"(xC5S3, ip(3 + count))(31 downto 16);

          idct_row_state <= idct_row_st2;
        when idct_row_st2 =>
          s_Ad <= "*"(xC4S4, (s_A - s_C))(31 downto 16);

          s_Bd <= "*"(xC4S4, (s_B - s_D))(31 downto 16);

          s_Cd <= (s_A + s_C);
          s_Dd <= (s_B + s_D);


          s_E <= "*"(xC4S4, (ip(0 + count) + ip(4 + count)) )(31 downto 16);

          
          s_F <= "*"(xC4S4, (ip(0 + count) - ip(4 + count)) )(31 downto 16);

          s_G <= "*"(xC2S6, ip(2 + count))(31 downto 16) +
                 "*"(xC6S2, ip(6 + count))(31 downto 16);

          s_H <= "*"(xC6S2, ip(2 + count))(31 downto 16) -
                 "*"(xC2S6, ip(6 + count))(31 downto 16);
          
          idct_row_state <= idct_row_st3;

        when idct_row_st3 =>
          s_Ed <= (s_E - s_G );
          s_Gd <= (s_E + s_G );

          s_Add <= (s_F + s_Ad );
          s_Bdd <= (s_Bd - s_H );

          s_Fd <= (s_F - s_Ad );
          s_Hd <= (s_Bd + s_H );

          idct_row_state <= idct_row_st4;
          
        when idct_row_st4 =>
          ip(0 + count) <= (s_Gd + s_Cd );
          ip(7 + count) <= (s_Gd - s_Cd );

          ip(1 + count) <= (s_Add + s_Hd );
          ip(2 + count) <= (s_Add - s_Hd );

          ip(3 + count) <= (s_Ed + s_Dd );
          ip(4 + count) <= (s_Ed - s_Dd );

          ip(5 + count) <= (s_Fd + s_Bdd );
          ip(6 + count) <= (s_Fd - s_Bdd );

          idct_row_state <= idct_row_st1;

          if( count = 56 )then
            count <= 0;
            state <= idct_col;
          else
            count <= count + 8;    
          end if;
          

        when others => null;
      end case;    
    end procedure Idct_row;








        -- Inverse DCT on the rows now
    procedure Idct_col is
    begin
       case idct_col_state is
         when idct_col_st1 =>
           s_A <= "*"(xC1S7, ip(1*8 + count))(31 downto 16) +
                  "*"(xC7S1, ip(7*8 + count))(31 downto 16);
          
           s_B <= "*"(xC7S1, ip(1*8 + count))(31 downto 16) -
                  "*"(xC1S7, ip(7*8 + count))(31 downto 16);

           s_C <= "*"(xC3S5, ip(3*8 + count))(31 downto 16) +
                  "*"(xC5S3, ip(5*8 + count))(31 downto 16);

           s_D <= "*"(xC3S5, ip(5*8 + count))(31 downto 16) -
                  "*"(xC5S3, ip(3*8 + count))(31 downto 16);

           idct_col_state <= idct_col_st2;
         when idct_col_st2 =>
           s_Ad <= "*"(xC4S4, (s_A - s_C))(31 downto 16);

           s_Bd <= "*"(xC4S4, (s_B - s_D))(31 downto 16);

           s_Cd <= (s_A + s_C);
           s_Dd <= (s_B + s_D);


           s_E <= "*"(xC4S4, (ip(0*8 + count) + ip(4*8 + count)) )(31 downto 16);
          
           s_F <= "*"(xC4S4, (ip(0*8 + count) - ip(4*8 + count)) )(31 downto 16);

           s_G <= "*"(xC2S6, ip(2*8 + count))(31 downto 16) +
                  "*"(xC6S2, ip(6*8 + count))(31 downto 16);

           s_H <= "*"(xC6S2, ip(2*8 + count))(31 downto 16) -
                  "*"(xC2S6, ip(6*8 + count))(31 downto 16);
          
           idct_col_state <= idct_col_st3;

         when idct_col_st3 =>
           s_Ed <= (s_E - s_G + 8 );
           s_Gd <= (s_E + s_G + 8 );

           s_Add <= (s_F + s_Ad + 8 );
           s_Bdd <= (s_Bd - s_H );

           s_Fd <= (s_F - s_Ad + 8 );
           s_Hd <= (s_Bd + s_H );

           idct_col_state <= idct_col_st4;
          
         when idct_col_st4 =>
           op(0*8 + count) <= shift_right( (s_Gd + s_Cd ), 4 )(15 downto 0);
           op(7*8 + count) <= shift_right( (s_Gd - s_Cd ), 4 )(15 downto 0);

           op(1*8 + count) <= shift_right( (s_Add + s_Hd ), 4 )(15 downto 0);
           op(2*8 + count) <= shift_right( (s_Add - s_Hd ), 4 )(15 downto 0);

           op(3*8 + count) <= shift_right( (s_Ed + s_Dd ), 4 )(15 downto 0);
           op(4*8 + count) <= shift_right( (s_Ed - s_Dd ), 4 )(15 downto 0);

           op(5*8 + count) <= shift_right( (s_Fd + s_Bdd ), 4 )(15 downto 0);
           op(6*8 + count) <= shift_right( (s_Fd - s_Bdd ), 4 )(15 downto 0);

           idct_col_state <= idct_col_st1;

           if( count = 7 )then
             count <= 0;
             state <= writeOut;
           else
             count <= count + 1;    
           end if;
          

         when others => null;
       end case;    
    end procedure Idct_col;



    

    
--     procedure Proc is
--     begin
      
--       op( count ) <= ip( count );

--       if( count = 63 )then
--         state <= writeOut;
--         count <= 0;
--       else
--         count <= count + 1;
--       end if;
      

--     end procedure Proc;



    
  begin

    
     if( Reset_n = '0' ) then
       state <= readIn;
       s_in_request <= '0';
       count <= 0;
       s_out_valid <= '0';
       idct_row_state <= idct_row_st1;
       
     elsif(clk'event and clk = '1') then
       case state is
         when readIn => ReadIn;
         when dequant => Dequant_slow;
         when idct_row => Idct_row;
         when idct_col => Idct_col;
--          when proc => proc;
         when writeOut => WriteOut;

         when others => ReadIn; state <= readIn;
       end case;  

     end if;
  end process;


  


end rtl;
