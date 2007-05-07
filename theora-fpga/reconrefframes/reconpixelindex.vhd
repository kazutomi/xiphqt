library std;
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity ReconPixelIndex is
  port (Clk,
        Reset_n       : in  std_logic;
        
        in_request    : out std_logic;
        in_valid      : in  std_logic;
        in_data       : in  signed(31 downto 0);
        
        out_requested : in  std_logic;
        out_valid     : out std_logic;
        out_data      : out signed(31 downto 0)
        );
end entity ReconPixelIndex;

architecture a_ReconPixelIndex of ReconPixelIndex is
  component Divider
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
          remainder     : out unsigned(WIDTH-1 downto 0));
  end component;


  
  constant VFRAGPIXELS             : unsigned(3 downto 0) := x"8";
  constant HFRAGPIXELS             : unsigned(3 downto 0) := x"8";
  
  constant RPI_POS_WIDTH           : positive := 17;
  constant HV_FRAG_WIDTH           : positive := 8;
  constant Y_STRIDE_WIDTH          : positive := 12;
  constant UV_STRIDE_WIDTH         : positive := 11;
  constant Y_PL_FRAG_WIDTH         : positive := 21;
  constant UV_PL_FRAG_WIDTH        : positive := 19;
  constant RECON_Y_DATA_OFS_WIDTH  : positive := 20;
  constant RECON_UV_DATA_OFS_WIDTH : positive := 20;
                                                 
  -- States machines
  type state_t is (stt_readIn, stt_Proc, stt_WriteOut);
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
                        stt_read_Position);
  signal read_state : read_state_t;

  type proc_state_t is (stt_proc_1, stt_proc_2);
  signal proc_state : proc_state_t;
  
  -- Signals that will be received
  signal rpi_position     : unsigned(RPI_POS_WIDTH-1 downto 0);
  signal HFragments       : unsigned(HV_FRAG_WIDTH-1 downto 0);
  signal VFragments       : unsigned(HV_FRAG_WIDTH-1 downto 0);
  signal YStride          : unsigned(Y_STRIDE_WIDTH-1 downto 0);
  signal UVStride         : unsigned(UV_STRIDE_WIDTH-1 downto 0);
  signal YPlaneFragments  : unsigned(Y_PL_FRAG_WIDTH-1 downto 0);
  signal UVPlaneFragments : unsigned(UV_PL_FRAG_WIDTH-1 downto 0);
  signal ReconYDataOffset : unsigned(RECON_Y_DATA_OFS_WIDTH-1 downto 0);
  signal ReconUDataOffset : unsigned(RECON_UV_DATA_OFS_WIDTH-1 downto 0);
  signal ReconVDataOffset : unsigned(RECON_UV_DATA_OFS_WIDTH-1 downto 0);

  -- Calculated value
  signal rpi_value        : signed(31 downto 0);

  -- Handshake signals
  signal s_in_request : std_logic;
  signal s_out_valid  : std_logic;

  -- Divider signals
  signal s_divider_in_request    : std_logic;
  signal s_divider_in_valid      : std_logic;
  signal s_divider_dividend      : unsigned(RPI_POS_WIDTH-1 downto 0);
  signal s_divider_divisor       : unsigned(RPI_POS_WIDTH-1 downto 0);
  signal s_divider_out_requested : std_logic;
  signal s_divider_out_valid     : std_logic;
  signal s_divider_quotient      : unsigned(RPI_POS_WIDTH-1 downto 0);
  signal s_divider_remainder     : unsigned(RPI_POS_WIDTH-1 downto 0);

  
begin  -- a_ReconPixelIndex

  divider0: divider
    generic map (WIDTH  => RPI_POS_WIDTH)
    port map(Clk            => Clk,
             Reset_n        => Reset_n,
             in_request     => s_divider_out_requested,
             in_valid       => s_divider_out_valid,
             dividend       => s_divider_dividend,
             divisor        => s_divider_divisor,
             out_requested  => s_divider_in_request,
             out_valid      => s_divider_in_valid,
             quotient       => s_divider_quotient,
             remainder      => s_divider_remainder);

  in_request <= s_in_request;
  out_valid <= s_out_valid;
  process(clk)

    procedure ReadIn is
    begin
      s_in_request <= '1';
      s_out_valid <= '0';

      if (s_in_request = '1' and in_valid = '1') then

        case read_state is
          when stt_read_HFragments =>
            read_state <= stt_read_YPlaneFragments;
            HFragments <= unsigned(in_data(HV_FRAG_WIDTH-1 downto 0));

          when stt_read_YPlaneFragments =>
            read_state <= stt_read_YStride;
            YPlaneFragments <= unsigned(in_data(Y_PL_FRAG_WIDTH-1 downto 0));

          when stt_read_YStride =>
            read_state <= stt_read_UVPlaneFragments;
            YStride <= unsigned(in_data(Y_STRIDE_WIDTH-1 downto 0));
            
          when stt_read_UVPlaneFragments =>
            read_state <= stt_read_UVStride;
            UVPlaneFragments <= unsigned(in_data(UV_PL_FRAG_WIDTH-1 downto 0));

          when stt_read_UVStride =>
            read_state <= stt_read_VFragments;
            UVStride <= unsigned(in_data(UV_STRIDE_WIDTH-1 downto 0));

          when stt_read_VFragments =>
            read_state <= stt_read_ReconYDataOffset;
            VFragments <= unsigned(in_data(HV_FRAG_WIDTH-1 downto 0));

          when stt_read_ReconYDataOffset =>
            read_state <= stt_read_ReconUDataOffset;
            ReconYDataOffset <= unsigned(in_data(RECON_Y_DATA_OFS_WIDTH-1 downto 0));

          when stt_read_ReconUDataOffset =>
            read_state <= stt_read_ReconVDataOffset;
            ReconUDataOffset <= unsigned(in_data(RECON_UV_DATA_OFS_WIDTH-1 downto 0));

          when stt_read_ReconVDataOffset =>
            read_state <= stt_read_Position;
            ReconVDataOffset <= unsigned(in_data(RECON_UV_DATA_OFS_WIDTH-1 downto 0));

          when others =>                -- stt_read_Position
            read_state <= stt_read_Position;
            state <= stt_Proc;
            proc_state <= stt_proc_1;
            rpi_position <= unsigned(in_data(RPI_POS_WIDTH-1 downto 0));
            s_in_request <= '0';
        end case;
      end if;
    end procedure ReadIn;

    procedure Proc is
    begin
      s_divider_out_valid <= '0';
      s_divider_in_request <= '0';
      case proc_state is
        when stt_proc_1 =>
          if (s_divider_out_requested = '1') then
            s_divider_out_valid <= '1';
            s_divider_in_request <= '1';
            proc_state <= stt_proc_2;
            if (rpi_position < YPlaneFragments) then
              s_divider_dividend <= rpi_position;
              s_divider_divisor <= resize(HFragments, RPI_POS_WIDTH);
              rpi_value <= resize(signed('0' & ReconYDataOffset), 32);

            elsif (rpi_position < YPlaneFragments + UVPlaneFragments) then
              s_divider_dividend <= resize(rpi_position - YPlaneFragments, RPI_POS_WIDTH);
              s_divider_divisor <= resize(SHIFT_RIGHT(HFragments, 1), RPI_POS_WIDTH);
              rpi_value <= resize(signed('0' & ReconUDataOffset) , 32);
            else
              s_divider_dividend <= resize(rpi_position - (YPlaneFragments + UVPlaneFragments), RPI_POS_WIDTH);
              s_divider_divisor <= resize(SHIFT_RIGHT(HFragments, 1), RPI_POS_WIDTH);
              rpi_value <= resize(signed('0' & ReconVDataOffset), 32);
            end if;
          end if;



        when others =>
          s_divider_in_request <= '1';
          if (s_divider_in_request = '1' and s_divider_in_valid = '1') then
            s_divider_in_request <= '0';
            proc_state <= stt_proc_1;
            state <= stt_WriteOut;
            
            if (rpi_position < YPlaneFragments) then
              rpi_value <= rpi_value +
                           resize(signed('0' &
                                  (s_divider_quotient * VFRAGPIXELS * YStride +
                                   s_divider_remainder * HFRAGPIXELS)), 32);
            else
              rpi_value <= rpi_value +
                           resize(signed('0' &
                                  (s_divider_quotient * VFRAGPIXELS * UVStride +
                                   s_divider_remainder * HFRAGPIXELS)), 32);

            end if;
            
          end if;
      end case;
    end procedure Proc;
        
    procedure WriteOut is
    begin
      s_out_valid <= '1';
      out_data <= rpi_value;
      if (out_requested = '1') then
        state <= stt_readIn;
      end if;
    end procedure WriteOut;


    
  begin
    if (clk'event and clk = '1') then
      if (Reset_n = '0') then
        state <= stt_readIn;
        read_state <= stt_read_HFragments;
        proc_state <= stt_proc_1;
        s_in_request <= '0';
        s_out_valid <= '0';
      else
        case state is
          when stt_readIn => ReadIn;
          when stt_Proc => Proc;
          when others => WriteOut;
        end case;
      end if;
    end if;

  end process;
  
end a_ReconPixelIndex;
