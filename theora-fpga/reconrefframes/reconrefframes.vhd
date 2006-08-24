library std;
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

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
  signal MaxDPFCount : unsigned(LG_MAX_SIZE*2 downto 0);

  signal s_in_request : std_logic;
  signal s_in_valid   : std_logic;

  signal CountCopies  : std_logic;
  signal CountUpdates : std_logic;
  signal count : integer;
  
  constant KEY_FRAME : unsigned(7 downto 0) := "00000000";

  type state_t is (stt_Send, stt_ReconFrames,
                   stt_CopyRecon, stt_LoopFilter,
                   stt_UpdateUMV, stt_WriteOut);
  signal state : state_t;
  
  
  type send_state_t is (stt_rec_framesize,
                        stt_send_uniq_common,
                        stt_send_uniq_cr_lf,
                        stt_send_uniq_lf,
                        stt_send_uniq_uu,
                        stt_send_rf,
                        stt_frametype,
                        stt_send_golden_ofs_rf,
                        stt_send_last_ofs_rf,
                        stt_send_this_ofs_rf,
                        stt_send_dispfrag,
                        stt_send_source_ofs_cr,
                        stt_send_dest_ofs_cr,
                        stt_send_lf,
                        stt_send_offset_lf,
                        stt_send_offset_uu,
                        stt_send_dispfrag_golden,
                        stt_send_none);
  signal send_state : send_state_t;

  type write_state_t is (stt_write1, stt_write2);
  signal write_state : write_state_t;
  
begin  -- a_ReconRefFrames

  reconframes0: entity work.reconframes
    port map(clk, Reset_n, rf_enable,
             out_rf_request, out_rf_valid, out_rf_data,
             out_rf_DtBuf_request, out_rf_DtBuf_valid, out_rf_DtBuf_addr, out_rf_DtBuf_data,
             in_rf_DtBuf_request, in_rf_DtBuf_valid, in_rf_DtBuf_addr, in_rf_DtBuf_data,
             rf_done, rf_eb_done);

  loopfilter0: entity work.loopfilter
    port map(clk, Reset_n, rf_enable,
             out_lf_request, out_lf_valid, out_lf_data,
             out_lf_DtBuf_request, out_lf_DtBuf_valid, out_lf_DtBuf_addr, out_lf_DtBuf_data,
             in_lf_DtBuf_request, in_lf_DtBuf_valid, in_lf_DtBuf_addr, in_lf_DtBuf_data,
             lf_done);
  
  copyrecon0: entity work.copyrecon
    port map(clk, Reset_n, cr_enable,
             out_cr_request, out_cr_valid, out_cr_data,
             out_cr_DtBuf_request, out_cr_DtBuf_valid, out_cr_DtBuf_addr, out_cr_DtBuf_data,
             in_cr_DtBuf_request, in_cr_DtBuf_valid, in_cr_DtBuf_addr, in_cr_DtBuf_data,
             cr_done);

  updateumv0: entity work.UpdateUMV
    port map(clk, Reset_n, uu_enable,
             out_uu_request, out_uu_valid, out_uu_data,
             out_uu_DtBuf_request, out_uu_DtBuf_valid, out_uu_DtBuf_addr, out_uu_DtBuf_data,
             in_uu_DtBuf_request, in_uu_DtBuf_valid, in_uu_DtBuf_addr, in_uu_DtBuf_data,
             uu_done);

  databuffer0: entity work.databuffer
    port map(clk, Reset_n,
             out_DtBuf_request, out_DtBuf_valid, out_DtBuf_addr, out_DtBuf_data,
             in_DtBuf_request, in_DtBuf_valid, in_DtBuf_addr, in_DtBuf_data);



  -----------------------------------------------------------------------------
  -- Switch the in_request
  -----------------------------------------------------------------------------
  in_request <= '0' when (send_state = stt_send_golden_ofs_rf or
                          send_state = stt_send_last_ofs_rf or
                          send_state = stt_send_this_ofs_rf or
                          send_state = stt_send_source_ofs_cr or
                          send_state = stt_send_dest_ofs_cr or
                          send_state = stt_send_offset_lf or
                          send_state = stt_send_offset_uu or
                          send_state = stt_send_dispfrag_golden or
                          send_state = stt_send_none) else
                s_in_request;


  
  -----------------------------------------------------------------------------
  -- Switch the in_data and the in_valid
  -----------------------------------------------------------------------------
  process (send_state,
           out_rf_request,
           out_cr_request,
           out_lf_request,
           out_uu_request,
           in_valid,
           in_data)

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
--    assert false report "in_data = "&integer'image(to_integer(in_data)) severity note;
--    assert false report "in_valid = "&std_logic'image(in_valid) severity note;
    -----------------------------------------------------------------------------
    -- Unique Parameters
    -----------------------------------------------------------------------------    
    if (send_state = stt_rec_framesize) then
      s_in_request <= '1';
      FrameSize <= unsigned(in_data(MEM_ADDR_WIDTH-1 downto 0));

    elsif (send_state = stt_send_uniq_common) then
--      assert false report "out_rf_request = "&std_logic'image(out_rf_request) severity note;
--      assert false report "out_cr_request = "&std_logic'image(out_cr_request) severity note;
--      assert false report "out_lf_request = "&std_logic'image(out_lf_request) severity note;
--      assert false report "out_uu_request = "&std_logic'image(out_uu_request) severity note;
      s_in_request <= out_rf_request and
                 out_cr_request and
                 out_lf_request and
                 out_uu_request;
      out_rf_valid <= in_valid;
      out_cr_valid <= in_valid;
      out_lf_valid <= in_valid;
      out_uu_valid <= in_valid;

    elsif (send_state = stt_send_uniq_cr_lf) then
      -------------------------------------------------------------------------
      -- UnitFragment is sent to CopyRecon and LoopFilter and read internal
      -------------------------------------------------------------------------
--      assert false report "out_cr_request = "&std_logic'image(out_cr_request) severity note;
--      assert false report "out_lf_request = "&std_logic'image(out_lf_request) severity note;
      s_in_request <= out_cr_request and
                 out_lf_request;
      
      out_cr_valid <= in_valid;
      out_lf_valid <= in_valid;

      MaxDPFCount <= SHIFT_RIGHT(
        unsigned(in_data(LG_MAX_SIZE*2 downto 0)), 5) + 1;
      if (in_data(4 downto 0) = "00000") then
        MaxDPFCount <= SHIFT_RIGHT(
          unsigned(in_data(LG_MAX_SIZE*2 downto 0)), 5);
      end if;

      
    elsif (send_state = stt_send_uniq_lf) then
      s_in_request <= out_lf_request;
      out_lf_valid <= in_valid;

    elsif (send_state = stt_send_uniq_uu) then
      s_in_request <= out_uu_request;
      out_uu_valid <= in_valid;
-------------------------------------------------------------------------------
-- ReconFrames Parameters
-------------------------------------------------------------------------------
    elsif (send_state = stt_send_rf) then
      s_in_request <= out_rf_request;
      out_rf_valid <= in_valid;

    elsif (send_state = stt_frametype) then
      -------------------------------------------------------------------------
      -- FrameType is sent to ReconFrames and read internal
      -------------------------------------------------------------------------
      s_in_request <= out_rf_request;
      out_rf_valid <= in_valid;
      FrameType <= unsigned(in_data(7 downto 0));
                   
    elsif (send_state = stt_send_golden_ofs_rf) then
      s_in_request <= out_rf_request;
      s_in_valid <= '1';
      out_rf_valid <= '1';
      out_rf_data <= resize('0' & signed(GoldenFrameOfs), 32);

    elsif (send_state = stt_send_last_ofs_rf) then
      s_in_request <= out_rf_request;
      s_in_valid <= '1';
      out_rf_valid <= '1';
      out_rf_data <= resize('0' & signed(LastFrameReconOfs), 32);

    elsif (send_state = stt_send_this_ofs_rf) then
      s_in_request <= out_rf_request;
      s_in_valid <= '1';
      out_rf_valid <= '1';
      out_rf_data <= resize('0' & signed(ThisFrameReconOfs), 32);

    elsif (send_state = stt_send_dispfrag) then
      s_in_request <= out_cr_request and
                      out_lf_request;
      out_cr_valid <= in_valid;
      out_lf_valid <= in_valid;
      
    elsif (send_state = stt_send_source_ofs_cr) then
      s_in_request <= out_cr_request;
      s_in_valid <= '1';
      out_cr_valid <= '1';
      out_cr_data <= resize('0' & signed(FrameOfsAuxSrc), 32);
     
    elsif (send_state = stt_send_dest_ofs_cr) then
      s_in_request <= out_cr_request;
      s_in_valid <= '1';
      out_cr_valid <= '1';
      out_cr_data <= resize('0' & signed(FrameOfsAux), 32);
      
    elsif (send_state = stt_send_lf) then
      s_in_request <= out_lf_request;
      out_lf_valid <= in_valid;
      
    elsif (send_state = stt_send_offset_lf) then
      s_in_request <= out_lf_request;
      s_in_valid <= '1';
      out_lf_valid <= '1';
      out_lf_data <= resize('0' & signed(LastFrameReconOfs), 32);

    elsif (send_state = stt_send_offset_uu) then
      s_in_request <= out_uu_request;
      s_in_valid <= '1';
      out_uu_valid <= '1';
      out_uu_data <= resize('0' & signed(FrameOfsAux), 32);

    elsif (send_state = stt_send_dispfrag_golden) then
      -------------------------------------------------------------------------
      -- If it is a key frame then all fragments must be displayed.
      -- In such case all values of display_fragments is one
      -------------------------------------------------------------------------
      s_in_request <= out_cr_request;
      s_in_valid <= '1';
      out_cr_valid <= '1';
      out_cr_data <= x"FFFFFFFF";
     
    end if;
   
  end process;


  process (state,
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
           out_rr_DtBuf_addr
           )
  begin  -- process state
    case state is
      when stt_ReconFrames =>
        in_DtBuf_request <= out_rf_DtBuf_request;
        out_rf_DtBuf_valid <= in_DtBuf_valid;
        in_DtBuf_addr <= out_rf_DtBuf_addr;
        out_rf_DtBuf_data  <= in_DtBuf_data;

        in_rf_DtBuf_request <= out_DtBuf_request;
        out_DtBuf_valid <= in_rf_DtBuf_valid;
        out_DtBuf_addr <= in_rf_DtBuf_addr;
        out_DtBuf_data <= in_rf_DtBuf_data;
        
      when stt_CopyRecon =>
        in_DtBuf_request <= out_cr_DtBuf_request;
        out_cr_DtBuf_valid <= in_DtBuf_valid;
        in_DtBuf_addr <= out_cr_DtBuf_addr;
        out_cr_DtBuf_data  <= in_DtBuf_data;

        in_cr_DtBuf_request <= out_DtBuf_request;
        out_DtBuf_valid <= in_cr_DtBuf_valid;
        out_DtBuf_addr <= in_cr_DtBuf_addr;
        out_DtBuf_data <= in_cr_DtBuf_data;

      when stt_LoopFilter =>
        in_DtBuf_request <= out_lf_DtBuf_request;
        out_lf_DtBuf_valid <= in_DtBuf_valid;
        in_DtBuf_addr <= out_lf_DtBuf_addr;
        out_lf_DtBuf_data  <= in_DtBuf_data;

        in_lf_DtBuf_request <= out_DtBuf_request;
        out_DtBuf_valid <= in_lf_DtBuf_valid;
        out_DtBuf_addr <= in_lf_DtBuf_addr;
        out_DtBuf_data <= in_lf_DtBuf_data;

      when stt_UpdateUMV =>
        in_DtBuf_request <= out_uu_DtBuf_request;
        out_uu_DtBuf_valid <= in_DtBuf_valid;
        in_DtBuf_addr <= out_uu_DtBuf_addr;
        out_uu_DtBuf_data  <= in_DtBuf_data;

        in_uu_DtBuf_request <= out_DtBuf_request;
        out_DtBuf_valid <= in_uu_DtBuf_valid;
        out_DtBuf_addr <= in_uu_DtBuf_addr;
        out_DtBuf_data <= in_uu_DtBuf_data;

      when others =>
        in_DtBuf_request <= out_rr_DtBuf_request;
        out_rr_DtBuf_valid <= in_DtBuf_valid;
        in_DtBuf_addr <= out_rr_DtBuf_addr;
        out_rr_DtBuf_data  <= in_DtBuf_data;

        out_DtBuf_valid <= '0';
        out_DtBuf_addr <= x"00000";
        out_DtBuf_data <= x"00000000";

    end case;
  end process;

  
  process(clk)
-------------------------------------------------------------------------------
-- Change the states syncronous
-------------------------------------------------------------------------------
    procedure SendControl is
    begin
--    assert false report "s_in_request = "&std_logic'image(s_in_request) severity note;
--    assert false report "in_request = "&std_logic'image(in_request) severity note;
--    assert false report "count = "&integer'image(count) severity note;
      if (s_in_request = '1' and s_in_valid = '1') then
        count <= 0;
        if (send_state = stt_rec_framesize) then
          send_state <= stt_send_uniq_common;
          
        elsif (send_state = stt_send_uniq_common) then
          GoldenFrameOfs <= x"00000";
          LastFrameReconOfs <= FrameSize;
          ThisFrameReconOfs <= SHIFT_LEFT(FrameSize, 1);
         
          count <= count + 1;
          if (count = 8) then
            send_state <= stt_send_uniq_cr_lf;
            count <= 0;
          end if;

          
        elsif (send_state = stt_send_uniq_cr_lf) then
          send_state <= stt_send_uniq_lf;

          
        elsif (send_state = stt_send_uniq_lf) then
          count <= count + 1;
          if (count = 79) then
            send_state <= stt_send_uniq_uu;
            count <= 0;
          end if;
        
        elsif (send_state = stt_send_uniq_uu) then
          send_state <= stt_send_rf;

        elsif (send_state = stt_send_rf) then
          count <= count + 1;
          if (count = 453) then
            send_state <= stt_frametype;
            count <= 0;
          end if;

          
        elsif (send_state = stt_frametype) then
          send_state <= stt_send_golden_ofs_rf;

        elsif (send_state = stt_send_golden_ofs_rf) then
          send_state <= stt_send_last_ofs_rf;
          
        elsif (send_state = stt_send_last_ofs_rf) then
          send_state <= stt_send_this_ofs_rf;

        elsif (send_state = stt_send_this_ofs_rf) then
          send_state <= stt_send_none;
          state <= stt_ReconFrames;
  --        assert false report "ReconFrames" severity note;

        elsif (send_state = stt_send_dispfrag or
               send_state = stt_send_dispfrag_golden) then
          count <= count + 1;
          if (count = MaxDPFCount - 1) then
            send_state <= stt_send_source_ofs_cr;
            count <= 0;
          end if;

        elsif (send_state = stt_send_source_ofs_cr) then
          send_state <= stt_send_dest_ofs_cr;

        elsif (send_state = stt_send_dest_ofs_cr) then
          send_state <= stt_send_none;
          state <= stt_CopyRecon;

        elsif (send_state = stt_send_lf) then
          send_state <= stt_send_offset_lf;

        elsif (send_state = stt_send_offset_lf) then
          send_state <= stt_send_none;
          state <= stt_LoopFilter;


        elsif (send_state = stt_send_offset_uu) then
          assert false report "Calling UU" severity note;
          send_state <= stt_send_none;
          state <= stt_UpdateUMV;

        end if;
      end if;
    end procedure SendControl;

    procedure ReconFrames is
    begin
--    assert false report "out_rf_request = "&std_logic'image(out_rf_request) severity note;
      if (rf_done = '1' and rf_eb_done = '1') then
        assert false report "ReconFrames Concluido" severity note;
        send_state <= stt_send_dispfrag;
        state <= stt_Send;
        FrameOfsAux <= LastFrameReconOfs;
        FrameOfsAuxSrc <= ThisFrameReconOfs;
      elsif (rf_eb_done = '1') then
   
        send_state <= stt_send_rf;
        state <= stt_Send;
        count <= 1;
      end if;
    end procedure ReconFrames;
    
    procedure CopyRecon is
    begin
      if (cr_done = '1') then
        assert false report "CopyRecon Concluido" severity note;
        send_state <= stt_send_lf;
        state <= stt_Send;
        CountCopies <= '0';
        if (FrameType = KEY_FRAME and CountCopies = '0') then
          CountCopies <= '1';
        elsif (FrameType = KEY_FRAME and CountCopies = '1') then
          send_state <= stt_send_offset_uu;
          state <= stt_Send;
        end if;
      end if;
    end procedure CopyRecon;
    
    procedure LoopFilter is
    begin
      if (lf_done = '1') then
        assert false report "LoopFilter Concluido" severity note;
        send_state <= stt_send_offset_uu;
        state <= stt_Send;
      end if;
    end procedure LoopFilter;

    procedure UpdateUMV is
    begin
      if (uu_done = '1') then
        assert false report "UpdateUMV Concluido" severity note;
        count <= 0;
        state <= stt_WriteOut;
        send_state <= stt_send_none;
        CountUpdates <= '0';
        if (FrameType = KEY_FRAME and CountUpdates = '0') then
          FrameOfsAux <= GoldenFrameOfs;
          FrameOfsAuxSrc <= LastFrameReconOfs;
          send_state <= stt_send_dispfrag_golden;
          state <= stt_Send;
          CountUpdates <= '1';
        end if;
      end if;
    end procedure UpdateUMV;

    procedure WriteOut is
    begin

      out_valid <= '0';
      if (write_state = stt_write1) then
        
        write_state <= stt_write2;
        out_rr_DtBuf_request <= '1';
        out_rr_DtBuf_addr <= out_rr_DtBuf_addr + 1;

        count <= count + 4;
        if (count = 0) then
--          assert false report "Writing Data" severity note;
--          out_rr_DtBuf_addr <= SHIFT_RIGHT(LastFrameReconOfs, 2);
          out_rr_DtBuf_addr <= SHIFT_RIGHT(LastFrameReconOfs, 2);

        elsif (count = FrameSize) then
--          assert false report "Data Wrote" severity note;
          count <= 0;
          send_state <= stt_send_rf;
          state <= stt_Send;
          write_state <= stt_write1;
          out_rr_DtBuf_request <= '0';
          out_rr_DtBuf_addr <= SHIFT_RIGHT(LastFrameReconOfs, 2);
          
        end if;

      elsif (write_state = stt_write2) then
        if (out_rr_DtBuf_valid = '1' and out_requested = '1') then
          out_valid <= '1';
          out_data <= out_rr_DtBuf_data;
          write_state <= stt_write1;
          out_rr_DtBuf_request <= '0';
        end if;
      end if;

    end procedure WriteOut;

  begin
    if (Reset_n = '0') then
      rf_enable <= '1';
      cr_enable <= '1';
      lf_enable <= '1';
      uu_enable <= '1';
      
      write_state <= stt_write1;
      send_state <= stt_rec_framesize;
      state <= stt_Send;

      CountCopies  <= '0';
      CountUpdates <= '0';
      count <= 0;

      
    elsif (clk'event and clk = '1') then
      case state is
        when stt_Send => SendControl;
        when stt_ReconFrames => ReconFrames;
        when stt_CopyRecon => CopyRecon;
        when stt_LoopFilter => LoopFilter;
        when stt_UpdateUMV => UpdateUMV;
        when others => WriteOut;
      end case;
    end if;
  end process;

    
end a_ReconRefFrames;
