
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
--use ieee.std_logic_arith.all;
--use ieee.std_logic_unsigned.all;
--use ieee.std_logic_signed.all;
--library work;
--use work.all;
library grlib;
use grlib.amba.all;
use grlib.stdlib.all;

package interfacevga is
  component theora_amba_interface is
  generic(
    pindex      : integer := 0; 
    paddr       : integer := 0;
    pmask       : integer := 16#fff#);
  port (
    rst   : in  std_ulogic;
    clk   : in  std_ulogic;
    apbi   : in  apb_slv_in_type;
    apbo   : out apb_slv_out_type;
    clk_25Mhz : in std_logic;
    red	         : out std_logic_vector(7 downto 0);  -- red component
    green	 : out std_logic_vector(7 downto 0);  -- green component
    blue	 : out std_logic_vector(7 downto 0);  -- blue component
--    line_pixel   : out std_logic_vector(9 downto 0);  -- compute line
--    column_pixel : out std_logic_vector(9 downto 0);  -- compute column
    m1, m2       : out std_logic;                     -- select dac mode
    blank_n      : out std_logic;                     -- dac command
    sync_n       : out std_logic;                     -- dac command
    sync_t       : out std_logic;                     -- dac command
    video_clk    : out std_logic;                     -- dac command
    vga_vs       : out std_logic;                     -- vertical sync
    vga_hs       : out std_logic                      -- horizontal sync
    
--    irq   : out std_logic
  );
  end component theora_amba_interface; 

end;
