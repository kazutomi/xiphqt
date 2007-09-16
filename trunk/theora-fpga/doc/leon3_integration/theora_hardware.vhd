
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

package theora_hardware is
  component theora_amba_interface is
  generic(
    pindex      : integer := 0; 
    paddr       : integer := 0;
    pmask       : integer := 16#fff#);
  port (
    rst   : in  std_ulogic;
    clk   : in  std_ulogic;
    apbi   : in  apb_slv_in_type;
    apbo   : out apb_slv_out_type
--    irq   : out std_logic
  );
  end component theora_amba_interface; 

end;
