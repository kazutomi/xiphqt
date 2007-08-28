-------------------------------------------------------------------------------
--  Description: Do the iDCTSlow job.
-------------------------------------------------------------------------------

library std;
library ieee;
library work;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.all;


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
  component dual_syncram
    generic (
      DEPTH : positive := 64;             -- How many slots
      DATA_WIDTH : positive := 16;        -- How many bits per slot
      ADDR_WIDTH : positive := 6          -- = ceil(log2(DEPTH))
      );
    port (
      clk : in std_logic;
      wr_e  : in std_logic;
      wr_addr : in unsigned(ADDR_WIDTH-1 downto 0);
      wr_data : in signed(DATA_WIDTH-1 downto 0);
      rd1_addr : in unsigned(ADDR_WIDTH-1 downto 0);
      rd1_data : out signed(DATA_WIDTH-1 downto 0);
      rd2_addr : in unsigned(ADDR_WIDTH-1 downto 0);
      rd2_data : out signed(DATA_WIDTH-1 downto 0)
      );
  end component;

  
  subtype ogg_int_16_t is signed(15 downto 0);
  subtype ogg_int_32_t is signed(31 downto 0);
  
  type mem64_t is array (0 to 63) of ogg_int_16_t;
  
  signal s_A, s_B, s_C, s_D, s_Ad, s_Bd, s_Cd, s_Dd, s_E, s_F, s_G, s_H : ogg_int_16_t;
  signal s_Ed, s_Gd, s_Add, s_Bdd, s_Fd, s_Hd : ogg_int_16_t;

  signal row_loop : std_logic;
  
-- FSMs
  type state_t is (readIn,idct,writeOut);
  signal state : state_t;
  
  type idct_state_t is (idct_st1,  idct_st2,  idct_st3,  idct_st4,
                        idct_st5,  idct_st6,  idct_st7,  idct_st8,
                        idct_st9,  idct_st10, idct_st11, idct_st12,
                        idct_st13, idct_st14, idct_st15, idct_st16);
  signal idct_state : idct_state_t;
  
  type wout_state_t is (wout_st1, wout_st2, wout_st3);
  signal wout_state : wout_state_t;

-- Memory

  signal mem_we  : std_logic;
  signal mem_waddr : unsigned(5 downto 0);
  signal mem_wdata : ogg_int_16_t;
  signal mem_raddr1 : unsigned(5 downto 0);
  signal mem_rdata1 : ogg_int_16_t;
  signal mem_raddr2 : unsigned(5 downto 0);
  signal mem_rdata2 : ogg_int_16_t;
  
-- Handshake
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
  
  
  
-- cos(n*pi/16) or sin(8-n)*pi/16)  
  constant xC1S7 : ogg_int_32_t := "00000000000000001111101100010101";
  constant xC2S6 : ogg_int_32_t := "00000000000000001110110010000011";
  constant xC3S5 : ogg_int_32_t := "00000000000000001101010011011011";
  constant xC4S4 : ogg_int_32_t := "00000000000000001011010100000101";
  constant xC5S3 : ogg_int_32_t := "00000000000000001000111000111010";
  constant xC6S2 : ogg_int_32_t := "00000000000000000110000111111000";
  constant xC7S1 : ogg_int_32_t := "00000000000000000011000111110001";


begin
  
  -- Data matrix 8 x 8 x 16 bits
  mem : dual_syncram
     generic map( DEPTH => 64, DATA_WIDTH => 16, ADDR_WIDTH => 6 )
     port map(clk, mem_we, mem_waddr, mem_wdata, mem_raddr1, mem_rdata1, mem_raddr2, mem_rdata2 );
  
  in_request <= s_in_request;
  out_valid <= s_out_valid;
  out_data <= mem_rdata1;
  
  
  process(clk)
    
    procedure ReadIn is
    begin
      s_out_valid <= '0';            -- came from WriteOut, out_valid must be 0
      s_in_request <= '1';
      
      if( s_in_request = '1' and in_valid = '1' )then
        mem_waddr <= dezigzag_index( count );
        mem_wdata <= "*"( in_data, in_quantmat )(15 downto 0);
        mem_we <= '1';
        
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
    begin
      case wout_state is
        when wout_st1 =>
          wout_state <= wout_st2;
          mem_raddr1 <= to_unsigned(count,6);

        when wout_st2 =>                -- Wait for the memory delay
          wout_state <= wout_st3;
          s_out_valid <= '0';
        
        when wout_st3 =>
          s_out_valid <= '1';
          
          if( out_requested = '1' )then
            if( count = 63 )then
              wout_state <= wout_st1;
              state <= readIn;          -- on readIn state must set out_valid to 0
              count <= 0;
            else
              wout_state <= wout_st2;
              mem_raddr1 <= to_unsigned(count + 1,6);
              count <= count + 1;
            end if;
          end if;
          
        when others => null;
      end case;
    end procedure WriteOut;


    
    procedure Idct is
      variable adjust: integer range 0 to 8;
      variable adjidx : integer range 0 to 8;
      variable shift : integer range 0 to 4;
    begin
      if (row_loop = '1') then
        adjust := 0;
        shift := 0;
        adjidx := 1;
      else
        adjust := 8;
        shift := 4;
        adjidx := 8;
      end if;

      case idct_state is
        when idct_st1 =>
          idct_state <= idct_st2;
          mem_raddr1 <= to_unsigned(1*adjidx + count,6);
          mem_raddr2 <= to_unsigned(7*adjidx + count,6);
        when idct_st2 =>
          idct_state <= idct_st3;
          mem_raddr1 <= to_unsigned(3*adjidx + count,6);
          mem_raddr2 <= to_unsigned(5*adjidx + count,6);
        when idct_st3 =>
          idct_state <= idct_st4;
          s_A <= "*"(xC1S7,mem_rdata1)(31 downto 16) + "*"(xC7S1,mem_rdata2)(31 downto 16);
          s_B <= "*"(xC7S1,mem_rdata1)(31 downto 16) - "*"(xC1S7,mem_rdata2)(31 downto 16);
          mem_raddr1 <= to_unsigned(3*adjidx + count,6);
          mem_raddr2 <= to_unsigned(5*adjidx + count,6);

        when idct_st4 =>
          idct_state <= idct_st5;
          s_C <= "*"(xC3S5,mem_rdata1)(31 downto 16) + "*"(xC5S3,mem_rdata2)(31 downto 16);
          s_D <= "*"(xC3S5,mem_rdata2)(31 downto 16) - "*"(xC5S3,mem_rdata1)(31 downto 16);
          mem_raddr1 <= to_unsigned(0*adjidx + count,6);
          mem_raddr2 <= to_unsigned(4*adjidx + count,6);
        when idct_st5 =>
          idct_state <= idct_st6;
          s_Ad <= "*"(xC4S4,(s_A - s_C))(31 downto 16);
          s_Bd <= "*"(xC4S4,(s_B - s_D))(31 downto 16);
          s_Cd <= s_A + s_C;
          s_Dd <= s_B + s_D;
          mem_raddr1 <= to_unsigned(2*adjidx + count,6);
          mem_raddr2 <= to_unsigned(6*adjidx + count,6);

        when idct_st6 =>
          idct_state <= idct_st7;
          s_E <= "*"(xC4S4,(mem_rdata1 + mem_rdata2))(31 downto 16);
          s_F <= "*"(xC4S4,(mem_rdata1 - mem_rdata2))(31 downto 16);
          
        when idct_st7 =>
          idct_state <= idct_st8;
          s_G <= "*"(xC2S6,mem_rdata1)(31 downto 16) + "*"(xC6S2,mem_rdata2)(31 downto 16);
          s_H <= "*"(xC6S2,mem_rdata1)(31 downto 16) - "*"(xC2S6,mem_rdata2)(31 downto 16);

        when idct_st8 =>
          idct_state <= idct_st9;
          s_Ed <= s_E - s_G + adjust;
          s_Gd <= s_E + s_G + adjust;
          s_Add <= s_F + s_Ad + adjust;
          s_Fd <= s_F - s_Ad + adjust;
          s_Bdd <= s_Bd - s_H;
          s_Hd <= s_Bd + s_H;
          
        when idct_st9 =>
          idct_state <= idct_st10;
          mem_waddr <= to_unsigned(0*adjidx + count,6);
          mem_wdata <= shift_right(s_Gd + s_Cd,shift);
          mem_we <= '1';
          
        when idct_st10 =>
          idct_state <= idct_st11;
          mem_waddr <= to_unsigned(7*adjidx + count,6);
          mem_wdata <= shift_right(s_Gd - s_Cd,shift);
          mem_we <= '1';
          
        when idct_st11 =>
          idct_state <= idct_st12;
          mem_waddr <= to_unsigned(1*adjidx + count,6);
          mem_wdata <= shift_right(s_Add + s_Hd,shift);
          mem_we <= '1';
          
        when idct_st12 =>
          idct_state <= idct_st13;
          mem_waddr <= to_unsigned(2*adjidx + count,6);
          mem_wdata <= shift_right(s_Add - s_Hd,shift);
          mem_we <= '1';
          
        when idct_st13 =>
          idct_state <= idct_st14;
          mem_waddr <= to_unsigned(3*adjidx + count,6);
          mem_wdata <= shift_right(s_Ed + s_Dd,shift);
          mem_we <= '1';
          
        when idct_st14 =>
          idct_state <= idct_st15;
          mem_waddr <= to_unsigned(4*adjidx + count,6);
          mem_wdata <= shift_right(s_Ed - s_Dd,shift);
          mem_we <= '1';
          
        when idct_st15 =>
          idct_state <= idct_st16;
          mem_waddr <= to_unsigned(5*adjidx + count,6);
          mem_wdata <= shift_right(s_Fd + s_Bdd,shift);
          mem_we <= '1';
          
        when idct_st16 =>
          idct_state <= idct_st1;
          mem_waddr <= to_unsigned(6*adjidx + count,6);
          mem_wdata <= shift_right(s_Fd - s_Bdd,shift);
          mem_we <= '1';

          if (row_loop = '1') then
            if ( count = 56 ) then
              count <= 0;
              row_loop <= '0';
            else
              count <= count + 8;
            end if;
          else
            if ( count = 7 ) then
              count <= 0;
              row_loop <= '1';
              state <= writeOut;
            else
              count <= count + 1;
            end if;
          end if;
        when others => null;
      end case;
    end procedure Idct;
    
    
  begin
    
    if(clk'event and clk = '1') then
      if( Reset_n = '0' ) then
        state <= readIn;
        idct_state <= idct_st1;
        wout_state <= wout_st1;
        s_in_request <= '0';
        count <= 0;
        s_out_valid <= '0';
        row_loop <= '1';
        mem_we <= '0';

        mem_waddr <= "000000";
        mem_wdata <= "0000000000000000";
        mem_raddr1 <= "000000";
        mem_raddr2 <= "000000";
      else
        mem_we <= '0';
        case state is
          when readIn => ReadIn;
          when idct => Idct;
          when writeOut => WriteOut;
          when others => ReadIn; state <= readIn;
        end case;  
      end if;
     end if;
  end process;

end rtl;
