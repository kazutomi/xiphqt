library std;
library ieee;
library work;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.all;

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
  component ExpandBlock
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

      in_newframe       : in  std_logic;
      out_done          : out std_logic);
  end component;

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

  signal s_out_done : std_logic;
-------------------------------------------------------------------------------
-- States and sub-states
-------------------------------------------------------------------------------
  type state_t is (stt_Read, stt_Proc);
  signal state : state_t;
  
  type read_state_t is (stt_read_HFragments,
                        stt_read_YPlaneFragments,
                        stt_read_YStride,
                        stt_read_UVPlaneFragments,
                        stt_read_UVStride,
                        stt_read_VFragments,
                        stt_read_ReconYDataOffset,
                        stt_read_ReconUDataOffset,
                        stt_read_ReconVDataOffset,
                        stt_read_QuantDispFrags,
                        stt_read_Others);
  signal read_state : read_state_t;

begin  -- a_ReconFrames

  expandblock0: expandblock
    port map(
      Clk               => clk,  
      Reset_n           => reset_n,
      Enable            => eb_enable,
      
      in_request        => out_eb_request,
      in_valid          => out_eb_valid,
      in_data           => out_eb_data,

      in_sem_request    => out_eb_DtBuf_request,
      in_sem_valid      => out_eb_DtBuf_valid,
      in_sem_addr       => out_eb_DtBuf_addr,
      in_sem_data       => out_eb_DtBuf_data,

      out_sem_requested => in_eb_DtBuf_request,
      out_sem_valid     => in_eb_DtBuf_valid,
      out_sem_addr      => in_eb_DtBuf_addr,
      out_sem_data      => in_eb_DtBuf_data,

      in_newframe       => s_out_done,
      out_done          => eb_done
    );

  in_sem_request      <= out_eb_DtBuf_request;
  out_eb_DtBuf_valid  <= in_sem_valid;
  in_sem_addr         <= out_eb_DtBuf_addr;
  out_eb_DtBuf_data   <= in_sem_data;

  in_eb_DtBuf_request <= out_sem_requested;
  out_sem_valid       <= in_eb_DtBuf_valid;
  out_sem_addr        <= in_eb_DtBuf_addr;
  out_sem_data        <= in_eb_DtBuf_data;
  out_eb_data         <= in_data;
  in_request          <= s_in_request;

  
  -----------------------------------------------------------------------------
  -- Put the s_out_done signal on the output port
  -----------------------------------------------------------------------------
  out_done <= s_out_done;
  
  -----------------------------------------------------------------------------
  -- Switch the in_request
  -----------------------------------------------------------------------------
  process(read_state, out_eb_request, in_valid, Enable)
  begin
    s_in_request <= out_eb_request;
    out_eb_valid <= in_valid;
    if (read_state = stt_read_QuantDispFrags) then
      s_in_request <= '1';
      out_eb_valid <= '0';
    end if;
    if (Enable = '0') then
      s_in_request <= '0';
      out_eb_valid <= '0';
    end if;
  end process;


  process(clk)
    variable QuantDispFragsIsZero : std_logic;
  begin
   
    if (clk'event and clk = '1') then
      if (Reset_n = '0') then
        s_out_done <= '0';
        count <= 0;
        countExpand <= to_unsigned(0, LG_MAX_SIZE*2);
        eb_enable <= '1';
        QuantDispFrags <= to_unsigned(0, LG_MAX_SIZE*2);
        read_state <= stt_read_HFragments;
      else
        s_out_done <= '0';
        out_eb_done <= '0';
        if (Enable = '1') then
          case state is

            when stt_Read =>

--              assert false report "read_state = "&read_state_t'image(read_state) severity note;

              if (s_in_request = '1' and in_valid = '1') then
--              assert false report "rf.in_data = "&integer'image(to_integer(in_data)) severity note;
                count <= count + 1;
                case read_state is
                  when stt_read_HFragments =>
                    -- Count = 0
                    read_state <= stt_read_YPlaneFragments;

                  when stt_read_YPlaneFragments =>
                    -- Count = 1
                    read_state <= stt_read_YStride;

                  when stt_read_YStride =>
                    -- Count = 2
                    read_state <= stt_read_UVPlaneFragments;

                  when stt_read_UVPlaneFragments =>
                    -- Count = 3
                    read_state <= stt_read_UVStride;

                  when stt_read_UVStride =>
                    -- Count = 4
                    read_state <= stt_read_VFragments;

                  when stt_read_VFragments =>
                    -- Count = 5
                    read_state <= stt_read_ReconYDataOffset;

                  when stt_read_ReconYDataOffset =>
                    -- Count = 6
                    read_state <= stt_read_ReconUDataOffset;

                  when stt_read_ReconUDataOffset =>
                    -- Count = 7
                    read_state <= stt_read_ReconVDataOffset;

                  when stt_read_ReconVDataOffset =>
                    -- Count = 8
                    read_state <= stt_read_QuantDispFrags;

                  when stt_read_QuantDispFrags =>
                    -- Count = 9
                    -- One per Frame
                    -- QuantDispFrags is equal to pbi->CodedBlockIndex of the software
                    read_state <= stt_read_Others;
                    QuantDispFrags <= unsigned(in_data(LG_MAX_SIZE*2-1 downto 0));
                    if (in_data(LG_MAX_SIZE*2-1 downto 0) = 0) then
                      state <= stt_Proc;
                    end if;
                  when others =>
                    -----------------------------------------------------------
                    -- Forward to ExpandBlock the parameters below that are
                    -- received only one time pre frame
                    -----------------------------------------------------------
                    -- For Count = 10 to Count = 73 receive the
                    -- pbi->dequant_Y_coeffs matrix
                    -----------------------------------------------------------
                    -- For Count = 74 to Count = 137 receive the
                    -- pbi->dequant_U_coeffs matrix
                    -----------------------------------------------------------
                    -- For Count = 138 to Count = 201 receive the
                    -- pbi->dequant_V_coeffs matrix
                    -----------------------------------------------------------
                    -- For Count = 202 to Count = 265 receive the
                    -- dequant_InterY_coeffs matrix
                    -----------------------------------------------------------
                    -- For Count = 266 to Count = 329 receive the
                    -- dequant_InterU_coeffs matrix
                    -----------------------------------------------------------
                    -- For Count = 330 to Count = 393 receive the
                    -- dequant_InterV_coeffs matrix
                    -----------------------------------------------------------
                    -- If Count = 394 receive the pbi->FrameType value
                    -----------------------------------------------------------
                    -- If Count = 395 receive the
                    -- Offset of the GoldenFrame Buffer
                    -----------------------------------------------------------
                    -- If Count = 396 receive the
                    -- Offset of the LastFrame Buffer
                    -----------------------------------------------------------
                    -- If Count = 397 receive the
                    -- Offset of the ThisFrame Buffer
                    -----------------------------------------------------------

                    -----------------------------------------------------------
                    -- Forward to ExpandBlock the parameters below that are
                    -- received for all fragments
                    -----------------------------------------------------------
                    -- For Count = 398 to Count = 461 receive the
                    -- pbi->QFragData(number of the fragment to be expanded)
                    -- matrix
                    ------------------------------------------------------------
                    -- If Count = 462 receive the
                    -- pbi->FragCodingMethod(number of the fragment to be expanded)
                    -- value
                    -----------------------------------------------------------
                    -- If Count = 463 receive the
                    -- pbi->FragCoefEOB(number of the fragment to be expanded)
                    -- value
                    -----------------------------------------------------------
                    -- If Count = 464 receive the
                    -- (pbi->FragMVect(number of the fragment to be expanded)).x
                    -- value
                    -----------------------------------------------------------
                    -- If Count = 465 receive the
                    -- (pbi->FragMVect(number of the fragment to be expanded)).y
                    -- value
                    -----------------------------------------------------------
                    -- If Count = 466 receive the
                    -- (number of fragment to be expanded)
                    -----------------------------------------------------------
                    if (count = 466) then
                      state <= stt_Proc;
                      count <= 398;
                    end if;
                end case;
              end if;

            when stt_Proc =>
              if (QuantDispFrags = 0) then
                QuantDispFragsIsZero := '1';
              else
                QuantDispFragsIsZero := '0';
              end if;
              
              if (eb_done = '1' or QuantDispFragsIsZero = '1') then
                out_eb_done <= '1';
                countExpand <= countExpand + 1;
                state <= stt_Read;
                if (countExpand = TO_INTEGER(QuantDispFrags-1) or QuantDispFragsIsZero = '1') then
                  count <= 9;
                  read_state <= stt_read_QuantDispFrags;
                  countExpand <= to_unsigned(0, LG_MAX_SIZE*2);
                  s_out_done <= '1';
                end if;
              end if;

          end case;
        end if;
      end if;
    end if;
  end process;                   
end a_ReconFrames;
