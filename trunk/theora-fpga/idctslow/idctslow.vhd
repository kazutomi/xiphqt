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

  signal IntermediateData : mem64_t;
  alias ip : mem64_t is  IntermediateData;


  signal s_A, s_B, s_C, s_D, s_Ad, s_Bd, s_Cd, s_Dd, s_E, s_F, s_G, s_H : ogg_int_16_t;
  signal s_Ed, s_Gd, s_Add, s_Bdd, s_Fd, s_Hd : ogg_int_16_t;




  
-- FSMs
  type state_t is (readIn,idct_row,idct_col,proc,writeOut);
  signal state : state_t;


  type row_state_t is (row_st1, row_st2, row_st3, row_st4);
  signal row_state : row_state_t;

  type col_state_t is (col0, col1, col2, col3, col4, col5, col6, col7, col8, col9, col10, col11, col12, col13, col14, col15, col16);
  signal col_state : col_state_t;




  type write_state_t is (w_st1, w_st2, w_st3, w_st4);
  signal write_state : write_state_t;



  
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



  

  signal mem0_we  : std_logic;
  signal mem0_waddr : unsigned(5 downto 0) := "000000";
  signal mem0_raddr : unsigned(5 downto 0) := "000000";
  signal mem0_wdata : ogg_int_16_t;
  signal mem0_rdata : ogg_int_16_t;


  
begin

  -- mem0 = 64 x 16 bits
   mem0 : entity work.syncram
     generic map( DEPTH => 64, DATA_WIDTH => 16, ADDR_WIDTH => 6 )
     port map(clk, mem0_we, mem0_waddr, mem0_wdata, mem0_raddr, mem0_rdata );


   out_data <= mem0_rdata;


  
  in_request <= s_in_request;
  out_valid <= s_out_valid;

  process(clk)
    
    procedure ReadIn is
    begin
      s_out_valid <= '0';            -- came from WriteOut, out_valid must be 0
      s_in_request <= '1';

      if( s_in_request = '1' and in_valid = '1' )then

        IntermediateData( to_integer(dezigzag_index( count )) ) <=
          "*"( in_data , in_quantmat)(15 downto 0);

        
        if( count = 63 )then
          state <= idct_row;
          s_in_request <= '0';
          count <= 0;
        else
          count <= count + 1;
        end if;


      end if;
    end procedure ReadIn;
                     

    procedure WriteOut is
      variable nextCount : integer range 0 to 63;
    begin
      s_out_valid <= '0';
      case write_state is
        when w_st1 =>
          write_state <= w_st2;
          mem0_raddr <= to_unsigned(count,6);

        when w_st2 =>
          write_state <= w_st3;

        when w_st3 =>
          s_out_valid <= '1';
          
          if( out_requested = '1' )then           
            if( count = 63 )then
              state <= readIn;          -- on readIn state must set out_valid to 0
              count <= 0;
              write_state <= w_st1;
            else
              nextCount := count + 1;
              mem0_raddr <= to_unsigned(nextCount,6);
              count <= nextCount;
              write_state <= w_st2;
            end if;
          end if;
        when others => null;
      end case;

    end procedure WriteOut;












    



    -- Inverse DCT on the rows now
    procedure Idct_row is
    begin
      case row_state is
        when row_st1 =>
          s_A <= "*"(xC1S7, ip(1 + count))(31 downto 16) +
                 "*"(xC7S1, ip(7 + count))(31 downto 16);
          
          s_B <= "*"(xC7S1, ip(1 + count))(31 downto 16) -
                 "*"(xC1S7, ip(7 + count))(31 downto 16);

          s_C <= "*"(xC3S5, ip(3 + count))(31 downto 16) +
                 "*"(xC5S3, ip(5 + count))(31 downto 16);

          s_D <= "*"(xC3S5, ip(5 + count))(31 downto 16) -
                 "*"(xC5S3, ip(3 + count))(31 downto 16);

          row_state <= row_st2;
        when row_st2 =>
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
          
          row_state <= row_st3;

        when row_st3 =>
          s_Ed <= (s_E - s_G );
          s_Gd <= (s_E + s_G );

          s_Add <= (s_F + s_Ad );
          s_Bdd <= (s_Bd - s_H );

          s_Fd <= (s_F - s_Ad );
          s_Hd <= (s_Bd + s_H );

          row_state <= row_st4;
          
        when row_st4 =>
          ip(0 + count) <= (s_Gd + s_Cd );
          ip(7 + count) <= (s_Gd - s_Cd );

          ip(1 + count) <= (s_Add + s_Hd );
          ip(2 + count) <= (s_Add - s_Hd );

          ip(3 + count) <= (s_Ed + s_Dd );
          ip(4 + count) <= (s_Ed - s_Dd );

          ip(5 + count) <= (s_Fd + s_Bdd );
          ip(6 + count) <= (s_Fd - s_Bdd );

          row_state <= row_st1;

          if( count = 56 )then
            count <= 0;
            state <= proc;
          else
            count <= count + 8;    
          end if;
          

        when others => null;
      end case;    
    end procedure Idct_row;








        -- Inverse DCT on the rows now
    procedure Idct_col is
    begin
       case col_state is
         when col0 =>
           col_state <= col1;
           s_A <= "*"(xC1S7, ip(1*8 + count))(31 downto 16) +
                  "*"(xC7S1, ip(7*8 + count))(31 downto 16);

         when col1 =>
           col_state <= col2;
           s_B <= "*"(xC7S1, ip(1*8 + count))(31 downto 16) -
                  "*"(xC1S7, ip(7*8 + count))(31 downto 16);

         when col2 =>
           col_state <= col3;
           s_C <= "*"(xC3S5, ip(3*8 + count))(31 downto 16) +
                  "*"(xC5S3, ip(5*8 + count))(31 downto 16);

         when col3 =>
           col_state <= col4;
           s_D <= "*"(xC3S5, ip(5*8 + count))(31 downto 16) -
                  "*"(xC5S3, ip(3*8 + count))(31 downto 16);


         when col4 =>
           col_state <= col5;
           s_Ad <= "*"(xC4S4, (s_A - s_C))(31 downto 16);

           s_Bd <= "*"(xC4S4, (s_B - s_D))(31 downto 16);

           s_Cd <= (s_A + s_C);
           s_Dd <= (s_B + s_D);

         when col5 =>
           col_state <= col6;
           s_E <= "*"(xC4S4, (ip(0*8 + count) + ip(4*8 + count)) )(31 downto 16);
          
           s_F <= "*"(xC4S4, (ip(0*8 + count) - ip(4*8 + count)) )(31 downto 16);

         when col6 =>
           col_state <= col7;
           s_G <= "*"(xC2S6, ip(2*8 + count))(31 downto 16) +
                  "*"(xC6S2, ip(6*8 + count))(31 downto 16);

         when col7 =>
           col_state <= col8;
           s_H <= "*"(xC6S2, ip(2*8 + count))(31 downto 16) -
                  "*"(xC2S6, ip(6*8 + count))(31 downto 16);
          
         when col8 =>
           col_state <=col9;

           s_Ed <= (s_E - s_G + 8 );
           s_Gd <= (s_E + s_G + 8 );

           s_Add <= (s_F + s_Ad + 8 );
           s_Bdd <= (s_Bd - s_H );

           s_Fd <= (s_F - s_Ad + 8 );
           s_Hd <= (s_Bd + s_H );

         when col9 =>
           col_state <= col10;
           mem0_we <= '1';
           mem0_waddr <= to_unsigned(0*8 + count,6);
           mem0_wdata <= shift_right( (s_Gd + s_Cd ), 4 )(15 downto 0);
           
         when col10 =>
           col_state <= col11;
           mem0_we <= '1';
           mem0_waddr <= to_unsigned(7*8 + count,6);
           mem0_wdata <= shift_right( (s_Gd - s_Cd ), 4 )(15 downto 0);

           
         when col11 =>
           col_state <= col12;         
           mem0_we <= '1';
           mem0_waddr <= to_unsigned(1*8 + count,6);
           mem0_wdata <= shift_right( (s_Add + s_Hd ), 4 )(15 downto 0);

         when col12 =>
           col_state <= col13;
           mem0_we <= '1';
           mem0_waddr <= to_unsigned(2*8 + count,6);
           mem0_wdata <= shift_right( (s_Add - s_Hd ), 4 )(15 downto 0);

         when col13 =>
           col_state <= col14;
           mem0_we <= '1';
           mem0_waddr <= to_unsigned(3*8 + count,6);
           mem0_wdata <= shift_right( (s_Ed + s_Dd ), 4 )(15 downto 0);

         when col14 =>
           col_state <= col15;
           mem0_we <= '1';
           mem0_waddr <= to_unsigned(4*8 + count,6);
           mem0_wdata <= shift_right( (s_Ed - s_Dd ), 4 )(15 downto 0);

         when col15 =>
           col_state <= col16;
           mem0_we <= '1';
           mem0_waddr <= to_unsigned(5*8 + count,6);
           mem0_wdata <= shift_right( (s_Fd + s_Bdd ), 4 )(15 downto 0);

         when col16 =>
           col_state <= col0;
           mem0_we <= '1';
           mem0_waddr <= to_unsigned(6*8 + count,6);
           mem0_wdata <= shift_right( (s_Fd - s_Bdd ), 4 )(15 downto 0);

           
           if( count = 7 )then
             count <= 0;
             state <= writeOut;
           else
             count <= count + 1;    
           end if;
          

         when others => null;
       end case;    
    end procedure Idct_col;



    

    
     procedure Proc is
     begin
       mem0_we <= '1';
       mem0_waddr <= to_unsigned(count,6);
       mem0_wdata <= ip( count );

       
       if( count = 63 )then
         state <= writeOut;
         count <= 0;
       else
         count <= count + 1;
       end if;
      

     end procedure Proc;



    
  begin

    
     if( Reset_n = '0' ) then
       state <= readIn;
       s_in_request <= '0';
       count <= 0;
       s_out_valid <= '0';
       row_state <= row_st1;
       col_state <= col0;
       mem0_we <= '0';
       mem0_raddr <= "000000";
       mem0_waddr <= "000000";

       write_state <= w_st1;

     elsif(clk'event and clk = '1') then
       mem0_we <= '0';

       case state is
         when readIn => ReadIn;
         when idct_row => Idct_row;
         when idct_col => Idct_col;
         when proc => proc;
         when writeOut => WriteOut;

         when others => ReadIn; state <= readIn;
       end case;  

     end if;
  end process;


  


end rtl;
