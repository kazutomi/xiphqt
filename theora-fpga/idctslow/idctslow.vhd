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



  signal s_A, s_B, s_C, s_D, s_E, s_F, s_G, s_H : ogg_int_16_t;



  
-- FSMs
  type state_t is (readIn,idct,proc,writeOut);
  signal state : state_t;


  type idct_state_t is (
    idct_st1, idct_st2, idct_st3, idct_st4, idct_st5,
    idct_st6, idct_st7, idct_st8, idct_st9, idct_st10,
    idct_st11, idct_st12, idct_st13, idct_st14, idct_st15,
    idct_st16 );
  signal idct_state : idct_state_t;


  signal col_loop : std_logic;

  
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



  signal mem1_we  : std_logic;
  signal mem1_waddr : unsigned(5 downto 0) := "000000";
  signal mem1_wdata : ogg_int_16_t;
  signal mem1_raddr1 : unsigned(5 downto 0) := "000000";
  signal mem1_rdata1 : ogg_int_16_t;
  signal mem1_raddr2 : unsigned(5 downto 0) := "000000";
  signal mem1_rdata2 : ogg_int_16_t;

  
begin

  -- mem1 = dual 64 x 16 bits
   mem1 : entity work.dual_syncram
     generic map( DEPTH => 64, DATA_WIDTH => 16, ADDR_WIDTH => 6 )
     port map(clk, mem1_we, mem1_waddr, mem1_wdata, mem1_raddr1, mem1_rdata1, mem1_raddr2, mem1_rdata2 );

   
   out_data <= mem1_rdata1;


  
  in_request <= s_in_request;
  out_valid <= s_out_valid;

  process(clk)
    
    procedure ReadIn is
    begin
      s_out_valid <= '0';            -- came from WriteOut, out_valid must be 0
      s_in_request <= '1';

      if( s_in_request = '1' and in_valid = '1' )then
        mem1_waddr <= dezigzag_index( count );
        mem1_wdata <= "*"( in_data , in_quantmat)(15 downto 0);
        mem1_we <= '1';

        
        if( count = 63 )then
          state <= idct;
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
          mem1_raddr1 <= to_unsigned(count,6);

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
              mem1_raddr1 <= to_unsigned(nextCount,6);
              count <= nextCount;
              write_state <= w_st2;
            end if;
          end if;
        when others => null;
      end case;

    end procedure WriteOut;












    



    -- Inverse DCT on the rows now
    procedure Idct is
      variable col : integer range 0 to 8;
      variable adjustBeforeShift : integer range 0 to 8;
      variable wdata_aux : ogg_int_16_t;
    begin

      if( col_loop = '1' )then
        col := 8;
        adjustBeforeShift := 8;
      else
        col := 1;
        adjustBeforeShift := 0;
      end if;



      case idct_state is
        when idct_st1 =>
          idct_state <= idct_st2;
          mem1_raddr1 <= to_unsigned(1*col + count, 6);
          mem1_raddr2 <= to_unsigned(7*col + count, 6);
          
        when idct_st2 =>
          idct_state <= idct_st3;

          mem1_raddr1 <= to_unsigned(3*col + count, 6);
          mem1_raddr2 <= to_unsigned(5*col + count, 6);

        when idct_st3 =>
          idct_state <= idct_st4;
           s_A <= "*"(xC1S7, mem1_rdata1)(31 downto 16) +
                  "*"(xC7S1, mem1_rdata2)(31 downto 16);
          
           s_B <= "*"(xC7S1, mem1_rdata1)(31 downto 16) -
                  "*"(xC1S7, mem1_rdata2)(31 downto 16);

        when idct_st4 =>
          idct_state <= idct_st5;        

          s_C <= "*"(xC3S5, mem1_rdata1 )(31 downto 16) +
                 "*"(xC5S3, mem1_rdata2 )(31 downto 16);

          s_D <= "*"(xC3S5, mem1_rdata2 )(31 downto 16) -
                 "*"(xC5S3, mem1_rdata1 )(31 downto 16);

          mem1_raddr1 <= to_unsigned(0*col + count, 6);
          mem1_raddr2 <= to_unsigned(4*col + count, 6);
          
        when idct_st5 =>
          idct_state <= idct_st6;        
          
          s_A <= "*"(xC4S4, (s_A - s_C))(31 downto 16);

          s_B <= "*"(xC4S4, (s_B - s_D))(31 downto 16);

          s_C <= (s_A + s_C);
          s_D <= (s_B + s_D);

          mem1_raddr1 <= to_unsigned(2*col + count, 6);
          mem1_raddr2 <= to_unsigned(6*col + count, 6);

        when idct_st6 =>
          idct_state <= idct_st7;

          s_E <= "*"(xC4S4, ( mem1_rdata1 + mem1_rdata2) )(31 downto 16);          
          s_F <= "*"(xC4S4, ( mem1_rdata1 - mem1_rdata2) )(31 downto 16);


        when idct_st7 =>
          idct_state <= idct_st8;
          
          s_G <= "*"(xC2S6, mem1_rdata1)(31 downto 16) +
                 "*"(xC6S2, mem1_rdata2)(31 downto 16);

          s_H <= "*"(xC6S2, mem1_rdata1)(31 downto 16) -
                 "*"(xC2S6, mem1_rdata2)(31 downto 16);

        when idct_st8 =>
          idct_state <= idct_st9;

          s_E <= (s_E - s_G + adjustBeforeShift );
          s_G <= (s_E + s_G + adjustBeforeShift );

          s_A <= (s_F + s_A + adjustBeforeShift );
          s_B <= (s_B - s_H );

          s_F <= (s_F - s_A + adjustBeforeShift );
          s_H <= (s_B + s_H );
        
        when idct_st9 =>
          idct_state <= idct_st10;
          mem1_waddr <= to_unsigned( 0*col + count, 6 );        
          mem1_we <= '1';          
          wdata_aux := (s_G + s_C );
          
        when idct_st10 =>
          idct_state <= idct_st11;
          mem1_waddr <= to_unsigned( 7*col + count, 6 );
          mem1_we <= '1';
          wdata_aux := (s_G - s_C );

        when idct_st11 =>
          idct_state <= idct_st12;
          mem1_waddr <= to_unsigned( 1*col + count, 6 );
          mem1_we <= '1';
          wdata_aux := (s_A + s_H );
          
        when idct_st12 =>
          idct_state <= idct_st13;
          mem1_waddr <= to_unsigned( 2*col + count, 6 );
          mem1_we <= '1';
          wdata_aux := (s_A - s_H );
          
        when idct_st13 =>
          idct_state <= idct_st14;
          mem1_waddr <= to_unsigned( 3*col + count, 6 );
          mem1_we <= '1';
          wdata_aux := (s_E + s_D );
          
        when idct_st14 =>
          idct_state <= idct_st15;
          mem1_waddr <= to_unsigned( 4*col + count, 6 );
          mem1_we <= '1';
          wdata_aux := (s_E - s_D );
          
        when idct_st15 =>
          idct_state <= idct_st16;
          mem1_waddr <= to_unsigned( 5*col + count, 6 );
          mem1_we <= '1';
          wdata_aux := (s_F + s_B );
          
        when idct_st16 =>
          idct_state <= idct_st1;
          mem1_waddr <= to_unsigned( 6*col + count, 6 );
          mem1_we <= '1';
          wdata_aux := (s_F - s_B );

          if( col_loop = '1' )then
            if( count = 7 )then
              count <= 0;
              state <= writeOut;
              col_loop <= '0';
            else
              count <= count + 1;    
            end if;
          else
            if( count = 56 )then
              count <= 0;
              col_loop <= '1';
            else
              count <= count + 8;    
            end if;
          end if;


        when others => null;
      end case;    

      if( col_loop = '1' )then
        mem1_wdata <= shift_right( wdata_aux, 4 )(15 downto 0);
      else
        mem1_wdata <= wdata_aux;
      end if;

    end procedure Idct;










    

    
--      procedure Proc is
--      begin

       
--        if( count = 63 )then
--          state <= writeOut;
--          count <= 0;
--        else
--          count <= count + 1;
--        end if;
      

--      end procedure Proc;



    
  begin
     if(clk'event and clk = '1') then
       if( Reset_n = '0' ) then
         state <= readIn;
         s_in_request <= '0';
         count <= 0;
         s_out_valid <= '0';
         idct_state <= idct_st1;

         mem1_we <= '0';
         mem1_waddr <= "000000";
         mem1_raddr1 <= "000000";
         mem1_raddr2 <= "000000";
         
         write_state <= w_st1;

         col_loop <= '0';
         
       else
         mem1_we <= '0';
         
         case state is
           when readIn => ReadIn;
           when idct => Idct;
                        --when proc => proc;
           when writeOut => WriteOut;

           when others => ReadIn; state <= readIn;
         end case;  
       end if;
     end if;
  end process;


  


end rtl;
