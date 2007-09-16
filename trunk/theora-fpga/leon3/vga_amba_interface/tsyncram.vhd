library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity tsyncram is
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
end entity tsyncram;

architecture rtl of tsyncram is

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
      rd_data <= memory( to_integer(rd_addr) );
    end if;
  end process;

end rtl;
