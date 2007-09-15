-------------------------------------------------------------------------------
--  Description: If x < 0 then sat receives 0.
--               If x > 255 then sat receives 255
--               Else sat receives the eights low-order bits. 
-------------------------------------------------------------------------------

library std;
library IEEE;
use IEEE.numeric_std.all;
use IEEE.std_logic_1164.all;

entity clamp is
  
  port (
    x   : in  SIGNED(16 downto 0);
    sat : out UNSIGNED(7 downto 0));

  -- purpose: saturate the number in x to an unsigned number till 255
end clamp;

architecture a_clamp of clamp is
begin  -- a_clamp
 
  sat <= "00000000" WHEN (x < 0) ELSE
         "11111111" WHEN (x > 255) ELSE
         unsigned(x(7 downto 0));

  
end a_clamp;
