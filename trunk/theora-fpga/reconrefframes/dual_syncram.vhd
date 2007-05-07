-------------------------------------------------------------------------------
--  Description: This file implements a dual-SRAM
-------------------------------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;


-- This entity will infferr two identical block rams
-- to permit two reads in the same clock cicle.

entity dual_syncram is
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
end entity dual_syncram;

architecture rtl of dual_syncram is

  type MEM_TYPE is array(0 to DEPTH-1) of
    signed(DATA_WIDTH-1 downto 0);
  signal memory : MEM_TYPE;
begin

  process( clk )
  begin
    if ( rising_edge(clk) ) then
      if ( wr_e = '1' ) then
        memory( to_integer(wr_addr) ) <= wr_data;
      end if;
      rd1_data <= memory( to_integer(rd1_addr) );
      rd2_data <= memory( to_integer(rd2_addr) );
    end if;
  end process;

end rtl;
