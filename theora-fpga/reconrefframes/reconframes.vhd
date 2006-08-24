library std;
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity ReconFrames is
    port (
    Clk,
    Reset_n           : in  std_logic;
    Enable            : in  std_logic;
    
    in_request        : out std_logic;
    in_valid          : in  std_logic;
    in_data           : in  signed(31 downto 0);

    in_sem_request    : out std_logic;
    in_sem_valid      : in  std_logic;
    in_sem_addr       : out unsigned(19 downto 0);
    in_sem_data       : in  signed(31 downto 0);

    out_sem_requested : in  std_logic;
    out_sem_valid     : out std_logic;
    out_sem_addr      : out unsigned(19 downto 0);
    out_sem_data      : out signed(31 downto 0);

    out_done          : out std_logic;
    out_eb_done       : out std_logic);
end ReconFrames;

architecture a_ReconFrames of ReconFrames is
  constant LG_MAX_SIZE    : natural := 10;
  constant MEM_ADDR_WIDTH : natural := 20;
-------------------------------------------------------------------------------
-- ExpandBlock's signals
-------------------------------------------------------------------------------
  signal out_eb_request : std_logic;
  signal out_eb_valid : std_logic := '0';
  signal out_eb_data : signed(31 downto 0);

  signal in_eb_DtBuf_request    : std_logic;
  signal in_eb_DtBuf_valid      : std_logic;
  signal in_eb_DtBuf_addr       : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal in_eb_DtBuf_data       : signed(31 downto 0);

  signal out_eb_DtBuf_request   : std_logic;
  signal out_eb_DtBuf_valid     : std_logic;
  signal out_eb_DtBuf_addr      : unsigned(19 downto 0);
  signal out_eb_DtBuf_data      : signed(31 downto 0);

  signal eb_done                : std_logic;
  signal eb_enable : std_logic;

-------------------------------------------------------------------------------
-- Internal signals
-------------------------------------------------------------------------------
  signal QuantDispFrags : unsigned(LG_MAX_SIZE*2-1 downto 0);
  
  signal count : integer;
  signal countExpand : unsigned(LG_MAX_SIZE*2-1 downto 0);
  signal s_in_request : std_logic;

  signal s_in_valid : std_logic;
-------------------------------------------------------------------------------
-- States and sub-states
-------------------------------------------------------------------------------
  type state_t is (stt_Read, stt_Proc);
  signal state : state_t;
  
  type read_state_t is (stt_read_Others, stt_read_QuantDispFrags);
  signal read_state : read_state_t;

begin  -- a_ReconFrames

  expandblock0: entity work.expandblock
    port map(clk, reset_n, eb_enable,
             out_eb_request, out_eb_valid, out_eb_data,
             out_eb_DtBuf_request, out_eb_DtBuf_valid, out_eb_DtBuf_addr, out_eb_DtBuf_data,
             in_eb_DtBuf_request, in_eb_DtBuf_valid, in_eb_DtBuf_addr, in_eb_DtBuf_data,
             eb_done
             );

  in_sem_request    <= out_eb_DtBuf_request;
  out_eb_DtBuf_valid <= in_sem_valid;
  in_sem_addr       <= out_eb_DtBuf_addr;
  out_eb_DtBuf_data <= in_sem_data;

  in_eb_DtBuf_request <= out_sem_requested;
  out_sem_valid     <= in_eb_DtBuf_valid;
  out_sem_addr      <= in_eb_DtBuf_addr;
  out_sem_data      <= in_eb_DtBuf_data;
  out_eb_data       <= in_data;
  in_request <= s_in_request;


-----------------------------------------------------------------------------
  -- Switch the in_request
  -----------------------------------------------------------------------------
  process(read_state, out_eb_request, in_valid, s_in_valid)
  begin
    s_in_request <= out_eb_request;
    out_eb_valid <= in_valid;
    if (read_state = stt_read_QuantDispFrags) then
      s_in_request <= '1';
      out_eb_valid <= s_in_valid;
    end if;
  end process;


  
  process(clk)
  begin
    if (Reset_n = '0') then
      count <= 0;
      countExpand <= to_unsigned(0, LG_MAX_SIZE*2);
      eb_enable <= '1';
      QuantDispFrags <= to_unsigned(0, LG_MAX_SIZE*2);
      read_state <= stt_read_Others;
      
    elsif (clk'event and clk = '1') then
      out_done <= '0';
      out_eb_done <= '0';
      if (Enable = '1') then
        case state is

          when stt_Read =>

--            assert false report "rf.in_valid = "&std_logic'image(in_valid) severity note;

            if (s_in_request = '1' and in_valid = '1') then
--              assert false report "rf.in_data = "&integer'image(to_integer(in_data)) severity note;
              count <= count + 1;
              if (count = 8) then
                read_state <= stt_read_QuantDispFrags;
                s_in_valid <= '0';

              elsif (read_state = stt_read_QuantDispFrags) then
--                assert false report "rf.QuantDispFrags = "&integer'image(to_integer(in_data)) severity note;
                -- One per Frame
                read_state <= stt_read_Others;
                QuantDispFrags <= unsigned(in_data(LG_MAX_SIZE*2-1 downto 0));
              elsif (count = 466) then
                state <= stt_Proc;
                count <= 10;
              end if;
            end if;


          when stt_Proc =>
            if (eb_done = '1') then
              out_eb_done <= '1';
              if ((to_integer(countExpand) mod 100) = 0) then
--                assert false report "Fragmento = "&integer'image(to_integer(countExpand)) severity note;
              end if;
              countExpand <= countExpand + 1;
              state <= stt_Read;
              if (countExpand = TO_INTEGER(QuantDispFrags-1)) then
                countExpand <= to_unsigned(0, LG_MAX_SIZE*2);
                out_done <= '1';

                s_in_valid <= '0';
                count <= 9;
                read_state <= stt_read_QuantDispFrags;
              end if;
            end if;
            
          when others => null;

        end case;
      end if;
    end if;
  end process;                   
end a_ReconFrames;
