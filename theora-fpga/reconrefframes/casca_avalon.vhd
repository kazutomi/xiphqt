library std;
library ieee;
library work;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
--use ieee.std_logic_arith.all;
--use ieee.std_logic_unsigned.all;
--use ieee.std_logic_signed.all;
use work.all;



entity casca_avalon is
  port (clk,
        reset_n : in std_logic;
        address : in std_logic_vector(1 downto 0);

        read : in std_logic;
        write : in std_logic;

        writedata : in std_logic_vector(31 downto 0);
        readdata : out std_logic_vector(31 downto 0);
        
        chipselect : in std_logic


        
        );

end entity casca_avalon;




architecture rtl of casca_avalon is
  component ReconRefFrames
    port (Clk,
          Reset_n       : in  std_logic;
          
          in_request    : out std_logic;
          in_valid      : in  std_logic;
          in_data       : in  signed(31 downto 0);
          
          out_requested : in  std_logic;
          out_valid     : out std_logic;
          out_data      : out signed(31 downto 0)

          );
  end component;
  
  signal in_request : std_logic;
  signal in_valid : std_logic;
  signal in_data : signed(31 downto 0);

  signal out_requested : std_logic;
  signal out_valid : std_logic;
  signal out_data : signed(31 downto 0);

begin

  recon1 : ReconRefFrames
    port map ( clk, Reset_n, in_request, in_valid, in_data, out_requested, out_valid, out_data);

   process(chipselect, read, address, in_request, out_valid, out_data)
   begin
--     out_requested <= '0';
     readdata <= "00000000000000000000000000000000";

     out_requested <= '0';

     if (chipselect = '1') then
       if (read = '1') then
         case address is
           when "00" => -- Can software write data to IDCT Module ?
             readdata <= "0000000000000000000000000000000"&in_request;
                 
           when "01" => -- Can software read data from IDCT Module ?
             readdata <= "0000000000000000000000000000000"&out_valid;

           when others => -- Read data from IDCT Module ?
             out_requested <= '1';
             readdata <= std_logic_vector(out_data);
         end case;
       end if;
     end if;

     if (Reset_n = '0') then
       readdata <= "00000000000000000000000000000000";
       out_requested <= '0';
     end if;
   end process;


  process(Reset_n, chipselect, write, writedata)
   begin

     in_valid <= '0';
     in_data <= "00000000000000000000000000000000";
     if (chipselect = '1') then
       if (write = '1') then
         in_data <= signed(writedata(31 downto 0));
         in_valid <= '1';
       end if;
     end if;
     if (Reset_n = '0') then
       in_valid <= '0';
       in_data <= "00000000000000000000000000000000";
     end if;
   end process;
end rtl;
