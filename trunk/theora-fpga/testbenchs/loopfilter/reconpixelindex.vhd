library std;
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity ReconPixelIndex is
  
  generic (
    RPI_POS_WIDTH           : positive := 17;
    HV_FRAG_WIDTH           : positive := 8;
    Y_STRIDE_WIDTH          : positive := 12;
    UV_STRIDE_WIDTH         : positive := 11;
    Y_PL_FRAG_WIDTH         : positive := 21;
    UV_PL_FRAG_WIDTH        : positive := 19;
    RECON_Y_DATA_OFS_WIDTH  : positive := 17;
    RECON_UV_DATA_OFS_WIDTH : positive := 15
    
    );

  port (
    rpi_position : in unsigned(RPI_POS_WIDTH-1 downto 0);
    HFragments : in unsigned(HV_FRAG_WIDTH-1 downto 0);
    VFragments : in unsigned(HV_FRAG_WIDTH-1 downto 0);
    YStride    : in unsigned(Y_STRIDE_WIDTH-1 downto 0);
    UVStride   : in unsigned(UV_STRIDE_WIDTH-1 downto 0);
    YPlaneFragments : in unsigned(Y_PL_FRAG_WIDTH-1 downto 0);
    UVPlaneFragments : in unsigned(UV_PL_FRAG_WIDTH-1 downto 0);
    ReconYDataOffset : in unsigned(RECON_Y_DATA_OFS_WIDTH-1 downto 0);
    ReconUDataOffset : in unsigned(RECON_UV_DATA_OFS_WIDTH-1 downto 0);
    ReconVDataOffset : in unsigned(RECON_UV_DATA_OFS_WIDTH-1 downto 0);

    rpi_value : out signed(31 downto 0)
    );
end ReconPixelIndex;

architecture a_ReconPixelIndex of ReconPixelIndex is
  constant VFRAGPIXELS : unsigned(3 downto 0) := x"8";
  constant HFRAGPIXELS : unsigned(3 downto 0) := x"8";

begin  -- a_ReconPixelIndex

  rpi_value <=
    resize(
      signed(

        ((rpi_position / HFragments) * VFRAGPIXELS * YStride) +
        ((rpi_position mod HFragments) * HFRAGPIXELS) +
        ReconYDataOffset), 32)
    when rpi_position < YPlaneFragments else
    resize(
      signed(
        (((rpi_position - YPlaneFragments) / SHIFT_RIGHT(HFragments, 1)) * VFRAGPIXELS * UVStride) +
        (((rpi_position - YPlaneFragments) mod SHIFT_RIGHT(HFragments, 1)) * HFRAGPIXELS) +
        ReconUDataOffset), 32)
    when rpi_position < YPlaneFragments + UVPlaneFragments else
    resize(
      signed(
        (((rpi_position - (YPlaneFragments + UVPlaneFragments))/ SHIFT_RIGHT(HFragments, 1)) * VFRAGPIXELS * UVStride) +
        (((rpi_position - (YPlaneFragments + UVPlaneFragments)) mod SHIFT_RIGHT(HFragments, 1)) * HFRAGPIXELS) +
        ReconVDataOffset), 32);

end a_ReconPixelIndex;
