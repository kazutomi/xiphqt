library std;
library ieee;
library work;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.all;


entity ReconRefFrames is
  port (Clk,
        Reset_n       : in  std_logic;
        
        in_request    : out std_logic;
        in_valid      : in  std_logic;
        in_data       : in  signed(31 downto 0);
        
        out_requested : in  std_logic;
        out_valid     : out std_logic;
        out_data      : out signed(31 downto 0)
        );
end entity ReconRefFrames;

architecture a_ReconRefFrames of ReconRefFrames is
  component ReconFrames
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
  end component;


  component LoopFilter
    port (Clk,
          Reset_n       :       in std_logic;
          Enable        :       in std_logic;
          
          in_request    :       out std_logic;
          in_valid      :       in std_logic;
          in_data       :       in signed(31 downto 0);

          in_sem_request    :   out std_logic;
          in_sem_valid      :   in  std_logic;
          in_sem_addr       :   out unsigned(19 downto 0);
          in_sem_data       :   in  signed(31 downto 0);

          out_sem_requested :   in  std_logic;
          out_sem_valid     :   out std_logic;
          out_sem_addr      :   out unsigned(19 downto 0);
          out_sem_data      :   out signed(31 downto 0);

          out_done          :   out std_logic
          );
  end component;

  component CopyRecon
    port (Clk,
          Reset_n       :       in std_logic;
          Enable        :       in std_logic;
          
          in_request    :       out std_logic;
          in_valid      :       in std_logic;
          in_data       :       in signed(31 downto 0);

          in_sem_request    :   out std_logic;
          in_sem_valid      :   in  std_logic;
          in_sem_addr       :   out unsigned(19 downto 0);
          in_sem_data       :   in  signed(31 downto 0);

          out_sem_requested :   in  std_logic;
          out_sem_valid     :   out std_logic;
          out_sem_addr      :   out unsigned(19 downto 0);
          out_sem_data      :   out signed(31 downto 0);

          out_done          :   out std_logic
          );
  end component;


  component UpdateUMV
    port (Clk,
          Reset_n           :   in std_logic;
          Enable            :   in std_logic;
          
          in_request        :   out std_logic;
          in_valid          :   in std_logic;
          in_data           :   in signed(31 downto 0);

          in_sem_request    :   out std_logic;
          in_sem_valid      :   in  std_logic;
          in_sem_addr       :   out unsigned(19 downto 0);
          in_sem_data       :   in  signed(31 downto 0);

          out_sem_requested :   in  std_logic;
          out_sem_valid     :   out std_logic;
          out_sem_addr      :   out unsigned(19 downto 0);
          out_sem_data      :   out signed(31 downto 0);

          out_done          :   out std_logic
          );
  end component;

  component DataBuffer
    port (Clk,
          Reset_n       :       in std_logic;

          in_request    :       out std_logic;
          in_valid      :       in std_logic;
          in_addr       :       in unsigned(19 downto 0);
          in_data       :       in signed(31 downto 0);

          
          out_requested :       in std_logic;
          out_valid     :       out std_logic;
          out_addr      :       in unsigned(19 downto 0);
          out_data      :       out signed(31 downto 0)
          );
  end component;

  constant LG_MAX_SIZE    : natural := 10;
  constant MEM_ADDR_WIDTH : natural := 20;
-------------------------------------------------------------------------------
-- Buffer's signals
-------------------------------------------------------------------------------
  signal in_DtBuf_request : std_logic;
  signal in_DtBuf_valid   : std_logic;
  signal in_DtBuf_addr    : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal in_DtBuf_data    : signed(31 downto 0);

  signal out_DtBuf_request : std_logic;
  signal out_DtBuf_valid   : std_logic;
  signal out_DtBuf_addr    : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal out_DtBuf_data    : signed(31 downto 0);

-------------------------------------------------------------------------------
-- ReconFrames' signals
-------------------------------------------------------------------------------
  signal out_rf_request : std_logic;
  signal out_rf_valid : std_logic := '0';
  signal out_rf_data : signed(31 downto 0);

  signal in_rf_DtBuf_request    : std_logic;
  signal in_rf_DtBuf_valid      : std_logic;
  signal in_rf_DtBuf_addr       : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal in_rf_DtBuf_data       : signed(31 downto 0);

  signal out_rf_DtBuf_request   : std_logic;
  signal out_rf_DtBuf_valid     : std_logic;
  signal out_rf_DtBuf_addr      : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal out_rf_DtBuf_data      : signed(31 downto 0);

  signal rf_done                : std_logic;
  signal rf_eb_done             : std_logic;
  signal rf_enable : std_logic;
  
-------------------------------------------------------------------------------
-- CopyRecon's signals
-------------------------------------------------------------------------------
  signal out_cr_request : std_logic;
  signal out_cr_valid : std_logic := '0';
  signal out_cr_data : signed(31 downto 0);

  signal in_cr_DtBuf_request    : std_logic;
  signal in_cr_DtBuf_valid      : std_logic;
  signal in_cr_DtBuf_addr       : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal in_cr_DtBuf_data       : signed(31 downto 0);

  signal out_cr_DtBuf_request   : std_logic;
  signal out_cr_DtBuf_valid     : std_logic;
  signal out_cr_DtBuf_addr      : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal out_cr_DtBuf_data      : signed(31 downto 0);

  signal cr_done                : std_logic;
  signal cr_enable : std_logic;

-------------------------------------------------------------------------------
-- LoopFilter's signals
-------------------------------------------------------------------------------
  signal out_lf_request : std_logic;
  signal out_lf_valid : std_logic := '0';
  signal out_lf_data : signed(31 downto 0);

  signal in_lf_DtBuf_request    : std_logic;
  signal in_lf_DtBuf_valid      : std_logic;
  signal in_lf_DtBuf_addr       : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal in_lf_DtBuf_data       : signed(31 downto 0);

  signal out_lf_DtBuf_request   : std_logic;
  signal out_lf_DtBuf_valid     : std_logic;
  signal out_lf_DtBuf_addr      : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal out_lf_DtBuf_data      : signed(31 downto 0);

  signal lf_done                : std_logic;
  signal lf_enable : std_logic;

-------------------------------------------------------------------------------
-- UpdateUMV's signals
-------------------------------------------------------------------------------
  signal out_uu_request : std_logic;
  signal out_uu_valid : std_logic := '0';
  signal out_uu_data : signed(31 downto 0);

  signal in_uu_DtBuf_request    : std_logic;
  signal in_uu_DtBuf_valid      : std_logic;
  signal in_uu_DtBuf_addr       : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal in_uu_DtBuf_data       : signed(31 downto 0);

  signal out_uu_DtBuf_request   : std_logic;
  signal out_uu_DtBuf_valid     : std_logic;
  signal out_uu_DtBuf_addr      : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal out_uu_DtBuf_data      : signed(31 downto 0);

  signal uu_done                : std_logic;
  signal uu_enable : std_logic;

-------------------------------------------------------------------------------
  signal in_rr_DtBuf_request    : std_logic;
  signal in_rr_DtBuf_valid      : std_logic;
  signal in_rr_DtBuf_addr       : unsigned(MEM_ADDR_WIDTH-1 downto 0);

  signal out_rr_DtBuf_request   : std_logic;
  signal out_rr_DtBuf_valid     : std_logic;
  signal out_rr_DtBuf_addr      : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal out_rr_DtBuf_data      : signed(31 downto 0);

  
  signal FrameSize : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal GoldenFrameOfs    : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal ThisFrameReconOfs : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal LastFrameReconOfs : unsigned(MEM_ADDR_WIDTH-1 downto 0);

  signal FrameOfsAux       : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal FrameOfsAuxSrc    : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  
  signal FrameType         : unsigned(7 downto 0);
  signal MaxDPFCount       : unsigned(LG_MAX_SIZE*2 downto 0);

  signal s_in_request : std_logic;
  signal s_in_valid   : std_logic;

  signal CountCopies  : std_logic;
  signal CountUpdates : std_logic;
  signal count : integer range 0 to 2097151;

  signal count_lines   : integer range 0 to 1023;
  signal count_columns : integer range 0 to 1023;
  
  constant KEY_FRAME : unsigned(7 downto 0) := "00000000";

  type state_t is (stt_CleanBuffer,
                   stt_Forward, stt_ReconFrames,
                   stt_CopyRecon, stt_LoopFilter,
                   stt_UpdateUMV, stt_WriteOut);
  signal state : state_t;
  
  
  type forward_state_t is (stt_rec_framesize,
                           stt_rec_height,
                           stt_rec_width,
                           stt_forward_uniq_common,
                           stt_forward_uniq_cr_lf,
                           stt_forward_uniq_lf,
                           stt_forward_uniq_uu,
                           stt_forward_uniqperframe_rf,
                           stt_frametype,
                           stt_forward_golden_ofs_rf,
                           stt_forward_last_ofs_rf,
                           stt_forward_this_ofs_rf,
                           stt_forward_rf,
                           stt_forward_dispfrag,
                           stt_forward_source_ofs_cr,
                           stt_forward_dest_ofs_cr,
                           stt_forward_lf,
                           stt_forward_offset_lf,
                           stt_forward_offset_uu,
                           stt_forward_dispfrag_golden,
                           stt_forward_none);
  signal forward_state : forward_state_t;

  type write_state_t is (stt_write1, stt_write2, stt_write3);
  signal write_state : write_state_t;

  type plane_write_state_t is (stt_plane_write_Y, stt_plane_write_Cb, stt_plane_write_Cr);
  signal plane_write_state : plane_write_state_t;

-- Fragment Parameters
  signal YStride          : unsigned(LG_MAX_SIZE+1 downto 0);
  signal UVStride         : unsigned(LG_MAX_SIZE   downto 0);
  signal ReconYDataOffset : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal ReconUDataOffset : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal ReconVDataOffset : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal video_height     : unsigned(9 downto 0);
  signal video_width      : unsigned(9 downto 0);

  signal s_out_valid : std_logic;
  signal s_out_data : signed(31 downto 0);


--   signal count_leo : integer range 0 to 127 := 0;

begin  -- a_ReconRefFrames

  reconframes0: reconframes
    port map(clk, Reset_n, rf_enable,
             out_rf_request, out_rf_valid, out_rf_data,
             out_rf_DtBuf_request, out_rf_DtBuf_valid, out_rf_DtBuf_addr, out_rf_DtBuf_data,
             in_rf_DtBuf_request, in_rf_DtBuf_valid, in_rf_DtBuf_addr, in_rf_DtBuf_data,
             rf_done, rf_eb_done);

  loopfilter0: loopfilter
    port map(clk, Reset_n, rf_enable,
             out_lf_request, out_lf_valid, out_lf_data,
             out_lf_DtBuf_request, out_lf_DtBuf_valid, out_lf_DtBuf_addr, out_lf_DtBuf_data,
             in_lf_DtBuf_request, in_lf_DtBuf_valid, in_lf_DtBuf_addr, in_lf_DtBuf_data,
             lf_done);
  
  copyrecon0: copyrecon
    port map(clk, Reset_n, cr_enable,
             out_cr_request, out_cr_valid, out_cr_data,
             out_cr_DtBuf_request, out_cr_DtBuf_valid, out_cr_DtBuf_addr, out_cr_DtBuf_data,
             in_cr_DtBuf_request, in_cr_DtBuf_valid, in_cr_DtBuf_addr, in_cr_DtBuf_data,
             cr_done);

  updateumv0: UpdateUMV
    port map(clk, Reset_n, uu_enable,
             out_uu_request, out_uu_valid, out_uu_data,
             out_uu_DtBuf_request, out_uu_DtBuf_valid, out_uu_DtBuf_addr, out_uu_DtBuf_data,
             in_uu_DtBuf_request, in_uu_DtBuf_valid, in_uu_DtBuf_addr, in_uu_DtBuf_data,
             uu_done);

  databuffer0: databuffer
    port map(clk, Reset_n,
             out_DtBuf_request, out_DtBuf_valid, out_DtBuf_addr, out_DtBuf_data,
             in_DtBuf_request, in_DtBuf_valid, in_DtBuf_addr, in_DtBuf_data);


  out_valid <= s_out_valid;
  -----------------------------------------------------------------------------
  -- Switch the in_request
  -----------------------------------------------------------------------------
  -- If forward_state is a state that doesn't need external data then
  -- in_request will be turned off
  with forward_state select in_request <=
    '0' when stt_forward_golden_ofs_rf,
    '0' when stt_forward_last_ofs_rf,
    '0' when stt_forward_this_ofs_rf,
    '0' when stt_forward_source_ofs_cr,
    '0' when stt_forward_dest_ofs_cr,
    '0' when stt_forward_offset_lf,
    '0' when stt_forward_offset_uu,
    '0' when stt_forward_dispfrag_golden,
    '0' when stt_forward_none,
    s_in_request when others;
  
  -----------------------------------------------------------------------------
  -- Switch the signals of the in_data and in_valid of the modules
  -----------------------------------------------------------------------------
  process (Reset_n,
           forward_state,
           out_rf_request,
           out_cr_request,
           out_lf_request,
           out_uu_request,
           in_valid,
           in_data,
           GoldenFrameOfs,
           LastFrameReconOfs,
           ThisFrameReconOfs,
           FrameOfsAuxSrc,
           FrameOfsAux)

  begin
   
    out_rf_valid <= '0';
    out_rf_data <= in_data;
    out_cr_valid <= '0';
    out_cr_data <= in_data;
    out_lf_valid <= '0';
    out_lf_data <= in_data;
    out_uu_valid <= '0';
    out_uu_data <= in_data;

    s_in_valid <= in_valid;
    s_in_request <= '0';

    -----------------------------------------------------------------------------
    -- Unique Parameters
    -----------------------------------------------------------------------------    
    if (forward_state = stt_rec_framesize) then
      s_in_request <= '1';

    elsif (forward_state = stt_rec_height) then
      s_in_request <= '1';

    elsif (forward_state = stt_rec_width) then
      s_in_request <= '1';
      
    elsif (forward_state = stt_forward_uniq_common) then
      s_in_request <= out_rf_request and
                 out_cr_request and
                 out_lf_request and
                 out_uu_request;
      out_rf_valid <= in_valid;
      out_cr_valid <= in_valid;
      out_lf_valid <= in_valid;
      out_uu_valid <= in_valid;

    elsif (forward_state = stt_forward_uniq_cr_lf) then
      -------------------------------------------------------------------------
      -- UnitFragment is sent to CopyRecon and LoopFilter and read internaly
      -------------------------------------------------------------------------
      s_in_request <= out_cr_request and
                 out_lf_request;
      
      out_cr_valid <= in_valid;
      out_lf_valid <= in_valid;
      
    elsif (forward_state = stt_forward_uniq_lf) then
      s_in_request <= out_lf_request;
      out_lf_valid <= in_valid;

    elsif (forward_state = stt_forward_uniq_uu) then
      s_in_request <= out_uu_request;
      out_uu_valid <= in_valid;


    -----------------------------------------------------------------------
    -- ReconFrames Parameters
    ---------------------------------------------------------------------------
    elsif (forward_state = stt_forward_uniqperframe_rf) then
      s_in_request <= out_rf_request;
      out_rf_valid <= in_valid;

    elsif (forward_state = stt_frametype) then
      -------------------------------------------------------------------------
      -- FrameType is sent to ReconFrames and read internaly
      -------------------------------------------------------------------------
      s_in_request <= out_rf_request;
      out_rf_valid <= in_valid;
                   
    elsif (forward_state = stt_forward_golden_ofs_rf) then
      s_in_request <= out_rf_request;
      s_in_valid <= '1';
      out_rf_valid <= '1';
      out_rf_data <= resize('0' & signed(GoldenFrameOfs), 32);

    elsif (forward_state = stt_forward_last_ofs_rf) then
      s_in_request <= out_rf_request;
      s_in_valid <= '1';
      out_rf_valid <= '1';
      out_rf_data <= resize('0' & signed(LastFrameReconOfs), 32);

    elsif (forward_state = stt_forward_this_ofs_rf) then
      s_in_request <= out_rf_request;
      s_in_valid <= '1';
      out_rf_valid <= '1';
      out_rf_data <= resize('0' & signed(ThisFrameReconOfs), 32);

    elsif (forward_state = stt_forward_rf) then
      s_in_request <= out_rf_request;
      out_rf_valid <= in_valid;

    elsif (forward_state = stt_forward_dispfrag) then
      s_in_request <= out_cr_request and
                      out_lf_request;
      out_cr_valid <= '0';
      out_lf_valid <= '0';
      if ((out_cr_request and out_lf_request)= '1') then
        out_cr_valid <= in_valid;
        out_lf_valid <= in_valid;
      else
        assert false report "Somebody doesn't want read" severity note;
      end if;
      
    elsif (forward_state = stt_forward_source_ofs_cr) then
      s_in_request <= out_cr_request;
      s_in_valid <= '1';
      out_cr_valid <= '1';
      out_cr_data <= resize('0' & signed(FrameOfsAuxSrc), 32);
     
    elsif (forward_state = stt_forward_dest_ofs_cr) then
      s_in_request <= out_cr_request;
      s_in_valid <= '1';
      out_cr_valid <= '1';
      out_cr_data <= resize('0' & signed(FrameOfsAux), 32);
      
    elsif (forward_state = stt_forward_lf) then
      s_in_request <= out_lf_request;
      out_lf_valid <= in_valid;
      
    elsif (forward_state = stt_forward_offset_lf) then
      s_in_request <= out_lf_request;
      s_in_valid <= '1';
      out_lf_valid <= '1';
      out_lf_data <= resize('0' & signed(LastFrameReconOfs), 32);

    elsif (forward_state = stt_forward_offset_uu) then
      s_in_request <= out_uu_request;
      s_in_valid <= '1';
      out_uu_valid <= '1';
      out_uu_data <= resize('0' & signed(FrameOfsAux), 32);

    elsif (forward_state = stt_forward_dispfrag_golden) then
      -------------------------------------------------------------------------
      -- If it is a key frame then all fragments must be displayed.
      -- In such case all values of display_fragments is one
      -------------------------------------------------------------------------
      s_in_request <= out_cr_request;
      s_in_valid <= '1';
      out_cr_valid <= '1';
      out_cr_data <= x"FFFFFFFF";
    else
      null;
    end if;
    if (Reset_n = '0') then
          out_rf_valid <= '0';
          out_cr_valid <= '0';
          out_lf_valid <= '0';
          out_uu_valid <= '0';
          s_in_request <= '0';
    end if;
  end process;

  -----------------------------------------------------------------------------
  -- Control the module's access to the Data Buffer
  -- This is just a big multiplexer
  -----------------------------------------------------------------------------
  process (Reset_n,
           state,
           in_DtBuf_valid,
           in_DtBuf_data,
           out_DtBuf_request,

           out_rf_DtBuf_request,
           out_rf_DtBuf_addr,
           in_rf_DtBuf_valid,
           in_rf_DtBuf_addr,
           in_rf_DtBuf_data,

           out_cr_DtBuf_request,
           out_cr_DtBuf_addr,
           in_cr_DtBuf_valid,
           in_cr_DtBuf_addr,
           in_cr_DtBuf_data,

           out_lf_DtBuf_request,
           out_lf_DtBuf_addr,
           in_lf_DtBuf_valid,
           in_lf_DtBuf_addr,
           in_lf_DtBuf_data,

           out_uu_DtBuf_request,
           out_uu_DtBuf_addr,
           in_uu_DtBuf_valid,
           in_uu_DtBuf_addr,
           in_uu_DtBuf_data,

           out_rr_DtBuf_request,
           out_rr_DtBuf_addr,
           in_rr_DtBuf_valid,
           in_rr_DtBuf_addr
           )
  begin  -- process state
    out_rr_DtBuf_data  <= x"00000000";
    out_rr_DtBuf_valid <= '0';
    
    out_cr_DtBuf_data  <= x"00000000";
    out_cr_DtBuf_valid <= '0';
    
    out_lf_DtBuf_data  <= x"00000000";
    out_lf_DtBuf_valid <= '0';
    
    out_rf_DtBuf_data <= x"00000000";
    out_rf_DtBuf_valid <= '0';
    
    out_uu_DtBuf_data <= x"00000000";
    out_uu_DtBuf_valid <= '0';

    in_uu_DtBuf_request <= '0';
    in_cr_DtBuf_request <= '0';
    in_lf_DtBuf_request <= '0';
    in_rf_DtBuf_request <= '0';
    in_rr_DtBuf_request <= '0';
    
    if (state = stt_ReconFrames) then
      in_DtBuf_request <= out_rf_DtBuf_request;
      out_rf_DtBuf_valid <= in_DtBuf_valid;
      in_DtBuf_addr <= out_rf_DtBuf_addr;
      out_rf_DtBuf_data  <= in_DtBuf_data;

      in_rf_DtBuf_request <= out_DtBuf_request;
      out_DtBuf_valid <= in_rf_DtBuf_valid;
      out_DtBuf_addr <= in_rf_DtBuf_addr;
      out_DtBuf_data <= in_rf_DtBuf_data;
      
    elsif (state = stt_CopyRecon) then
      in_DtBuf_request <= out_cr_DtBuf_request;
      out_cr_DtBuf_valid <= in_DtBuf_valid;
      in_DtBuf_addr <= out_cr_DtBuf_addr;
      out_cr_DtBuf_data  <= in_DtBuf_data;

      in_cr_DtBuf_request <= out_DtBuf_request;
      out_DtBuf_valid <= in_cr_DtBuf_valid;
      out_DtBuf_addr <= in_cr_DtBuf_addr;
      out_DtBuf_data <= in_cr_DtBuf_data;

    elsif (state = stt_LoopFilter) then
      in_DtBuf_request <= out_lf_DtBuf_request;
      out_lf_DtBuf_valid <= in_DtBuf_valid;
      in_DtBuf_addr <= out_lf_DtBuf_addr;
      out_lf_DtBuf_data  <= in_DtBuf_data;

      in_lf_DtBuf_request <= out_DtBuf_request;
      out_DtBuf_valid <= in_lf_DtBuf_valid;
      out_DtBuf_addr <= in_lf_DtBuf_addr;
      out_DtBuf_data <= in_lf_DtBuf_data;

    elsif (state = stt_UpdateUMV) then
      in_DtBuf_request <= out_uu_DtBuf_request;
      out_uu_DtBuf_valid <= in_DtBuf_valid;
      in_DtBuf_addr <= out_uu_DtBuf_addr;
      out_uu_DtBuf_data  <= in_DtBuf_data;

      in_uu_DtBuf_request <= out_DtBuf_request;
      out_DtBuf_valid <= in_uu_DtBuf_valid;
      out_DtBuf_addr <= in_uu_DtBuf_addr;
      out_DtBuf_data <= in_uu_DtBuf_data;

    elsif (state = stt_CleanBuffer) then
      
      in_DtBuf_request <= out_rr_DtBuf_request;
      out_rr_DtBuf_valid <= in_DtBuf_valid;
      in_DtBuf_addr <= out_rr_DtBuf_addr;
      out_rr_DtBuf_data  <= in_DtBuf_data;

      in_rr_DtBuf_request <= out_DtBuf_request;
      out_DtBuf_valid <= in_rr_DtBuf_valid;
      out_DtBuf_addr <= in_rr_DtBuf_addr;
      out_DtBuf_data <= x"00000000";
      
    else
      in_DtBuf_request <= out_rr_DtBuf_request;
      out_rr_DtBuf_valid <= in_DtBuf_valid;
      in_DtBuf_addr <= out_rr_DtBuf_addr;
      out_rr_DtBuf_data  <= in_DtBuf_data;

      out_DtBuf_valid <= '0';
      out_DtBuf_addr <= x"00000";
      out_DtBuf_data <= x"00000000";

    end if;

    if (Reset_n = '0') then
      out_rr_DtBuf_data  <= x"00000000";
      out_rr_DtBuf_valid <= '0';
     
      out_cr_DtBuf_data  <= x"00000000";
      out_cr_DtBuf_valid <= '0';
      
      out_lf_DtBuf_data  <= x"00000000";
      out_lf_DtBuf_valid <= '0';
      
      out_rf_DtBuf_data <= x"00000000";
      out_rf_DtBuf_valid <= '0';
      
      out_uu_DtBuf_data <= x"00000000";
      out_uu_DtBuf_valid <= '0';

      in_uu_DtBuf_request <= '0';
      in_cr_DtBuf_request <= '0';
      in_lf_DtBuf_request <= '0';
      in_rf_DtBuf_request <= '0';
    end if;
  end process;

  
  process(clk)

    ---------------------------------------------------------------------------
    -- Procedure that write zero in all positions of Data Buffer
    ---------------------------------------------------------------------------
    procedure CleanBuffer is
    begin
      in_rr_DtBuf_valid <= '1';
      if (count = 0) then
        in_rr_DtBuf_addr <= x"00000";
      else
        in_rr_DtBuf_addr <= in_rr_DtBuf_addr + 1;
      end if;

      if (in_rr_DtBuf_request = '1') then
        count <= count + 1;
      end if;

      if (count = SHIFT_RIGHT(3*FrameSize,2)) then
        state <= stt_Forward;
        forward_state <= stt_forward_uniq_common;
        in_rr_DtBuf_addr <= x"00000";
        count <= 0;
        in_rr_DtBuf_valid <= '0';
      end if;

    end CleanBuffer;

    
-------------------------------------------------------------------------------
-- Change the states syncronously
-------------------------------------------------------------------------------
    procedure ForwardControl is
    begin
      if (s_in_request = '1' and s_in_valid = '1') then
--        assert false report "forward_state = "&forward_state_t'image(forward_state) severity note;
        count <= 0;
      
        if (forward_state = stt_rec_framesize) then
          -- The first parameter is FrameType
          FrameSize <= unsigned(in_data(MEM_ADDR_WIDTH-1 downto 0));

          --   This is a hack. On FPGA when reset the module, I don't know explain
          -- why, reads the first value as zero.
          --   Here we are ignoring this zero value. So the first valid value is
          -- the second one that the module reads. The problem happens on an
          -- Altera Stratix II.
          if (count = 0) then
            count <= 1;
          else
            count <= 0;
            forward_state <= stt_rec_height;
          end if;

          
        elsif (forward_state = stt_rec_height) then
          video_height <= unsigned(in_data(9 downto 0));
          forward_state <= stt_rec_width;

        elsif (forward_state = stt_rec_width) then
          video_width <=  unsigned(in_data(9 downto 0));
          count <= 0;
          forward_state <= stt_forward_none;
          state <= stt_CleanBuffer;

          
        elsif (forward_state = stt_forward_uniq_common) then
          -- Define the offsets
          GoldenFrameOfs <= x"00000";
          LastFrameReconOfs <= FrameSize;
          ThisFrameReconOfs <= SHIFT_LEFT(FrameSize, 1);
       
    
          ---------------------------------------------------------------------
          -- Forward and read the unique values common for all modules
          ---------------------------------------------------------------------
          -- if count = 0 then and forward the pbi->HFragments value
          ---------------------------------------------------------------------
          -- if count = 1 then read and forward the pbi->YPlaneFragments value
          ---------------------------------------------------------------------
          -- if count = 2 then read and forward the pbi->YStride value
          ---------------------------------------------------------------------
          -- if count = 3 then read and forward the pbi->UVPlaneFragments value
          ---------------------------------------------------------------------
          -- if count = 4 then read and forward the pbi->UVStride value
          ---------------------------------------------------------------------
          -- if count = 5 then read and forward the pbi->VFragments value
          ---------------------------------------------------------------------
          -- if count = 6 then read and forward the pbi->ReconYDataOffset value
          ---------------------------------------------------------------------
          -- if count = 7 then read and forward the pbi->ReconUDataOffset value
          ---------------------------------------------------------------------
          -- if count = 8 then read and forward the pbi->ReconVDataOffset value
          ---------------------------------------------------------------------
          count <= count + 1;
          if (count = 2) then
            YStride <= unsigned(in_data(LG_MAX_SIZE+1 downto 0));

          elsif (count = 4) then
            UVStride <= unsigned(in_data(LG_MAX_SIZE   downto 0));

          elsif (count = 6) then
            ReconYDataOffset <= unsigned(in_data(MEM_ADDR_WIDTH-1 downto 0));

          elsif (count = 7) then
            ReconUDataOffset <= unsigned(in_data(MEM_ADDR_WIDTH-1 downto 0));

          elsif (count = 8) then
            ReconVDataOffset <= unsigned(in_data(MEM_ADDR_WIDTH-1 downto 0));
            forward_state <= stt_forward_uniq_cr_lf;
            count <= 0;
          end if;
          
        elsif (forward_state = stt_forward_uniq_cr_lf) then
          -- Forward the pbi->UnitFragments value to CopyRecon and LoopFilter
          forward_state <= stt_forward_uniq_lf;

          -- Verify if the pbi-UnitFragments value is some multiple of 32
          -- because the matrix pbi->display_fragments is package
          MaxDPFCount <= SHIFT_RIGHT(
            unsigned(in_data(LG_MAX_SIZE*2 downto 0)), 5) + 1;
          if (in_data(4 downto 0) = "00000") then
            MaxDPFCount <= SHIFT_RIGHT(
              unsigned(in_data(LG_MAX_SIZE*2 downto 0)), 5);
          end if;

        
        elsif (forward_state = stt_forward_uniq_lf) then
          ---------------------------------------------------------------------
          -- Forward the Matrices pbi->QThreshTable and pbi->LoopFilterLimits
          -- to LoopFilter module
          ---------------------------------------------------------------------
          -- For Count = 0 to Count = 63 forward pbi->QThreshTable
          ---------------------------------------------------------------------
          -- For Count = 64 to Count = 79 forward pbi->LoopFilterLimits
          ---------------------------------------------------------------------
          count <= count + 1;
          if (count = 79) then
            forward_state <= stt_forward_uniq_uu;
            count <= 0;
          end if;
        
        elsif (forward_state = stt_forward_uniq_uu) then
          -- Forward the pbi->info.height value to UpdateUMV module
          forward_state <= stt_forward_uniqperframe_rf;
          
        elsif (forward_state = stt_forward_uniqperframe_rf) then
          ---------------------------------------------------------------------
          -- If Count = 0 forward to ReconFrame the QuantDispFrags that is
          -- equal to pbi->CodedBlockIndex of the software
          ---------------------------------------------------------------------
          -- For Count = 1 to Count = 64 forward the
          -- pbi->dequant_Y_coeffs matrix to ReconFrames
          -----------------------------------------------------------
          -- For Count = 65 to Count = 128 forward the
          -- pbi->dequant_U_coeffs matrix to ReconFrames
          -----------------------------------------------------------
          -- For Count = 129 to Count = 192 forward the
          -- pbi->dequant_V_coeffs matrix to ReconFrames
          -----------------------------------------------------------
          -- For Count = 193 to Count = 256 forward the
          -- dequant_InterY_coeffs matrix to ReconFrames
          -----------------------------------------------------------
          -- For Count = 257 to Count = 320 forward the
          -- dequant_InterU_coeffs matrix to ReconFrames
          -----------------------------------------------------------
          -- For Count = 321 to Count = 384 forward the
          -- dequant_InterV_coeffs matrix to ReconFrames
          count <= count + 1;
          if (Count = 0) then
            if (in_data(LG_MAX_SIZE*2-1 downto 0) = 0) then
              forward_state <= stt_forward_none;
              state <= stt_ReconFrames;
              count <= 0;
              FrameType <= (others => '1');
            end if;
          elsif (count = 384) then
            forward_state <= stt_frametype;
            count <= 0;
          end if;
          
        elsif (forward_state = stt_frametype) then
          -- Forward and read the pbi->FrameType
          forward_state <= stt_forward_golden_ofs_rf;
          FrameType <= unsigned(in_data(7 downto 0));

        -----------------------------------------------------------------------
        --   The three states below is used to forward the three Data Buffer's
        -- offsets to the modules that need these informations
        --   The hardware is responsible for the offsets.
        -----------------------------------------------------------------------
        elsif (forward_state = stt_forward_golden_ofs_rf) then
          forward_state <= stt_forward_last_ofs_rf;
          
        elsif (forward_state = stt_forward_last_ofs_rf) then
          forward_state <= stt_forward_this_ofs_rf;

        elsif (forward_state = stt_forward_this_ofs_rf) then
          forward_state <= stt_forward_rf;

        elsif (forward_state = stt_forward_rf) then
          -----------------------------------------------------------
          -- Forward to ReconFrames the parameters below that are
          -- sent for all fragments
          -----------------------------------------------------------
          -- For Count = 0 to Count = 63 forward the
          -- pbi->QFragData(number of the fragment to be expanded)
          -- matrix
          ------------------------------------------------------------
          -- If Count = 64 forward the
          -- pbi->FragCodingMethod(number of the fragment to be expanded)
          -- value
          -----------------------------------------------------------
          -- If Count = 65 forward the
          -- pbi->FragCoefEOB(number of the fragment to be expanded)
          -- value
          -----------------------------------------------------------
          -- If Count = 66 forward the
          -- (pbi->FragMVect(number of the fragment to be expanded)).x
          -- value
          -----------------------------------------------------------
          -- If Count = 67 forward the
          -- (pbi->FragMVect(number of the fragment to be expanded)).y
          -- value
          -----------------------------------------------------------
          -- If Count = 68 forward the
          -- (number of fragment to be expanded)
          -----------------------------------------------------------
          count <= count + 1;
          if (count = 68) then
            forward_state <= stt_forward_none;
            state <= stt_ReconFrames;
            count <= 0;
          end if;

        elsif (forward_state = stt_forward_dispfrag or
               forward_state = stt_forward_dispfrag_golden) then
--           assert false report "forward_state = "&forward_state_t'image(forward_state) severity note;
--           assert false report "Count = "&integer'image(count) severity note;
--           assert false report "MaxDPFCount = "&integer'image(to_integer(MaxDPFCount)) severity note;

          count <= count + 1;
          if (count = MaxDPFCount - 1) then
            forward_state <= stt_forward_source_ofs_cr;
            count <= 0;
          end if;

        elsif (forward_state = stt_forward_source_ofs_cr) then
--           assert false report "forward_state = "&forward_state_t'image(forward_state) severity note;
          forward_state <= stt_forward_dest_ofs_cr;

        elsif (forward_state = stt_forward_dest_ofs_cr) then
--           assert false report "forward_state = "&forward_state_t'image(forward_state) severity note;
          forward_state <= stt_forward_none;
          state <= stt_CopyRecon;

        elsif (forward_state = stt_forward_lf) then
          forward_state <= stt_forward_offset_lf;

        elsif (forward_state = stt_forward_offset_lf) then
          forward_state <= stt_forward_none;
          state <= stt_LoopFilter;

        elsif (forward_state = stt_forward_offset_uu) then
          assert false report "Calling UU" severity note;
          forward_state <= stt_forward_none;
          state <= stt_UpdateUMV;
        else
          null;
        end if;
      end if;
    end procedure ForwardControl;

    procedure ReconFrames is
    begin
--      assert false report "out_rf_request = "&std_logic'image(out_rf_request) severity note;
      if (rf_done = '1' and rf_eb_done = '1') then
        assert false report "ReconFrames Concluido" severity note;
        forward_state <= stt_forward_dispfrag;
        state <= stt_Forward;
        FrameOfsAux <= LastFrameReconOfs;
        FrameOfsAuxSrc <= ThisFrameReconOfs;
      elsif (rf_eb_done = '1') then
        forward_state <= stt_forward_rf;
        state <= stt_Forward;
        count <= 0;
      else
        null;
      end if;
    end procedure ReconFrames;
    
    procedure CopyRecon is
    begin
      if (cr_done = '1') then
        assert false report "CopyRecon Concluido" severity note;
      
        forward_state <= stt_forward_lf;
        state <= stt_Forward;
        CountCopies <= '0';
         if (FrameType = KEY_FRAME and CountCopies = '0') then
           CountCopies <= '1';
         elsif (FrameType = KEY_FRAME and CountCopies = '1') then
           forward_state <= stt_forward_offset_uu;
           state <= stt_Forward;
         else
           null;
         end if;
      end if;
    end procedure CopyRecon;
    
    procedure LoopFilter is
    begin
      if (lf_done = '1') then
        assert false report "LoopFilter Concluido" severity note;
        forward_state <= stt_forward_offset_uu;
        state <= stt_Forward;
      end if;
    end procedure LoopFilter;

    procedure UpdateUMV is
    begin
      if (uu_done = '1') then
        assert false report "UpdateUMV Concluido" severity note;
        count_lines <= 0;
        count_columns <= 0;
        state <= stt_WriteOut;
        plane_write_state <= stt_plane_write_Y;
        write_state <= stt_write1;
        forward_state <= stt_forward_none;
        CountUpdates <= '0';
        if (FrameType = KEY_FRAME and CountUpdates = '0') then
          FrameOfsAux <= GoldenFrameOfs;
          FrameOfsAuxSrc <= LastFrameReconOfs;
          forward_state <= stt_forward_dispfrag_golden;
          state <= stt_Forward;
          CountUpdates <= '1';
        end if;
      end if;
    end procedure UpdateUMV;

    procedure WriteOut is
    begin
      s_out_valid <= '0';
      if (write_state = stt_write1) then
        write_state <= stt_write2;
        out_rr_DtBuf_request <= '1';
        out_rr_DtBuf_addr <= out_rr_DtBuf_addr + 1;

        count_columns <= count_columns + 4;
        case plane_write_state is

          when stt_plane_write_Y =>
            if (count_columns = 0) then
              out_rr_DtBuf_addr <= resize(SHIFT_RIGHT(LastFrameReconOfs + ReconYDataOffset + YStride * (video_height - 1) - YStride*(count_lines), 2), MEM_ADDR_WIDTH);

            elsif (count_columns = video_width - 4) then
              count_columns <= 0;
              count_lines <= count_lines + 1;
              if (count_lines = video_height - 1) then
                count_lines <= 0;
                plane_write_state <= stt_plane_write_Cb;
              end if;
            end if;

          when stt_plane_write_Cb =>
            if (count_columns = 0) then
              out_rr_DtBuf_addr <= resize(SHIFT_RIGHT(LastFrameReconOfs + ReconUDataOffset + UVStride * ((video_height/2) - 1) - UVStride*(count_lines), 2), MEM_ADDR_WIDTH);
              
            elsif (count_columns = (video_width / 2) - 4) then
              count_columns <= 0;
              count_lines <= count_lines + 1;
              if (count_lines = (video_height / 2) - 1) then
                count_lines <= 0;
                plane_write_state <= stt_plane_write_Cr;
              end if;
            end if;
    
          when stt_plane_write_Cr =>
            if (count_columns = 0) then
              out_rr_DtBuf_addr <= resize(SHIFT_RIGHT(LastFrameReconOfs + ReconVDataOffset + UVStride * ((video_height/2) - 1) - UVStride*(count_lines), 2), MEM_ADDR_WIDTH);
              
            elsif (count_columns = (video_width / 2) - 4) then
              count_columns <= 0;
              count_lines <= count_lines + 1;
              if (count_lines = (video_height / 2) - 1) then
                ---------------------------------------------------------------
                -- Because count_columns, by construction, is always dividable by
                -- 4, count_columns = 1 is used as a flag that indicate we have
                -- wrote all the visible frame
                ---------------------------------------------------------------
                count_columns <= 1;
                count_lines <= 0;
              end if;

            -------------------------------------------------------------------
            -- If we have already wrote all the visible frame we are done
            -------------------------------------------------------------------
            elsif (count_columns = 1) then
--               count_leo <= count_leo + 1;
--               assert count_leo /= 2 report "Dois frames" severity FAILURE;
              assert false report "Teste4" severity note;
              count_columns <= 0;
              forward_state <= stt_forward_uniqperframe_rf;
              plane_write_state <= stt_plane_write_Y;
              state <= stt_Forward;
              write_state <= stt_write1;
              out_rr_DtBuf_request <= '0';
            end if;
        end case;

      elsif (write_state = stt_write2) then
        if (out_rr_DtBuf_valid = '1') then
          s_out_data <= out_rr_DtBuf_data;
          out_rr_DtBuf_request <= '0';
          write_state <= stt_write3;
        end if;
        
      else
        s_out_valid <= '1';
        out_data <= s_out_data;
        if (out_requested = '1') then
          write_state <= stt_write1;
          out_rr_DtBuf_request <= '0';
        end if;
      end if;
    end procedure WriteOut;

  begin
    if (clk'event and clk = '1') then
      if (Reset_n = '0') then
        rf_enable <= '1';
        cr_enable <= '1';
        lf_enable <= '1';
        uu_enable <= '1';

        plane_write_state <= stt_plane_write_Y;
        write_state <= stt_write1;
        forward_state <= stt_rec_framesize;
        state <= stt_Forward;

        
        CountCopies  <= '0';
        CountUpdates <= '0';
        count <= 0;
        out_data <= x"00000000";
        s_out_data <= x"00000000";
        s_out_valid <= '0';

        out_rr_DtBuf_request <= '0';
      else
        case state is
          when stt_CleanBuffer => CleanBuffer;
          when stt_Forward => ForwardControl;
          when stt_ReconFrames => ReconFrames;
          when stt_CopyRecon => CopyRecon;
          when stt_LoopFilter => LoopFilter;
          when stt_UpdateUMV => UpdateUMV;
          when others => WriteOut;
        end case;
      end if;
    end if;
  end process;

    
end a_ReconRefFrames;
