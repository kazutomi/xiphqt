-- Theora AMBA Interface
-- Tested on LEON3
-- André Costa - andre.lnc@gmail.com

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
library grlib;
use grlib.amba.all;
use grlib.stdlib.all;
use grlib.devices.all;


library opencores;
use opencores.theora_hardware.all;

entity theora_amba_interface is
  generic(
  --  memtech     : integer := DEFMEMTECH;
    pindex      : integer := 0; 
    paddr       : integer := 0;
    pmask       : integer := 16#fff#
    );
  port (
    rst   : in  std_ulogic;
    clk   : in  std_ulogic;
    apbi   : in  apb_slv_in_type;
    apbo   : out apb_slv_out_type
--    irq   : out std_logic
  );
end entity theora_amba_interface;

architecture rtl of theora_amba_interface is

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
  
--constant REVISION       : amba_version_type := 0; 
constant pconfig        : apb_config_type := (
                        0 => ahb_device_reg ( VENDOR_OPENCORES, OPENCORES_THEORA_HARDWARE, 0, 0, 0),
                        1 => apb_iobar(paddr, pmask));

  --signal r, rin : std_logic;
   signal rdata : std_logic_vector(31 downto 0);
begin

  recon1 : ReconRefFrames
  port map ( clk, rst, in_request, in_valid, in_data, out_requested, out_valid, out_data);
 
  reg : process(rst, clk)
  begin 
    if (rst = '0') then
    rdata <= (others => '0');
    out_requested <= '0';
    in_valid <= '0';
    in_data <= (others => '0');
  elsif rising_edge(clk) then 

  rdata <= (others => '0');             -- init
  out_requested <= '0';
  in_valid <= '0';
  in_data <= (others => '0');
    
-- APB BUS want Read?
 if (apbi.psel(pindex)) = '1' then
  case apbi.paddr(4 downto 2) is
    when "000" =>  -- Can software write data to Theora Hardware?
      rdata(0) <= in_request;
    when "010" => -- Can software read data from Theora Hardware?
      rdata(0) <= out_valid;
    when others => -- Read data from Theora Hardware
      out_requested <= '1';
      rdata <= std_logic_vector(out_data);
  end case;
 end if;
   
-- APB BUS want Write?
  if (apbi.psel(pindex) and apbi.penable and apbi.pwrite) = '1' then
      in_valid <= '1';
      in_data <= signed(apbi.pwdata(31 downto 0)); -- a:=pwdata
  end if;
  end if;  
  end process;

    apbo.prdata <= rdata;
    apbo.pindex <= pindex;
    apbo.pirq <= (others => '0');
    apbo.pconfig <= pconfig;
    
   -- pragma translate_off   
   bootmsg : report_version 
   generic map ("apbvgreport_versiona" & tost(pindex) & ": APB THEORA HARDWARE module rev 0");
   -- pragma translate_on
   
end rtl;
