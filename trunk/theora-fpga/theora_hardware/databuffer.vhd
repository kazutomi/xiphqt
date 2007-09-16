-------------------------------------------------------------------------------
--  Description: This file implements a big buffer to keep
--               the roconstructed frames (This, Golden and Last)
-------------------------------------------------------------------------------

library std;
library ieee;
library work;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.all;

entity DataBuffer is

  port (Clk,
        Reset_n       :       in std_logic;

        in_request    :       out std_logic;
        in_valid      :       in std_logic;
        in_addr       :       in unsigned(19 downto 0);
        in_data       :       in signed(31 downto 0);

        
        out_requested :       in std_logic;
        out_valid     :       out std_logic;
        out_addr      :       in unsigned(19 downto 0);
        out_data      :       out signed(31 downto 0)
        );
end DataBuffer;


architecture a_DataBuffer of DataBuffer is
  component tsyncram
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
      rd_addr : in unsigned(ADDR_WIDTH-1 downto 0);
      rd_data : out signed(DATA_WIDTH-1 downto 0)
      );
  end component;


  signal count : integer;
-- Handshake
  signal s_in_request : std_logic;
  signal s_out_valid : std_logic;

  constant MEM_DEPTH : natural := 16384;
  constant MEM_DATA_WIDTH : natural := 32;
  constant MEM_ADDR_WIDTH : natural := 20;
  
  signal mem_wr_e     : std_logic;
  signal mem_wr_addr  : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal mem_wr_data  : signed(MEM_DATA_WIDTH-1 downto 0);
  signal mem_rd_addr  : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal mem_rd_data  : signed(MEM_DATA_WIDTH-1 downto 0);

begin  -- a_DataBuffer
  in_request <= s_in_request;
  out_valid <= s_out_valid;
  
  mem_int32: tsyncram
    generic map (MEM_DEPTH, MEM_DATA_WIDTH, MEM_ADDR_WIDTH)
    port map (clk, mem_wr_e, mem_wr_addr, mem_wr_data,
              mem_rd_addr, mem_rd_data);

  process (clk)

  begin  -- process
    
    if (clk'event and clk = '1') then
      if (Reset_n = '0') then
        s_in_request <= '0';
        s_out_valid <= '0';

        count <= 0;
--memory's signals
        mem_wr_e <= '0';
        mem_wr_addr <= x"00000";
        mem_wr_data <= x"00000000";
        mem_rd_addr <= x"00000";
      else

        s_out_valid <= '0';
        s_in_request <= '1';
        mem_wr_e <= '0';
        if (s_in_request = '1' and in_valid = '1') then
          mem_wr_e <= '1';
          mem_wr_data <= in_data;
          mem_wr_addr <= in_addr;
        end if;
        count <= 0;
        if (out_requested = '1' and s_out_valid = '0') then
          if (count = 0) then
            mem_rd_addr <= out_addr;
            count <= count + 1;
          elsif (count = 1) then
            count <= count + 1;
          else
            out_data <= mem_rd_data;
            s_out_valid <= '1';
            count <= 0;
          end if;
        end if;
      end if;
    end if;
  end process;

end a_DataBuffer;
