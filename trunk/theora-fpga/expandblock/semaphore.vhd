library std;
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity Semaphore is

  port (Clk,
        Reset_n       :       in std_logic;
        Enable        :       in std_logic;
        
        in_request    :       out std_logic;
        in_valid      :       in std_logic;
        in_data       :       in signed(31 downto 0);

        out_requested :       in std_logic;
        out_valid     :       out std_logic;
        out_data      :       out signed(31 downto 0);

        buffer_wr_e   :       in std_logic;

        in_lf_request    :       out std_logic;
        in_lf_valid      :       in std_logic;
        in_lf_addr       :       in unsigned(19 downto 0);
        in_lf_data       :       in signed(31 downto 0);

        
        out_lf_requested :       in std_logic;
        out_lf_valid     :       out std_logic;
        out_lf_addr      :       in unsigned(19 downto 0);
        out_lf_data      :       out signed(31 downto 0)


        );
end Semaphore;


architecture a_Semaphore of Semaphore is

-- Handshake
  type state_t is (readIn, proc, writeOut);
  signal state : state_t;

  type read_state_t is (stt_lastFraRecCnt, stt_lastFraRec);
  signal read_state : read_state_t;

  signal FrameCount : integer;
  signal count : integer;
  signal maxCount : integer;

  signal s_in_request : std_logic;
  signal s_out_valid : std_logic;

  signal s_in_lf_request : std_logic;
  signal s_out_lf_valid : std_logic;

  signal LastFrameReconSize : integer;
  
  constant LFR_DEPTH : natural := 107712;
--  constant LFR_DEPTH : natural := 71808;
  constant LFR_DATA_WIDTH : natural := 32;
  constant LFR_ADDR_WIDTH : natural := 20;
  
  signal lfr_wr_e     : std_logic;
  signal lfr_wr_addr  : unsigned(LFR_ADDR_WIDTH-1 downto 0);
  signal lfr_wr_data  : signed(LFR_DATA_WIDTH-1 downto 0);
  signal lfr_rd_addr  : unsigned(LFR_ADDR_WIDTH-1 downto 0);
  signal lfr_rd_data  : signed(LFR_DATA_WIDTH-1 downto 0);
  
begin  -- a_Semaphore
  in_request <= s_in_request;
  out_valid <= s_out_valid;
  in_lf_request <= s_in_lf_request;
  out_lf_valid <= s_out_lf_valid;
  
  mem_65536_int32: entity work.syncram
    generic map (LFR_DEPTH, LFR_DATA_WIDTH, LFR_ADDR_WIDTH)
    port map (clk, lfr_wr_e, lfr_wr_addr, lfr_wr_data, lfr_rd_addr, lfr_rd_data);

  process (clk)

    procedure LastFraRecCnt is
    begin
      LastFrameReconSize <= TO_INTEGER(in_data);
      read_state <= stt_lastFraRec;
    end procedure LastFraRecCnt;
    
    procedure LastFraRec is
    begin
      lfr_wr_e <= '1';
      lfr_wr_data <= in_data;
      lfr_wr_addr <= lfr_wr_addr + 1;
      if (count = 0) then
        if (FrameCount = 0) then
          lfr_wr_addr <= x"00000";
        elsif (FrameCount = 1) then
          lfr_wr_addr <= x"08C40";
        else
          lfr_wr_addr <= x"11880";
        end if;
        count <= count + 4;
      elsif(count = LastFrameReconSize - 4)then
        read_state <= stt_lastFraRecCnt;
        count <= 0;
        FrameCount <= FrameCount + 1;
        if (FrameCount = 2) then
          FrameCount <= 0;
          s_out_valid <= '0';
          s_in_request <= '0';
          state <= Proc;
        end if;
      else
        count <= count + 4;
      end if;
    end procedure LastFraRec;


    procedure ReadIn is
    begin
      if (Enable = '1') then
        s_out_valid <= '0';            -- came from WriteOut, out_valid must be 0
        s_in_request <= '1';
        if (s_in_request = '1' and in_valid = '1') then
          case read_state is
            when stt_lastFraRecCnt => LastFraRecCnt;
            when others => LastFraRec;
          end case;
        end if;
      end if;
    end procedure ReadIn;

    procedure Proc is
    begin
      s_out_lf_valid <= '0';
      s_in_lf_request <= '1';
      lfr_wr_e <= '0';
      if (s_in_lf_request = '1' and in_lf_valid = '1') then
        lfr_wr_e <= '1';
        lfr_wr_data <= in_lf_data;
        lfr_wr_addr <= in_lf_addr;
      end if;

      if (out_lf_requested = '1' and s_out_lf_valid = '0') then
        if (count = 0) then
          lfr_rd_addr <= out_lf_addr;
          count <= count + 1;
        elsif (count = 1) then
          count <= count + 1;
        else
          out_lf_data <= lfr_rd_data;
          s_out_lf_valid <= '1';
          count <= 0;
        end if;
      end if;
    end procedure Proc;

    procedure WriteOut is
    begin
      if( out_requested = '1' )then
        if (count = 0) then
          lfr_rd_addr <= x"11880";
          count <= 4;

        elsif (count = 4) then
          lfr_rd_addr <= lfr_rd_addr + 1;
          count <= 8;

        elsif(count < LastFrameReconSize)then
          lfr_rd_addr <= lfr_rd_addr + 1;
--          assert false report "aki" severity note;
          out_data <= lfr_rd_data;
--          assert false report "aki1" severity note;
          s_out_valid <= '1';
          count <= count + 4;
        elsif(count = LastFrameReconSize or
              count = LastFrameReconSize + 4)then
          out_data <= lfr_rd_data;
          s_out_valid <= '1';
          count <= count + 4;
        else
          state <= readIn;  -- on readIn state must set out_valid to 0
          read_state <= stt_lastFraRecCnt;
          count <= 0;
          s_out_valid <= '0';
--LastFrameRecon signal's memory
          lfr_wr_e <= '0';
          lfr_wr_addr <= x"00000";
          lfr_wr_data <= x"00000000";
          lfr_rd_addr <= x"00000";
        end if;
      end if;
    end procedure WriteOut;


    
  begin  -- process
    if (Reset_n = '0') then
      state <= readIn;
      s_in_request <= '0';
      s_out_valid <= '0';

      s_in_lf_request <= '0';
      s_out_lf_valid <= '0';

      count <= 0;
      FrameCount <= 0;
      
--LastFrameRecon signal memories
      lfr_wr_e <= '0';
      lfr_wr_addr <= x"00000";
      lfr_wr_data <= x"00000000";
      lfr_rd_addr <= x"00000";

      
    elsif (clk'event and clk = '1') then
      case state is
        when readIn => ReadIn;
        when proc => Proc;
        when writeOut => WriteOut;
        when others => ReadIn; state <= readIn;
      end case;

      if (buffer_wr_e = '1') then
        state <= writeOut;
        s_in_lf_request <= '0';
        s_out_lf_valid <= '0';
        s_out_valid <= '0';
        s_in_request <= '0';
        count <= 0;
      end if;
    end if;
  end process;

end a_Semaphore;
