float exc_table[64][8] = {{-1.04159,1.21534,0.0281572,0.567357,-0.66105,0.917141,-0.44328,0.644215},
{2.14338,-0.724956,-0.243856,0.0636575,0.051804,-0.0366771,0.277851,0.0973481},
{0.689252,-0.0152788,0.0669421,0.6023,-1.23989,-0.454837,1.50022,0.387449},
{-0.655571,-0.28521,1.32617,0.833705,-0.876822,0.231166,0.878329,-0.172353},
{1.56631,1.21942,0.487014,0.112435,-0.399279,-0.521386,-0.351448,0.247221},
{0.681692,-0.551779,0.437978,-0.382249,0.616708,0.68908,-1.47909,1.10475},
{-1.24841,0.670471,0.137208,0.489972,1.01366,0.587233,0.681082,-0.681514},
{0.690688,-0.756335,0.775508,0.178926,-0.797518,1.39874,-0.958312,0.48669},
{0.0736325,0.563406,-0.374662,1.00672,-1.22035,0.873397,0.854658,-0.733435},
{1.2404,0.125571,0.202958,-1.46055,0.943786,0.112168,0.523025,-0.122483},
{-0.312712,-0.264677,0.510043,1.46644,1.14451,-0.0998857,-0.692069,-0.0233302},
{0.281634,-0.271152,0.194237,0.235868,0.945309,1.42704,-0.26301,-1.02114},
{0.261912,1.41646,-1.15821,-0.263153,0.994369,-0.144499,0.515897,-0.428628},
{-0.495821,0.798849,0.383719,-1.00158,1.36995,-0.762316,0.784773,-0.0896762},
{0.476335,0.29456,0.104288,0.0255712,-0.0095248,0.274596,1.49057,-1.46249},
{0.125313,0.0868791,0.0358873,0.357145,-0.544518,0.895025,-1.26285,1.62109},
{0.126897,0.313613,-0.756072,1.75117,-0.952912,0.119785,-0.0697364,0.628111},
{1.10276,-0.980415,-0.416322,1.38715,-0.0291589,-0.669674,0.49797,0.376363},
{-0.0167108,0.672873,-0.64257,0.133314,1.01199,-1.46729,1.05946,0.320406},
{0.1257,0.800025,1.20711,-0.5645,-1.00903,-0.356976,0.467638,1.05267},
{1.14128,1.02441,-0.584344,-0.8726,-0.41885,0.114374,0.691641,0.916212},
{-0.022079,0.1634,0.992857,-0.974776,-0.177774,1.37748,-0.912142,0.627073},
{-0.172577,1.09201,-0.0479552,-1.1333,1.04384,0.647798,-0.790628,0.533184},
{0.0223689,-0.927858,1.79641,-0.442921,-0.260088,0.338522,-0.210261,0.80733},
{1.16543,-0.831543,0.0314858,1.16987,-0.961704,0.687389,0.308519,-0.469408},
{1.15331,-1.17282,0.685973,-0.338185,1.00024,-0.644017,-0.126828,0.779918},
{-0.141278,0.254503,-0.274451,0.196185,-0.299476,-0.341986,0.772743,2.04174},
{0.509146,-0.122004,0.900112,-0.469538,-1.11347,1.35141,0.557897,-0.411944},
{0.293851,0.994832,-1.11123,0.405403,0.313717,0.518397,-1.05291,1.03034},
{0.112151,-0.130469,0.55345,-0.564969,0.823356,-1.30379,0.573645,1.387},
{1.20244,1.24831,1.36317,1.41022,1.38998,1.36448,1.28507,1.1929},
{1.54916,-0.0432367,-0.190724,-0.53311,-0.147609,1.38607,-0.658561,0.129495},
{-0.279107,1.51921,-1.28467,0.926618,-0.375457,0.412457,0.102877,0.0269284},
{1.03453,-1.26679,1.10359,0.0725896,-0.196979,-0.270971,0.969828,-0.177658},
{-0.200594,0.246468,0.381804,-0.235524,0.133145,-0.862605,1.98216,-0.236907},
{-1.35127,0.587961,1.3321,-0.178175,0.319357,-0.303824,0.099813,0.754535},
{0.45814,1.09232,1.21856,0.832085,0.258844,-0.183363,-0.615538,-0.699842},
{-0.090229,0.0868827,0.409788,-0.352685,0.530318,-0.220869,-0.985997,1.94863},
{-0.828624,1.43615,-0.511756,0.687363,0.0400852,-0.61435,1.07193,-0.282428},
{1.31246,-1.51496,0.653514,0.427736,-0.304515,0.428299,-0.442741,0.613971},
{0.374916,0.312996,-0.802911,1.2433,-0.179408,-0.640173,1.34948,-0.641773},
{1.23106,-1.09148,1.18843,-0.919555,0.344335,0.503526,-0.246095,0.218774},
{0.0365025,-0.654073,0.969715,1.00994,-0.520716,-0.574648,-0.257959,1.36526},
{0.445859,1.28031,-0.45128,-0.717641,-0.0283588,1.35575,0.111841,-0.634202},
{0.535436,0.595257,-1.44207,1.02815,0.624512,-0.82565,0.241634,0.481186},
{-0.461299,0.734322,1.24596,-1.37214,0.147082,0.654636,0.416385,-0.212952},
{0.894284,-0.366136,0.0767162,-0.0555811,1.20647,-1.06341,1.06868,-0.515867},
{0.336375,0.430788,-0.554035,0.848724,-1.03638,1.48748,-0.744741,0.332997},
{1.36569,0.377193,-1.35813,0.961464,-0.235192,0.396929,0.107201,-0.332525},
{-0.537131,-0.996903,-0.214434,0.81525,0.987716,0.889586,0.721483,0.615579},
{-0.325722,0.0916103,0.279179,1.05899,0.0228423,-1.51322,0.845647,0.757875},
{-0.46886,1.5701,0.253309,-0.511086,-0.752487,0.289817,1.10434,-0.125427},
{0.847028,-0.298864,-1.14845,-0.197791,0.234586,0.933259,1.26716,0.171616},
{0.0795572,-0.212621,1.0749,-1.35585,1.13205,-0.02539,-0.501638,0.872285},
{-0.840241,1.67483,-0.377716,-0.252819,0.660874,-0.493493,-0.0227003,0.806073},
{1.21971,-0.542819,0.496746,-0.577628,-0.266686,0.256195,-0.347802,1.69556},
{0.545959,-0.343555,0.504532,0.674489,-1.68592,0.794496,0.0466908,0.770471},
{0.344366,-0.648986,1.37931,-0.924877,0.955779,-0.634533,0.746397,-0.134892},
{0.921527,-0.254106,-0.509001,0.602565,0.589816,-0.868366,-0.474392,1.53031},
{-0.254479,-0.302833,0.0283111,-0.725651,0.275966,1.22451,1.13252,1.20505},
{1.32048,-0.00804473,-0.999356,-0.0684118,1.3133,0.239322,-0.585385,0.183382},
{0.0945416,0.418628,-0.342742,-0.437575,1.70389,-0.795247,-0.449866,0.942269},
{0,0,0,0,0,0,0,0},
{1.34164,-0.56087,-0.574784,0.94598,-0.454148,0.619891,-0.828585,0.901291}};
