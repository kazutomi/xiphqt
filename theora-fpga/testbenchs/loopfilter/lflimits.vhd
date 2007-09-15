library std;
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity LFLimits is
  
  port (
    parameter : in  unsigned(8 downto 0);
    FLimit    : in  signed(8 downto 0);

    fbv_value : out signed(9 downto 0));

end LFLimits;

architecture a_LFLimits of LFLimits is

begin  -- a_LFLimits

  fbv_value <=
    "0000000000"
         when ((parameter <= 256 - unsigned(2*('0' & FLimit))) or
               (parameter >= 256 + unsigned(2*('0' & FLimit)))) else
    ('0' & signed(parameter)) - "0100000000"
         when ((parameter > 256 - unsigned(FLimit)) and
               (parameter < 256 + unsigned(FLimit))) else
    resize(256 - 2*('0' & FLimit) - ('0' & signed(parameter)), 10)
         when ((parameter > 256 - unsigned(2*('0' & FLimit))) and
               (parameter <= 256 - unsigned(FLimit))) else
    resize(256 + 2*('0' & FLimit) -('0' & signed(parameter)), 10);
end a_LFLimits;
