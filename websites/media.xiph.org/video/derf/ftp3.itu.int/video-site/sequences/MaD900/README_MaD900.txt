
                 Copyright (C) 1996, 1998, VTEL Corporation

              See accompanying copyright and license information file


Description of the "MaD900" sequence:

The "MaD900" sequence consists of 900 CIF and QCIF image frames.


Video:

Each frame of the video sequence has its own filename, starting with:

  MaD900_0001.yuv  and  MaD900_qcif_0001.yuv

and ending with:

  MaD900_0900.yuv  and  MaD900_qcif_0900.yuv

Each frame is separated into its Y, Cb, and Cr components.  At the beginning
of each CIF frame file, the 352 x 288 values for Y are found.  Following these
values are found the 176 x 144 Cb values, and then the 176 x 144 Cr values.
The file format for QCIF is the same as CIF other than image size parameters.
Each value is stored in a single unsigned byte with the luma information
ranging from 16 (black) to 235 (white), and the chroma information ranging
from 16 to 240. No headers exist in the files.

