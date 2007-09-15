library std;
library ieee;
library work;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.all;

entity ExpandBlock is
  
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

end ExpandBlock;

architecture a_ExpandBlock of ExpandBlock is
  component syncram
    generic (
      DEPTH : positive := 64;             -- How many slots
      DATA_WIDTH : positive := 16;        -- How many bits per slot
      ADDR_WIDTH : positive := 6          -- = ceil(log2(DEPTH))
      );
    port (
      clk : in std_logic;
      wr_e  : in std_logic;
      wr_addr : in unsigned(ADDR_WIDTH-1 downto 0);
      wr_data : in signed(DATA_WIDTH-1 downto 0);
      rd_addr : in unsigned(ADDR_WIDTH-1 downto 0);
      rd_data : out signed(DATA_WIDTH-1 downto 0)
      );
  end component;

  component ReconPixelIndex
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

  component clamp
    port (
      x   : in  SIGNED(16 downto 0);
      sat : out UNSIGNED(7 downto 0));
  end component;

  component IDctSlow
    port (Clk,
          Reset_n : in std_logic;
          
          in_request : out std_logic;
          in_valid : in std_logic;
          in_data : in signed(15 downto 0);
          in_quantmat : in signed(15 downto 0);
          
          out_requested : in std_logic;
          out_valid : out std_logic;
          out_data : out signed(15 downto 0)
          );
  end component;
  
  -- We are using 1024 as the maximum width and height size
  -- = ceil(log2(Maximum Size))
  constant LG_MAX_SIZE    : natural := 10;
  constant MEM_ADDR_WIDTH : natural := 20;
  
-------------------------------------------------------------------------------
-- Signals that must be read at the beginning
-------------------------------------------------------------------------------
--   signal HFragments              : unsigned(LG_MAX_SIZE-3 downto 0);
--   signal VFragments              : unsigned(LG_MAX_SIZE-3 downto 0);
  signal YStride                 : unsigned(LG_MAX_SIZE+1 downto 0);
  signal UVStride                : unsigned(LG_MAX_SIZE   downto 0);
  signal YPlaneFragments         : unsigned(LG_MAX_SIZE*2 downto 0);
  signal UVPlaneFragments        : unsigned(LG_MAX_SIZE*2-2 downto 0);
--   signal ReconYDataOffset        : unsigned(MEM_ADDR_WIDTH-1 downto 0);
--   signal ReconUDataOffset        : unsigned(MEM_ADDR_WIDTH-1 downto 0);
--   signal ReconVDataOffset        : unsigned(MEM_ADDR_WIDTH-1 downto 0);

  
-------------------------------------------------------------------------------
-- Signal that must be read for all frames
-------------------------------------------------------------------------------
  signal FragCodMeth_FragNumber  : unsigned(2 downto 0);
--  signal FragCoefEOB_FragNumber  : unsigned(31 downto 0);
  signal FragMVect_FragNumber_x  : signed(31 downto 0);
  signal FragMVect_FragNumber_y  : signed(31 downto 0);
  signal FragmentNumber          : unsigned(31 downto 0);
  signal FrameType               : unsigned(7 downto 0);

  signal GoldenFrameOfs    : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal ThisFrameReconOfs : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal LastFrameReconOfs : unsigned(MEM_ADDR_WIDTH-1 downto 0);

  
-------------------------------------------------------------------------------
-- Internal Signals
-------------------------------------------------------------------------------
  signal CodingMode         : unsigned(2 downto 0);
  signal ReconPixelsPerLine : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal MvShift            : unsigned(1 downto 0);
  signal MvModMask          : unsigned(31 downto 0);
  signal dequant_coeffs_Ofs : unsigned(8 downto 0);
  signal MVOffset           : signed(31 downto 0);
  signal LastFrameRecPtr    : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal LastFrameRecPtr2   : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal ReconPtr2Offset    : signed(31 downto 0);

  signal s_in_request : std_logic;

  signal count : integer range 0 to 511;
-------------------------------------------------------------------------------
-- dequant_coeffs Offsets 
-------------------------------------------------------------------------------
  constant Y_COEFFS_OFS      : unsigned(8 downto 0) := "000000000";
  constant U_COEFFS_OFS      : unsigned(8 downto 0) := "001000000";
  constant V_COEFFS_OFS      : unsigned(8 downto 0) := "010000000";
  constant INTERY_COEFFS_OFS : unsigned(8 downto 0) := "011000000";
  constant INTERU_COEFFS_OFS : unsigned(8 downto 0) := "100000000";
  constant INTERV_COEFFS_OFS : unsigned(8 downto 0) := "101000000";

  
-------------------------------------------------------------------------------
-- CodingMode constants
-------------------------------------------------------------------------------
  constant CODE_INTER_NO_MV :
    unsigned(2 downto 0) := "000"; -- INTER prediction, (0,0) motion vector implied. 
  constant CODE_INTRA            :
    unsigned(2 downto 0) := "001"; -- INTRA i.e. no prediction.
  constant CODE_INTER_PLUS_MV    :
    unsigned(2 downto 0) := "010"; -- INTER prediction, non zero motion vector.
  constant CODE_INTER_LAST_MV    :
    unsigned(2 downto 0) := "011"; -- Use Last Motion vector
  constant CODE_INTER_PRIOR_LAST :
    unsigned(2 downto 0) := "100"; -- Prior last motion vector
  constant CODE_USING_GOLDEN     :
    unsigned(2 downto 0) := "101"; -- 'Golden frame' prediction (no MV).
  constant CODE_GOLDEN_MV        :
    unsigned(2 downto 0) := "110"; -- 'Golden frame' prediction plus MV.
  constant CODE_INTER_FOURMV     :
    unsigned(2 downto 0) := "111";  -- Inter prediction 4MV per macro block.


-------------------------------------------------------------------------------
-- ReconPixelIndex signal
-------------------------------------------------------------------------------
  constant RPI_DATA_WIDTH : positive := 32;
  constant RPI_POS_WIDTH  : positive := 17;
  signal rpi_position     : unsigned(RPI_POS_WIDTH-1 downto 0);
  signal rpi_value        : signed(RPI_DATA_WIDTH-1 downto 0);

  signal s_rpi_in_request    : std_logic;
  signal s_rpi_in_valid      : std_logic;
  signal s_rpi_in_data       : signed(31 downto 0);
        
  signal s_rpi_out_requested : std_logic;
  signal s_rpi_out_valid     : std_logic;
  signal s_rpi_out_data      : signed(31 downto 0);

  
-------------------------------------------------------------------------------
-- Constants
-------------------------------------------------------------------------------
  constant KEY_FRAME : unsigned(7 downto 0) := "00000000";
  type ModeUsesMC_t is array (0 to 7) of std_logic;
  constant ModeUsesMC : ModeUsesMC_t := ('0','0','1','1','1','0','1','1');

  constant ZERO_M_VECTOR : signed(31 downto 0) := x"00000000";

  
-------------------------------------------------------------------------------
-- States and sub-states
-------------------------------------------------------------------------------

  type state_t is (stt_readIn, stt_PreRecon, stt_Recon, stt_EndProc);
  signal state : state_t;
  
  type read_state_t is (stt_read1, stt_read_uniq_frame_data,
                        stt_read_dqc, stt_read_qtl, stt_read2);
  signal read_state : read_state_t;

  type pre_recon_state_t is (stt_PrepIDct, stt_CallIDct, stt_RecIDct,
                             stt_Calc_RPI_Value, stt_SelectRecons);
  signal pre_recon_state : pre_recon_state_t;

  type calc_rpi_state_t is (stt_calc_rpi1, stt_calc_rpi2);
  signal calc_rpi_state : calc_rpi_state_t;

  
  type select_recons_state_t is (stt_SelectRecons_1, stt_SelectRecons_2,
                                 stt_SelectRecons_3, stt_SelectRecons_4,
                                 stt_SelectRecons_5, stt_SelectRecons_6);
  signal select_recons_state : select_recons_state_t;

  
  type recon_state_t is (stt_Calculate_ReconIntra,
                         stt_ReadPixels,
                         stt_Calculate_ReconInter,
                         stt_Calculate_ReconInterHalf, stt_WriteOut_Recon,
                         stt_ReadMemory, stt_WriteMemory);
  signal recon_state, gotostate, gotostate2 : recon_state_t;

  type recon_calc_state_t is (stt_CalcRecon1, stt_CalcRecon2,
                              stt_CalcRecon3, stt_CalcRecon4,
                              stt_CalcRecon5, stt_CalcRecon6,
                              stt_CalcRecon7, stt_CalcRecon8);
  signal recon_calc_state : recon_calc_state_t;

  
  type read_pixel_state_t is (stt_ReadPixels1, stt_ReadPixels2,
                              stt_ReadPixels3, stt_ReadPixels4,
                              stt_ReadPixels5, stt_ReadPixels6);
  signal read_pixel_state : read_pixel_state_t;


  type write_recon_state_t is (stt_WriteRecon1, stt_WriteRecon2,
                               stt_WriteReconLast);
  signal write_recon_state : write_recon_state_t;
  
-------------------------------------------------------------------------------
-- IDct signals
-------------------------------------------------------------------------------
  signal out_idct_requested : std_logic;
  signal out_idct_valid : std_logic := '0';
  signal out_idct_data : signed(15 downto 0);
  signal out_idct_quantmat : signed(15 downto 0);

  signal in_idct_request : std_logic := '0';
  signal in_idct_valid : std_logic;
  signal in_idct_data : signed(15 downto 0);

  -----------------------------------------------------------------------------
  -- IDct's handshake signals
  -----------------------------------------------------------------------------
  signal s_out_idct_valid   : std_logic;
  signal s_in_idct_request : std_logic;
  
-------------------------------------------------------------------------------
-- Memories signals and constants
-------------------------------------------------------------------------------

  -----------------------------------------------------------------------------
  -- Quantized list
  -----------------------------------------------------------------------------
  signal qtl_wr_e    : std_logic;
  signal qtl_wr_addr : unsigned(5 downto 0);
  signal qtl_wr_data : signed(15 downto 0);
  signal qtl_rd_addr : unsigned(5 downto 0);
  signal qtl_rd_data : signed(15 downto 0);

  -----------------------------------------------------------------------------
  -- Dequantizer coefficients
  -----------------------------------------------------------------------------
  signal dqc_wr_e    : std_logic;
  signal dqc_wr_addr : unsigned(8 downto 0);
  signal dqc_wr_data : signed(15 downto 0);
  signal dqc_rd_addr : unsigned(8 downto 0);
  signal dqc_rd_data : signed(15 downto 0);

  -----------------------------------------------------------------------------
  -- Recon Data Buffer
  -----------------------------------------------------------------------------
  signal rdb_wr_e    : std_logic;
  signal rdb_wr_addr : unsigned(5 downto 0);
  signal rdb_wr_data : signed(15 downto 0);
  signal rdb_rd_addr : unsigned(5 downto 0);
  signal rdb_rd_data : signed(15 downto 0);

  
-------------------------------------------------------------------------------
-- Reconstruct signals, constants and types
-------------------------------------------------------------------------------
  subtype ogg_int_17_t is signed(16 downto 0);
  subtype ogg_int_16_t is signed(15 downto 0);
  subtype ogg_uint_16_t is unsigned(15 downto 0);
  subtype ogg_uint_8_t is unsigned(7 downto 0);
  subtype ogg_int_8_t is signed(7 downto 0);
  subtype ogg_uint_32_t is unsigned(31 downto 0);

  subtype tiny_int is integer range 0 to 255;
  
  signal sum : ogg_int_17_t;
  signal sat : ogg_uint_8_t;
  shared variable auxs17b : ogg_int_17_t;

-- Handshake

  constant dC128 : ogg_int_17_t := "00000000010000000";

  signal s_in_sem_request : std_logic;
  signal s_out_sem_valid : std_logic;

  signal colcount : tiny_int := 0;
  signal offset_RefPtr : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal offset_RefPtr2 : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal offset_ReconPtr : unsigned(MEM_ADDR_WIDTH-1 downto 0); --buffer

  type mem_8_8bits_t is array (0 to 7) of ogg_uint_8_t;
  signal Pixel  : mem_8_8bits_t;
  signal Pixel1 : mem_8_8bits_t;

-- Memories Signals
  signal mem_rd_data  : signed(31 downto 0);
  
begin  -- a_ExpandBlock

  in_request <= s_in_request;
  in_sem_request <= s_in_sem_request;
  out_sem_valid <= s_out_sem_valid;

  clamp255_0: clamp port map (sum, sat); 

  syncram_384_16: syncram
    generic map (DEPTH => 384, DATA_WIDTH => 16, ADDR_WIDTH => 9)
    port map (clk, dqc_wr_e, dqc_wr_addr, dqc_wr_data,
              dqc_rd_addr, dqc_rd_data);

  syncram_64_16: syncram
    generic map (DEPTH => 64, DATA_WIDTH => 16, ADDR_WIDTH => 6)
    port map (clk, rdb_wr_e, rdb_wr_addr, rdb_wr_data,
              rdb_rd_addr, rdb_rd_data);

  
  mem_64_int16: syncram
    generic map (64, 16, 6)
    port map (clk, qtl_wr_e, qtl_wr_addr, qtl_wr_data,
              qtl_rd_addr, qtl_rd_data);

  rpi0: reconpixelindex
    port map (Clk => Clk,
              Reset_n => Reset_n,
              in_request => s_rpi_out_requested,
              in_valid => s_rpi_out_valid,
              in_data => s_rpi_out_data,

              out_requested => s_rpi_in_request,
              out_valid => s_rpi_in_valid,
              out_data => s_rpi_in_data);

  idctslow0: IDctSlow
    port map(clk, Reset_n,
             out_idct_requested, out_idct_valid, out_idct_data,
             out_idct_quantmat,
             in_idct_request, in_idct_valid, in_idct_data);


  in_idct_request <= s_in_idct_request;
  out_idct_valid <= s_out_idct_valid;


  RPI_HandShake: process (in_data, in_valid,
                          state, read_state, pre_recon_state,
                          calc_rpi_state, rpi_position,
                          s_in_request)
  begin  -- process RPI_HandShake
    s_rpi_out_data <= x"00000000";
    s_rpi_out_valid <= '0';
    if (s_in_request = '1') then
      if (state = stt_readIn and read_state = stt_read1) then
        s_rpi_out_data <= in_data;
        s_rpi_out_valid <= in_valid;
      end if;
    else
      if (state = stt_PreRecon and
          pre_recon_state = stt_Calc_RPI_Value and
          calc_rpi_state = stt_calc_rpi1) then
        s_rpi_out_data <= resize(signed('0'&rpi_position), 32);
        s_rpi_out_valid <= '1';
      end if;
    end if;
  end process RPI_HandShake;

  

  process (clk)
-------------------------------------------------------------------------------
-- Procedures called when state is stt_readIn
-------------------------------------------------------------------------------
    ---------------------------------------------------------------------------
    -- Select the procedure called according the read_state
    ---------------------------------------------------------------------------
    procedure read1 is
    begin
      if (count = 0) then
--         HFragments <= unsigned(in_data(LG_MAX_SIZE-3 downto 0));
        count <= count + 1;
      elsif (count = 1) then
        YPlaneFragments <= unsigned(in_data(LG_MAX_SIZE*2 downto 0));
        count <= count + 1;
      elsif (count = 2) then
        YStride <= unsigned(in_data(LG_MAX_SIZE+1 downto 0));
        count <= count + 1;
      elsif (count = 3) then
        UVPlaneFragments <= unsigned(in_data(LG_MAX_SIZE*2-2 downto 0));
        count <= count + 1;
      elsif (count = 4) then
        UVStride <= unsigned(in_data(LG_MAX_SIZE downto 0));
        count <= count + 1;
      elsif (count = 5) then
--         VFragments <= unsigned(in_data(LG_MAX_SIZE-3 downto 0));
        count <= count + 1;
      elsif (count = 6) then
--         ReconYDataOffset <= unsigned(in_data(MEM_ADDR_WIDTH-1 downto 0));
        count <= count + 1;
      elsif (count = 7) then
--         ReconUDataOffset <= unsigned(in_data(MEM_ADDR_WIDTH-1 downto 0));
        count <= count + 1;
      --elsif (count = 8) then
      else
--         ReconVDataOffset <= unsigned(in_data(MEM_ADDR_WIDTH-1 downto 0));
        count <= 0;
        read_state <= stt_read_dqc;
      end if;
    end procedure read1;

    procedure read_dqc is
    begin
      dqc_wr_e <= '1';
      dqc_wr_data <= signed(in_data(15 downto 0));
      dqc_wr_addr <= dqc_wr_addr + 1;

      if (count = 0) then
        dqc_wr_addr <= Y_COEFFS_OFS;
        count <= count + 1;
      elsif (count = 383) then
        count <= 0;
	read_state <= stt_read_uniq_frame_data;
      else
        count <= count + 1;
      end if;
    end procedure read_dqc;

    ---------------------------------------------------------------------------
    -- Procedure that read the parameters sent one time per frame
    ---------------------------------------------------------------------------
    procedure ReadUniqFrameData is
    begin
     
      count <= count + 1;
      if (count = 0) then
        FrameType <= unsigned(in_data(7 downto 0));
      elsif (count = 1) then
        GoldenFrameOfs <= unsigned(in_data(MEM_ADDR_WIDTH-1 downto 0));
      elsif (count = 2) then
        LastFrameReconOfs <= unsigned(in_data(MEM_ADDR_WIDTH-1 downto 0));
      --elsif (count = 3) then
      else
        ThisFrameReconOfs <= unsigned(in_data(MEM_ADDR_WIDTH-1 downto 0));
        count <= 0;
        read_state <= stt_read_qtl;
      end if;
    end procedure ReadUniqFrameData;
    
    procedure read_qtl is
    begin
      dqc_wr_e <= '0';
      
      qtl_wr_e <= '1';
      qtl_wr_data <= signed(in_data(15 downto 0));
      qtl_wr_addr <= qtl_wr_addr + 1;

      if (count = 0) then
        qtl_wr_addr <= "000000";
        count <= count + 1;
      elsif (count = 63) then
        count <= 0;
        read_state <= stt_read2;
      else
        count <= count + 1;
      end if;
    end procedure read_qtl;

    procedure read2 is
    begin
      qtl_wr_e <= '0';
      
      if (count = 0) then
        FragCodMeth_FragNumber <= unsigned(in_data(2 downto 0));
        count <= count + 1;
      elsif (count = 1) then
--        FragCoefEOB_FragNumber <= unsigned(in_data(31 downto 0));
        count <= count + 1;
      elsif (count = 2) then
        FragMVect_FragNumber_x <= in_data;
        count <= count + 1;
      elsif (count = 3) then
        FragMVect_FragNumber_y <= in_data;
        count <= count + 1;
      elsif (count = 4) then
        FragmentNumber <= unsigned(in_data);
        pre_recon_state <= stt_PrepIDct;
        read_state <= stt_read_qtl;
        state <= stt_PreRecon;
        s_in_request <= '0';
        count <= 0;
      end if;
    end procedure read2;


    procedure ReadIn is
    begin
      s_in_request <= '1';
      if (s_in_request = '1' and in_valid = '1') then 
        case read_state is
          when stt_read1 => read1;
          when stt_read_uniq_frame_data => ReadUniqFrameData;
          when stt_read_dqc => read_dqc;
          when stt_read_qtl => read_qtl;
          when others => read2;
        end case;
      end if;
    end procedure ReadIn;

    
-------------------------------------------------------------------------------
-- Procedures called when state is stt_PreRecon
-------------------------------------------------------------------------------

    procedure PrepareToCallIDct is
    begin
      if (count = 0) then
        -- Get coding mode for this block
        if (FrameType = KEY_FRAME) then
          CodingMode <= CODE_INTRA;
        else
          -- Get Motion vector and mode for this block.
          CodingMode <= FragCodMeth_FragNumber;
        end if;
        count <= count + 1;
      else
        -- Select the appropriate inverse Q matrix and line stride
        if (FragmentNumber < YPlaneFragments) then
          ReconPixelsPerLine <= resize(YStride, MEM_ADDR_WIDTH);
          MvShift <= "01";
          MvModMask <= x"00000001";

          -- Select appropriate dequantiser matrix.
          if (CodingMode = CODE_INTRA) then
            dequant_coeffs_Ofs <= Y_COEFFS_OFS;
          else
            dequant_coeffs_Ofs <= INTERY_COEFFS_OFS;
          end if;
        else
          ReconPixelsPerLine <= resize(UVStride, MEM_ADDR_WIDTH);
          MvShift <= "10";
          MvModMask <= x"00000003";

          -- Select appropriate dequantiser matrix. 
          if (CodingMode = CODE_INTRA) then
            if (FragmentNumber < YPlaneFragments + UVPlaneFragments) then
              dequant_coeffs_Ofs <= U_COEFFS_OFS;
            else
              dequant_coeffs_Ofs <= V_COEFFS_OFS;
            end if;
          else
            if (FragmentNumber < YPlaneFragments + UVPlaneFragments) then
              dequant_coeffs_Ofs <= INTERU_COEFFS_OFS;
            else
              dequant_coeffs_Ofs <= INTERV_COEFFS_OFS;
            end if;
          end if;
        end if;
        count <= 0;
        pre_recon_state <= stt_CallIDct;
      end if;
    end procedure PrepareToCallIDct;

    procedure CallIDct is
    begin
      if (out_idct_requested = '1') then
        if (count = 0) then
          qtl_rd_addr <= "000000";
          dqc_rd_addr <= dequant_coeffs_Ofs;
          count <= count + 1;
        elsif (count = 1) then
          -- Wait for the memory delay
          qtl_rd_addr <= qtl_rd_addr + 1;
          dqc_rd_addr <= dqc_rd_addr + 1;
          count <= count + 1;
        elsif (count = 64) then
          out_idct_data <= dqc_rd_data;
          out_idct_quantmat <= qtl_rd_data;
          s_out_idct_valid <= '1';
          count <= count + 1;
        elsif (count = 65) then
          out_idct_data <= dqc_rd_data;
          out_idct_quantmat <= qtl_rd_data;
          s_out_idct_valid <= '1';
          -- IDct can process. The module will be waiting it
          count <= 0;
          pre_recon_state <= stt_RecIDct;
        else
          qtl_rd_addr <= qtl_rd_addr + 1;
          dqc_rd_addr <= dqc_rd_addr + 1;

          out_idct_data <= dqc_rd_data;
          out_idct_quantmat <= qtl_rd_data;
          s_out_idct_valid <= '1';
          count <= count + 1;
        end if;
      end if;
    end procedure CallIDct;

    procedure RecIDct is
    begin
      s_out_idct_valid <= '0';
      s_in_idct_request <= '1';
      if (count = 64) then
        pre_recon_state <= stt_Calc_RPI_Value;
        calc_rpi_state <= stt_calc_rpi1;
        count <= 0;
        s_in_idct_request <= '0';
        -- Convert fragment number to a pixel offset in a reconstruction buffer.
        rpi_position <= resize(FragmentNumber, RPI_POS_WIDTH);
      else
        if (s_in_idct_request = '1' and in_idct_valid = '1') then
          rdb_wr_e <= '1';
          rdb_wr_addr <= rdb_wr_addr + 1;
          rdb_wr_data <= in_idct_data;
          if (count = 0) then
            rdb_wr_addr <= "000000";
          end if;
          count <= count + 1;
        end if;
      end if;
    end procedure RecIDct;


    procedure CalcRPIValue is
    begin
      case calc_rpi_state is
        when stt_calc_rpi1 =>
          -- Wait until ReconPixelIndex can receive the data
          if (s_rpi_out_requested = '1') then
            calc_rpi_state <= stt_calc_rpi2;
          end if;


        when others =>
          -- Wait until ReconPixelIndex returns the value
          s_rpi_in_request <= '1';
          if (s_rpi_in_request = '1' and s_rpi_in_valid = '1') then
            rpi_value <= s_rpi_in_data;
            s_rpi_in_request <= '0';
            pre_recon_state <= stt_SelectRecons;
            select_recons_state <= stt_SelectRecons_1;
          end if;
      end case;
    end procedure CalcRPIValue;


    
    procedure SelectRecons is
    begin
      if (select_recons_state = stt_SelectRecons_1) then
        rdb_wr_e <= '0';
        -- Action depends on decode mode.
        if (CodingMode = CODE_INTER_NO_MV) then
          -- Inter with no motion vector
          
          -- Reconstruct the pixel data using the last frame Reconstructq'ction
          -- and change data when the motion vector is (0,0), the recon is
          -- based on the lastframe without loop filtering---- for testing
          offset_ReconPtr <= resize(ThisFrameReconOfs +
                                    unsigned(rpi_value), MEM_ADDR_WIDTH);
          offset_RefPtr <= resize(LastFrameReconOfs +
                                  unsigned(rpi_value), MEM_ADDR_WIDTH);
          offset_RefPtr2 <= x"00000";
          rdb_rd_addr <= "000000";
          gotostate <= stt_Calculate_ReconInter;
          recon_state <= stt_ReadPixels;
          state <= stt_Recon;
        
        elsif (ModeUsesMC(to_integer(CodingMode)) = '1') then
          -- Work out the base motion vector offset and the 1/2 pixel offset
          -- if any.  For the U and V planes the MV specifies 1/4 pixel
          -- accuracy. This is adjusted to 1/2 pixel as follows ( 0->0,
          -- 1/4->1/2, 1/2->1/2, 3/4->1/2 ).

          ReconPtr2Offset <= x"00000000";
          MVOffset <= x"00000000";
          if (FragMVect_FragNumber_x > ZERO_M_VECTOR) then
            MVOffset <= SHIFT_RIGHT(FragMVect_FragNumber_x, to_integer(MvShift));
            if ((FragMVect_FragNumber_x and signed(MvModMask)) /= x"00000000") then
              ReconPtr2Offset <= x"00000001";
            end if;
          elsif (FragMVect_FragNumber_x < x"000000000") then
            MVOffset <= - SHIFT_RIGHT(- FragMVect_FragNumber_x, to_integer(MvShift)); 
            if (((-FragMVect_FragNumber_x) and signed(MvModMask)) /= x"00000000") then
              ReconPtr2Offset <= x"FFFFFFFF";
            end if;
          end if;
          select_recons_state <= stt_SelectRecons_2;

        elsif (CodingMode = CODE_USING_GOLDEN) then
          -- Golden frame with motion vector
          -- Reconstruct the pixel data using the golden frame
          -- reconstruction and change data
          offset_ReconPtr <= resize(ThisFrameReconOfs +
                                    unsigned(rpi_value), MEM_ADDR_WIDTH);
          offset_RefPtr <= resize(GoldenFrameOfs +
                                  unsigned(rpi_value), MEM_ADDR_WIDTH);
          offset_RefPtr2 <= x"00000";
          rdb_rd_addr <= "000000";
          gotostate <= stt_Calculate_ReconInter;
          recon_state <= stt_ReadPixels;
          state <= stt_Recon;

        else
          -- Simple Intra coding
          -- Get the pixel index for the first pixel in the fragment.
          offset_ReconPtr <= resize(ThisFrameReconOfs +
                                    unsigned(rpi_value), MEM_ADDR_WIDTH);
          offset_RefPtr <= x"00000";
          offset_RefPtr2 <= x"00000";
          rdb_rd_addr <= "000000";
          gotostate <= stt_Calculate_ReconIntra;
          recon_state <= stt_Calculate_ReconIntra;
          state <= stt_Recon;
        end if;

        
      elsif (select_recons_state = stt_SelectRecons_2) then
        if (FragMVect_FragNumber_y > ZERO_M_VECTOR) then
          MVOffset <= resize(
            MVOffset +
            SHIFT_RIGHT(FragMVect_FragNumber_y, to_integer(MvShift)) *
            signed('0' & ReconPixelsPerLine), 32);
          if ((FragMVect_FragNumber_y and signed(MvModMask)) /= x"00000000") then
            ReconPtr2Offset <= ReconPtr2Offset + signed('0' & ReconPixelsPerLine);
          end if;
        elsif (FragMVect_FragNumber_y < ZERO_M_VECTOR) then
          MVOffset <= resize(
            MVOffset -
            SHIFT_RIGHT(-FragMVect_FragNumber_y, to_integer(MvShift)) *
            signed('0' & ReconPixelsPerLine), 32);
          if (((-FragMVect_FragNumber_y) and signed(MvModMask)) /= x"00000000") then
            ReconPtr2Offset <= ReconPtr2Offset - signed('0' & ReconPixelsPerLine);
          end if;
        end if;
        select_recons_state <= stt_SelectRecons_3;

      elsif (select_recons_state = stt_SelectRecons_3) then
        -- Set up the first of the two reconstruction buffer pointers.
        if (CodingMode = CODE_GOLDEN_MV) then
          LastFrameRecPtr <= resize(
            unsigned(('0' & signed(GoldenFrameOfs)) +
                     rpi_value +
                     MVOffset), MEM_ADDR_WIDTH);
        else
          LastFrameRecPtr <= resize(
            unsigned(('0' & signed(LastFrameReconOfs)) +
                     rpi_value +
                     MVOffset), MEM_ADDR_WIDTH);
        end if;
        select_recons_state <= stt_SelectRecons_4;

      elsif (select_recons_state = stt_SelectRecons_4) then
        -- Select the appropriate reconstruction function
        if (ReconPtr2Offset = x"00000000") then
          -- Reconstruct the pixel dats from the reference frame and change data
          -- (no half pixel in this case as the two references were the same.
          offset_ReconPtr <= resize(ThisFrameReconOfs +
                                    unsigned(rpi_value), MEM_ADDR_WIDTH);
          offset_RefPtr <= LastFrameRecPtr;
          offset_RefPtr2 <= x"00000";
          rdb_rd_addr <= "000000";
          gotostate <= stt_Calculate_ReconInter;
          recon_state <= stt_ReadPixels;
          state <= stt_Recon;
        else
          -- Fractional pixel reconstruction.
          -- Note that we only use two pixels per reconstruction even for
          -- the diagonal.
          offset_ReconPtr <= resize(ThisFrameReconOfs +
                             unsigned(rpi_value), MEM_ADDR_WIDTH);
          offset_RefPtr <= LastFrameRecPtr;
          offset_RefPtr2 <= resize(
            unsigned(signed('0' & LastFrameRecPtr) +
                     ReconPtr2Offset), MEM_ADDR_WIDTH);
          rdb_rd_addr <= "000000";
          gotostate <= stt_Calculate_ReconInterHalf;
          recon_state <= stt_ReadPixels;
          state <= stt_Recon;
        end if;
      end if;  
    end procedure SelectRecons;



    procedure PreRecon is
    begin
      case pre_recon_state is
        when stt_PrepIDct => PrepareToCallIDct;
        when stt_CallIDct => CallIDct;
        when stt_RecIDct => RecIDct;
        when stt_Calc_RPI_Value => CalcRPIValue;
     -- when stt_SelectRecons = other
        when others => SelectRecons;
      end case;  
    end procedure PreRecon;


-------------------------------------------------------------------------------
-- Procedures called when state is stt_Recon
-------------------------------------------------------------------------------

    ---------------------------------------------------------------------------
    -- ReconIntra
    ---------------------------------------------------------------------------
    -- Parameters:
    --   ReconPixelsPerLine - 'Distance' to the next buffer's pixel
    --   gotostate - Must be stt_Calculate_ReconIntra
    --   rdb_rd_addr - Must be zero
    --   offset_ReconPtr - offset to write
    --   offset_RefPtr - Must be zero
    --   offset_RefPtr2 - Must be zero
    procedure Calculate_ReconIntra is
    begin
      if (count = 8) then
        out_done <= '1';
        recon_calc_state <= stt_CalcRecon1;
        state <= stt_EndProc;
        count <= 0;
      else
        if (recon_calc_state = stt_CalcRecon1) then
          rdb_rd_addr <= rdb_rd_addr + 1;
          recon_calc_state <= stt_CalcRecon2;

        elsif (recon_calc_state = stt_CalcRecon2) then
          sum <= rdb_rd_data + dC128;
          recon_calc_state <= stt_CalcRecon3;
          -- Wait the clamp

        else
          rdb_rd_addr <= rdb_rd_addr + 1;
          Pixel(colcount) <= sat;
          colcount <= colcount + 1;
          recon_calc_state <= stt_CalcRecon2;
          if (colcount = 7) then
            rdb_rd_addr <= rdb_rd_addr;
            recon_state <= stt_WriteOut_Recon;
            colcount <= 0;
            recon_calc_state <= stt_CalcRecon1;
          end if;
        end if;
      end if;
    end procedure Calculate_ReconIntra;


    procedure ReadPixels is
      variable pointer : unsigned(1 downto 0);
    begin
      if (read_pixel_state = stt_ReadPixels1) then
        s_in_sem_request <= '1';
        in_sem_addr <= SHIFT_RIGHT(offset_RefPtr, 2);
        recon_state <= stt_ReadMemory;
        gotostate2 <= stt_ReadPixels;
        if (offset_RefPtr(1 downto 0) = "00") then
          read_pixel_state <= stt_ReadPixels2;
        else
          read_pixel_state <= stt_ReadPixels4;
        end if;
         
      elsif (read_pixel_state = stt_ReadPixels2) then
          Pixel(0) <= unsigned(mem_rd_data(31 downto 24));
          Pixel(1) <= unsigned(mem_rd_data(23 downto 16));
          Pixel(2) <= unsigned(mem_rd_data(15 downto 8));
          Pixel(3) <= unsigned(mem_rd_data(7 downto 0));
          s_in_sem_request <= '1';
          in_sem_addr <= SHIFT_RIGHT(offset_RefPtr + 4, 2);
          recon_state <= stt_ReadMemory;
          gotostate2 <= stt_ReadPixels;
          read_pixel_state <= stt_ReadPixels3;

      elsif (read_pixel_state = stt_ReadPixels3) then
        Pixel(4) <= unsigned(mem_rd_data(31 downto 24));
        Pixel(5) <= unsigned(mem_rd_data(23 downto 16));
        Pixel(6) <= unsigned(mem_rd_data(15 downto 8));
        Pixel(7) <= unsigned(mem_rd_data(7 downto 0));
        if (gotostate = stt_Calculate_ReconInter) then
          rdb_rd_addr <= rdb_rd_addr + 1;
        end if;
        -- The eigth pixels have already been brought from buffer
        recon_state <= gotostate;
        read_pixel_state <= stt_ReadPixels1;

        
      elsif (read_pixel_state = stt_ReadPixels4) then
        -- If offset_RefPtr is not a multiple of 4
        case offset_RefPtr(1 downto 0) is
          when "01" =>
            Pixel(0) <= unsigned(mem_rd_data(23 downto 16));
            Pixel(1) <= unsigned(mem_rd_data(15 downto 8));
            Pixel(2) <= unsigned(mem_rd_data(7 downto 0));
          when "10" =>
            Pixel(0) <= unsigned(mem_rd_data(15 downto 8));
            Pixel(1) <= unsigned(mem_rd_data(7 downto 0));
          when others =>
            Pixel(0) <= unsigned(mem_rd_data(7 downto 0));
        end case;
        s_in_sem_request <= '1';
        in_sem_addr <= SHIFT_RIGHT(offset_RefPtr + 4, 2);
        recon_state <= stt_ReadMemory;
        gotostate2 <= stt_ReadPixels;
        read_pixel_state <= stt_ReadPixels5;

      elsif (read_pixel_state = stt_ReadPixels5) then
        case offset_RefPtr(1 downto 0) is
          when "01" =>
            pointer := "11";
          when "10" =>
            pointer := "10";
          when others =>
            pointer := "01";
        end case;
        Pixel(0 + to_integer(pointer)) <= unsigned(mem_rd_data(31 downto 24));
        Pixel(1 + to_integer(pointer)) <= unsigned(mem_rd_data(23 downto 16));
        Pixel(2 + to_integer(pointer)) <= unsigned(mem_rd_data(15 downto 8));
        Pixel(3 + to_integer(pointer)) <= unsigned(mem_rd_data(7 downto 0));
        s_in_sem_request <= '1';
        in_sem_addr <= SHIFT_RIGHT(offset_RefPtr + 8, 2);
        recon_state <= stt_ReadMemory;
        gotostate2 <= stt_ReadPixels;
        read_pixel_state <= stt_ReadPixels6;

      elsif (read_pixel_state = stt_ReadPixels6) then
        case offset_RefPtr(1 downto 0) is
          when "01" =>
            Pixel(7) <= unsigned(mem_rd_data(31 downto 24));
          when "10" =>
            Pixel(6) <= unsigned(mem_rd_data(31 downto 24));
            Pixel(7) <= unsigned(mem_rd_data(23 downto 16));
          when others =>
            Pixel(5) <= unsigned(mem_rd_data(31 downto 24));
            Pixel(6) <= unsigned(mem_rd_data(23 downto 16));
            Pixel(7) <= unsigned(mem_rd_data(15 downto 8));
        end case;
        if (gotostate = stt_Calculate_ReconInter) then
          rdb_rd_addr <= rdb_rd_addr + 1;
        end if;

        -- The eigth pixels have already been brought from buffer
        recon_state <= gotostate;
        read_pixel_state <= stt_ReadPixels1;
      end if;
    end procedure ReadPixels;


    ---------------------------------------------------------------------------
    -- ReconInter
    ---------------------------------------------------------------------------
    -- Parameters:
    --   ReconPixelsPerLine - 'Distance' to the next buffer's pixel
    --   gotostate - Must be stt_ReadRefPtr
    --   rdb_rd_addr - Must be zero
    --   offset_ReconPtr - offset to write
    --   offset_RefPtr - offset of the reference buffer
    --   offset_RefPtr2 - Must be zero
    procedure Calculate_ReconInter is
    begin
      if (count = 8) then
        out_done <= '1';
        recon_calc_state <= stt_CalcRecon1;
        state <= stt_EndProc;
        count <= 0;
      else
        if (recon_calc_state = stt_CalcRecon1) then
          sum <= rdb_rd_data + ("000000000" & signed(Pixel(colcount)));
          recon_calc_state <= stt_CalcRecon2;
          -- Wait the clamp
        else
          rdb_rd_addr <= rdb_rd_addr + 1;
          Pixel(colcount) <= sat;
          colcount <= colcount + 1;
          recon_calc_state <= stt_CalcRecon1;
          if (colcount = 7) then
            rdb_rd_addr <= rdb_rd_addr;
            recon_state <= stt_WriteOut_Recon;
            colcount <= 0;
            recon_calc_state <= stt_CalcRecon1;
          end if;
        end if;
      end if;
    end procedure Calculate_ReconInter;
  
    ---------------------------------------------------------------------------
    -- ReconInterHalf
    ---------------------------------------------------------------------------
    -- Parameters:
    --   ReconPixelsPerLine - 'Distance' to the next buffer's pixel
    --   gotostate - Must be stt_ReadRefPtr
    --   rdb_rd_addr - Must be zero
    --   offset_ReconPtr - offset to write
    --   offset_RefPtr - offset of the first reference buffer
    --   offset_RefPtr2 - offset of the second reference buffer

    procedure Calculate_ReconInterHalf is
      variable pointer : unsigned(1 downto 0);
    begin
      if (count = 8) then
        out_done <= '1';
        recon_calc_state <= stt_CalcRecon1;
        state <= stt_EndProc;
        count <= 0;
      else
        if (recon_calc_state = stt_CalcRecon1) then
          s_in_sem_request <= '1';
          in_sem_addr <= SHIFT_RIGHT(offset_RefPtr2, 2);
          recon_state <= stt_ReadMemory;
          gotostate2 <= gotostate;
          -- if offset_RefPtr2 mod 4 = 0
          if (offset_RefPtr2(1 downto 0) = "00") then
            recon_calc_state <= stt_CalcRecon2;
          else
            recon_calc_state <= stt_CalcRecon4;
          end if;
         
        elsif (recon_calc_state = stt_CalcRecon2) then
          Pixel1(0) <= unsigned(mem_rd_data(31 downto 24));
          Pixel1(1) <= unsigned(mem_rd_data(23 downto 16));
          Pixel1(2) <= unsigned(mem_rd_data(15 downto 8));
          Pixel1(3) <= unsigned(mem_rd_data(7 downto 0));
          s_in_sem_request <= '1';
          in_sem_addr <= SHIFT_RIGHT(offset_RefPtr2 + 4, 2);
          recon_state <= stt_ReadMemory;
          gotostate2 <= gotostate;
          recon_calc_state <= stt_CalcRecon3;

        elsif (recon_calc_state = stt_CalcRecon3) then
          Pixel1(4) <= unsigned(mem_rd_data(31 downto 24));
          Pixel1(5) <= unsigned(mem_rd_data(23 downto 16));
          Pixel1(6) <= unsigned(mem_rd_data(15 downto 8));
          Pixel1(7) <= unsigned(mem_rd_data(7 downto 0));
          rdb_rd_addr <= rdb_rd_addr + 1;
          recon_calc_state <= stt_CalcRecon7;


        elsif (recon_calc_state = stt_CalcRecon4) then
          -- If offset_RefPtr2 is not a multiple of 4
          case offset_RefPtr2(1 downto 0) is
            when "01" =>
              Pixel1(0) <= unsigned(mem_rd_data(23 downto 16));
              Pixel1(1) <= unsigned(mem_rd_data(15 downto 8));
              Pixel1(2) <= unsigned(mem_rd_data(7 downto 0));
            when "10" =>
              Pixel1(0) <= unsigned(mem_rd_data(15 downto 8));
              Pixel1(1) <= unsigned(mem_rd_data(7 downto 0));
            when others =>
              Pixel1(0) <= unsigned(mem_rd_data(7 downto 0));
          end case;
          s_in_sem_request <= '1';
          in_sem_addr <= SHIFT_RIGHT(offset_RefPtr2 + 4, 2);
          recon_state <= stt_ReadMemory;
          gotostate2 <= gotostate;
          recon_calc_state <= stt_CalcRecon5;

        elsif (recon_calc_state = stt_CalcRecon5) then
          case offset_RefPtr2(1 downto 0) is
            when "01" =>
              pointer := "11";
            when "10" =>
              pointer := "10";
            when others =>
              pointer := "01";
          end case;
          Pixel1(0 + to_integer(pointer)) <= unsigned(mem_rd_data(31 downto 24));
          Pixel1(1 + to_integer(pointer)) <= unsigned(mem_rd_data(23 downto 16));
          Pixel1(2 + to_integer(pointer)) <= unsigned(mem_rd_data(15 downto 8));
          Pixel1(3 + to_integer(pointer)) <= unsigned(mem_rd_data(7 downto 0));
          s_in_sem_request <= '1';
          in_sem_addr <= SHIFT_RIGHT(offset_RefPtr2 + 8, 2);
          recon_state <= stt_ReadMemory;
          gotostate2 <= gotostate;
          recon_calc_state <= stt_CalcRecon6;

        elsif (recon_calc_state = stt_CalcRecon6) then
          case offset_RefPtr2(1 downto 0) is
            when "01" =>
              Pixel1(7) <= unsigned(mem_rd_data(31 downto 24));
            when "10" =>
              Pixel1(6) <= unsigned(mem_rd_data(31 downto 24));
              Pixel1(7) <= unsigned(mem_rd_data(23 downto 16));
            when others =>
              Pixel1(5) <= unsigned(mem_rd_data(31 downto 24));
              Pixel1(6) <= unsigned(mem_rd_data(23 downto 16));
              Pixel1(7) <= unsigned(mem_rd_data(15 downto 8));
          end case;
          rdb_rd_addr <= rdb_rd_addr + 1;
          recon_calc_state <= stt_CalcRecon7;
          
        elsif (recon_calc_state = stt_CalcRecon7) then
          sum <= rdb_rd_data +
                 SHIFT_RIGHT(
                   ("000000000" & signed(Pixel(colcount))) +
                   ("000000000" & signed(Pixel1(colcount))) , 1);
          recon_calc_state <= stt_CalcRecon8;
          -- Wait the clamp

        else
          rdb_rd_addr <= rdb_rd_addr + 1;
          Pixel(colcount) <= sat;
          colcount <= colcount + 1;
          recon_calc_state <= stt_CalcRecon7;
          if (colcount = 7) then
            rdb_rd_addr <= rdb_rd_addr;
            recon_state <= stt_WriteOut_Recon;
            colcount <= 0;
            recon_calc_state <= stt_CalcRecon1;
          end if;
        end if;
      end if;
    end procedure Calculate_ReconInterHalf;
    

    procedure WriteOut_Recon is
    begin

      if (write_recon_state = stt_WriteRecon1) then
          out_sem_addr <= SHIFT_RIGHT(offset_ReconPtr, 2);
          out_sem_data <= signed(Pixel(0)) &
                          signed(Pixel(1)) &
                          signed(Pixel(2)) &
                          signed(Pixel(3));
          s_out_sem_valid <= '1';
          write_recon_state <= stt_WriteRecon2;
          recon_state <= stt_WriteMemory;

      elsif (write_recon_state <= stt_WriteRecon2) then
        out_sem_addr <= SHIFT_RIGHT(offset_ReconPtr + 4, 2);
        out_sem_data <= signed(Pixel(4)) &
                        signed(Pixel(5)) &
                        signed(Pixel(6)) &
                        signed(Pixel(7));
        s_out_sem_valid <= '1';
        write_recon_state <= stt_WriteReconLast;
        recon_state <= stt_WriteMemory;
        
      else
        --write_recon_state = stt_WriteReconLast;
        write_recon_state <= stt_WriteRecon1;

        recon_state <= stt_ReadPixels;
        if (gotostate = stt_Calculate_ReconIntra) then
          recon_state <= gotostate;
        end if;
        
        offset_ReconPtr <= offset_ReconPtr + ReconPixelsPerLine;
        offset_RefPtr <= offset_RefPtr + ReconPixelsPerLine;
        offset_RefPtr2 <= offset_RefPtr2 + ReconPixelsPerLine;
        count <= count + 1;
      end if;
    end procedure WriteOut_Recon;
    
    procedure ReadMemory is
    begin
      s_in_sem_request <= '1';
      if (s_in_sem_request = '1' and in_sem_valid = '1') then
        mem_rd_data <= in_sem_data;
        s_in_sem_request <= '0';
        recon_state <= gotostate2;
      end if;
    end procedure ReadMemory;


    
    procedure WriteMemory is
    begin
      if (out_sem_requested = '1') then
        recon_state <= stt_WriteOut_Recon;
        s_out_sem_valid <= '0';
      end if;
    end procedure WriteMemory;

    procedure Recon is
    begin
      case recon_state is
        when stt_WriteOut_Recon => WriteOut_Recon;
        when stt_WriteMemory => WriteMemory;
        when stt_ReadMemory => ReadMemory;
        when stt_ReadPixels => ReadPixels;
        when stt_Calculate_ReconInterHalf => Calculate_ReconInterHalf;
        when stt_Calculate_ReconInter => Calculate_ReconInter;
        --when stt_Calculate_ReconIntra => Calculate_ReconIntra;
        when others => Calculate_ReconIntra;
      end case;  
    end procedure Recon;

    
-------------------------------------------------------------------------------
-- Procedures called when state is stt_EndProc
-------------------------------------------------------------------------------
    procedure EndProc is
    begin
      count <= 0;
      out_done <= '0';
      state <= stt_readIn;
    end procedure EndProc;
    
  begin  -- process
    if (clk'event and clk = '1') then
      if (Reset_n = '0') then
        state <= stt_readIn;
        read_state <= stt_read1;
        pre_recon_state <= stt_PrepIDct;
        select_recons_state <= stt_SelectRecons_1;
        read_pixel_state <= stt_ReadPixels1;
        write_recon_state <= stt_WriteRecon1;
        
        s_in_request <= '0';
        s_in_sem_request <= '0';
        count <= 0;
        s_out_sem_valid <= '0';
        out_done <= '0';

        s_out_idct_valid <= '0';
        s_in_idct_request <= '0';


        colcount <= 0;
        sum <= '0' & x"0000";
        
        rpi_position <= '0' & x"0000";
--         HFragments <= x"11";
--         VFragments <= x"00";
        YStride <= x"000";
        UVStride <= "000" & x"00";
        YPlaneFragments <= '0' & x"00000";
        UVPlaneFragments <= "000" & x"0000";
--         ReconYDataOffset <= x"00000";
--         ReconUDataOffset <= x"00000";
--         ReconVDataOffset <= x"00000";

        qtl_wr_e <= '0';
        qtl_wr_addr <= "000000";
        qtl_wr_data <= "0000000000000000";
        qtl_rd_addr <= "000000";

        dqc_wr_e <= '0';
        dqc_wr_addr <= "000000000";
        dqc_wr_data <= "0000000000000000";
        dqc_rd_addr <= "000000000";

        rdb_wr_e <= '0';
        rdb_wr_addr <= "000000";
        rdb_wr_data <= "0000000000000000";
        rdb_rd_addr <= "000000";


      else
        s_in_request <= '0';
        if (Enable = '1') then
          -- If is a new frame should read the dequantized matrix again.
          if (in_newframe = '1') then
            read_state <= stt_read_dqc;
          end if;

          case state is
            when stt_readIn => ReadIn;
            when stt_PreRecon => PreRecon;
            when stt_Recon => Recon;
            when others => EndProc;
          end case;
        end if;
      end if;
    end if;
  end process;
  
  
end a_ExpandBlock;
