library std;
library ieee;
library work;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.all;


entity divider is
  generic (           
    WIDTH : positive := 32);
  port (Clk,
        Reset_n       : in  std_logic;
        
        in_request    : out std_logic;
        in_valid      : in  std_logic;
        dividend      : in  unsigned(WIDTH-1 downto 0);
        divisor       : in  unsigned(WIDTH-1 downto 0);
        
        out_requested : in  std_logic;
        out_valid     : out std_logic;
        quotient      : out unsigned(WIDTH-1 downto 0);
        remainder     : out unsigned(WIDTH-1 downto 0)
        );
end entity divider;

architecture a_divider of divider is
  type state_t is (stt_readIn, stt_divide, stt_writeOut);
  signal state : state_t;


  signal s_divisor   :   unsigned(WIDTH*2-1 downto 0);
  signal s_quotient  :   unsigned(WIDTH-1 downto 0);
  signal s_remainder :   unsigned(WIDTH*2-1 downto 0);

  signal s_in_request : std_logic;
  signal s_out_valid  : std_logic;
  signal s_repetition : integer range 0 to WIDTH+1;

begin  -- a_divider

  in_request <= s_in_request;
  out_valid <= s_out_valid;

  process (clk)

    procedure ReadIn is
    begin
      s_out_valid <= '0';            -- came from WriteOut, out_valid must be 0
      s_in_request <= '1';
      if( s_in_request = '1' and in_valid = '1' )then
        s_remainder <= resize("00", WIDTH) & dividend;
        s_divisor <= divisor & resize("00", WIDTH);
        s_quotient <= resize("00", WIDTH);
        s_repetition <= 0;
        s_in_request <= '0';
        state <= stt_divide;
      end if;
    end procedure ReadIn;

    procedure Divide is
      variable v_subtractor : unsigned(WIDTH*2-1 downto 0);
    begin
      v_subtractor := s_remainder - s_divisor;

      s_divisor <= SHIFT_RIGHT(s_divisor, 1);
      s_quotient <= SHIFT_LEFT(s_quotient, 1);
      if (v_subtractor(WIDTH*2-1) = '0') then  -- positive
        s_quotient(0) <= '1';
        s_remainder <= v_subtractor;
      else
        s_quotient(0) <= '0';
      end if;
      s_repetition <= s_repetition + 1;
      if (s_repetition = WIDTH) then
        state <= stt_writeOut;
        s_repetition <= 0;
      end if;
    end procedure Divide;


    procedure WriteOut is
    begin
      s_out_valid <= '1';
      quotient <= s_quotient;
      remainder <= s_remainder(WIDTH-1 downto 0);
      if (out_requested = '1') then
        state <= stt_readIn;
      end if;
    end procedure WriteOut;

  begin 
    if (clk'event and clk = '1') then
      if (Reset_n = '0') then
        s_in_request <= '0';
        s_out_valid <= '0';
        
        s_repetition <= 0;
        state <= stt_readIn;
        
      else
        case state is
          when stt_readIn => ReadIn;
          when stt_divide => Divide;
          when stt_writeOut => WriteOut;
          when others => ReadIn; state <= stt_readIn;
        end case;
      end if;
    end if;
  end process;

end a_divider;
