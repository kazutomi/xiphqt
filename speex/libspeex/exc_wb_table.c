float exc_wb_table[128][8]={{0,0,0,0,0,0,0,0},
{-1.03633,-0.598067,0.434577,1.08032,1.51859,0.789043,-0.178551,-0.54917},
{0.304001,0.923823,-0.692259,-1.55388,0.526058,0.611538,0.481522,1.08309},
{-0.0395937,-0.941631,0.548087,0.900493,-0.613452,-0.846422,0.984429,1.44138},
{1.00466,-1.11178,-0.164767,1.61077,-0.153895,-0.896901,0.564655,0.40669},
{-0.197253,0.570781,0.130693,-0.912574,1.74496,-0.911399,0.656065,0.250751},
{-0.552421,1.13952,1.0122,0.213186,0.487877,-1.02474,-0.870557,1.06442},
{1.02968,1.73221,-0.214526,-0.8054,0.335269,-0.425264,-0.57369,0.766483},
{-0.783575,0.720319,-0.345052,-0.743789,1.67356,1.03267,-0.0434878,0.112781},
{-0.591047,1.83683,0.504509,-1.20575,0.375511,0.338137,-0.36274,0.432143},
{-0.0535243,1.73867,1.39265,-0.318134,-0.907516,-0.110963,0.193565,-0.279976},
{0.889124,0.0155015,-0.765889,1.41013,-0.436154,-0.265441,1.24686,-0.873299},
{-0.644664,0.0936137,1.6392,0.372162,-1.35114,-0.241367,0.936308,0.488901},
{1.54768,-0.451705,0.299703,-1.14522,-0.317506,1.00613,-0.0602516,0.873914},
{0.426336,-0.626112,-0.329146,1.19378,0.656525,-1.18469,-0.319269,1.47621},
{-1.36223,-0.791803,0.0625929,0.491553,1.00923,1.08838,0.91873,0.44169},
{1.42367,0.152665,-1.1033,0.471422,1.02241,-0.752209,-0.674225,0.883491},
{0.375788,-0.948336,0.345699,1.1966,-0.0607664,1.19629,0.586635,-1.12236},
{0.550637,1.40832,-1.64741,0.658325,0.54599,-0.0246801,-0.0852093,0.0730009},
{0.505058,-0.305684,-1.12605,-0.558787,1.17137,1.5405,0.581629,-0.0860507},
{1.0247,-0.66466,0.333468,-0.289871,1.05117,-1.25069,0.392119,1.04094},
{0.34235,-0.77537,0.197171,1.93113,0.942771,-0.177548,-0.552188,-0.324675},
{-0.132227,0.0842547,1.14422,-1.65995,0.55817,1.10678,-0.0949896,0.311947},
{1.36473,1.19028,0.780531,0.910884,0.854454,0.554635,0.23845,-0.0516568},
{-0.357525,0.139316,0.324104,-0.0609313,0.279953,-0.962127,0.124862,2.17393},
{-0.544888,1.24128,-0.57203,0.680379,-0.375316,0.896237,-0.676307,1.03254},
{-0.195987,-0.237371,1.60586,-0.945201,0.660371,-0.750605,0.651732,0.662051},
{1.31943,-0.0456948,-0.979656,0.766511,-0.106572,-1.06806,0.838036,1.02573},
{1.04875,1.56567,0.485407,-0.328864,0.0307096,0.595221,-0.387986,-1.19226},
{-0.615406,1.16911,0.836092,-0.991569,-0.55754,-0.17565,0.843537,1.25318},
{1.15097,0.820474,-0.244995,-1.10569,-0.920918,-0.0718691,0.963504,1.14681},
{-1.34022,-0.360778,1.2518,0.916983,0.166487,-0.427368,0.181518,1.15882},
{1.07359,-0.998889,0.91467,0.205166,-0.753279,1.16532,-0.892312,0.556756},
{-0.187475,1.24065,0.301209,-1.26458,-0.101276,1.48232,0.546266,-0.686011},
{1.9719,0.875912,-0.316355,0.48677,0.20829,-0.482079,-0.428518,-0.527602},
{0.667535,0.638927,-1.48704,0.098544,0.984151,-0.682752,1.14227,0.180132},
{-0.266673,-0.440114,-0.137814,-0.302929,0.0302609,1.78561,1.44693,-0.469607},
{-0.535545,1.50408,-0.44837,1.12612,-0.857486,0.156953,0.771872,-0.272476},
{-0.475877,0.2286,-0.189876,0.633206,0.492666,-1.51294,1.29672,0.938709},
{1.50469,-1.67631,0.504659,0.194321,0.319824,0.0569853,-0.0101858,0.533611},
{0.0293009,-1.34627,0.368322,0.323108,0.634108,1.60474,-0.391468,0.527697},
{-1.54165,0.392745,1.08604,-0.513175,0.808133,0.595405,-0.144862,0.790425},
{-0.817728,0.734455,1.26271,-0.704938,-0.921229,0.910552,1.06223,-0.193172},
{0.543015,-1.28247,-0.730261,0.686474,-0.0443365,0.38415,1.5215,0.585236},
{-0.572683,-0.866989,1.07872,1.68109,-0.327426,-0.218851,0.779091,-0.129686},
{1.06434,0.567725,0.135952,0.458792,-0.376199,-1.42542,-0.150611,1.47526},
{0.443761,0.0876726,0.232774,1.17326,-1.06799,-1.10216,1.3778,0.361752},
{1.03474,0.525746,-0.538619,-1.15767,-0.0206156,1.52656,0.718243,-0.673775},
{1.70035,-0.619964,-1.2712,0.897354,0.489729,-0.158784,0.406952,-0.0478478},
{-0.365051,0.226432,1.82109,-0.627386,-0.756473,0.483177,-0.410344,0.98143},
{1.16776,0.356135,-1.17274,0.261811,0.635242,0.851413,0.575169,-1.23932},
{-0.0839711,1.02839,-0.576034,0.460636,-0.192236,-0.745266,1.87024,-0.474172},
{-0.173316,0.461404,0.944518,1.26763,1.31267,1.02898,0.548032,0.0737827},
{-0.430553,-0.164814,0.0640487,0.468624,1.83374,-0.309847,-0.943881,0.95846},
{0.621088,1.5807,0.0449819,-1.16888,-0.781455,0.516797,1.06907,-0.140764},
{-1.1442,1.44111,-0.402632,-0.0575239,1.06291,-0.674973,0.32242,0.74638},
{0.807472,0.147589,1.08147,-0.1285,-1.4431,1.03913,0.597444,-0.664379},
{-0.70017,0.713683,1.946,0.856833,-0.37006,-0.403706,-0.466334,-0.112292},
{-1.01529,1.35764,0.310164,-0.825529,0.411963,0.0294461,1.37026,-0.265291},
{-0.0603154,-0.974908,-0.805054,0.747251,1.43413,1.17733,0.498444,-0.34983},
{-0.316024,1.60226,0.176445,0.756254,0.852603,-0.798242,0.339327,-0.86298},
{0.450913,-1.00346,1.13199,0.961589,-0.626081,-0.27571,-0.574891,1.30058},
{0.141401,0.101393,0.357711,1.01541,-1.60171,1.00424,-0.252998,0.760351},
{1.22157,0.0719505,-0.584999,0.339341,-0.653905,1.44827,-0.907596,0.628404},
{0.334184,-0.108394,-0.546346,-0.379586,-0.458024,-0.0928911,1.47283,1.76012},
{-0.162063,0.694466,1.40184,0.584844,-0.970119,-1.21435,0.0882554,1.02449},
{1.53975,-0.352929,0.381668,0.291988,-1.30129,-0.284057,0.627504,0.98398},
{-0.12692,0.57948,0.381193,-1.09662,1.11488,-0.210008,-0.819789,1.52889},
{0.230278,0.919943,-0.822515,0.132687,1.09833,-1.19091,-0.242295,1.36411},
{-0.550353,-1.31563,-0.0418793,1.43688,0.903282,-0.0705952,0.42134,0.842356},
{1.20049,-0.450979,-0.167785,-0.71386,1.17516,0.672425,-1.11838,0.78385},
{-0.111437,0.365976,0.611356,0.0367128,-1.09808,-0.802827,1.24046,1.55254},
{1.13537,-0.165433,-1.22634,-0.831482,0.0102606,0.942127,1.15616,0.689254},
{-1.25323,-0.14009,1.45747,0.613556,-0.134901,0.920981,0.68534,-0.587489},
{0.539659,0.865729,-0.775736,0.709136,-0.938594,1.41837,0.190293,-0.696013},
{1.72967,0.424288,-0.946193,-0.480369,0.0584857,-0.310042,0.134891,1.29625},
{0.265227,0.0827433,0.274625,-1.09196,-1.04672,1.17188,1.38651,0.579558},
{0.455276,-0.065565,0.0437782,0.252775,0.178328,0.179633,-1.4148,1.86591},
{0.886572,0.664862,0.806079,0.972233,0.931183,0.884471,0.804077,1.05459},
{0.272214,-0.604275,-1.1212,0.0469548,0.971637,0.158346,0.464247,1.68325},
{0.103203,1.71343,-0.980884,-0.61129,0.744114,0.324151,0.707933,-0.666761},
{0.071369,1.33608,0.483294,-0.0719594,1.28015,0.0864999,-1.41304,-0.177506},
{0.782507,1.12221,-0.528724,-1.10764,0.858326,1.20321,-0.655616,-0.383123},
{1.63524,0.150715,-1.38169,-0.306651,1.07264,0.556524,-0.330863,-0.0336586},
{-0.153903,-0.689427,1.67468,0.584019,0.544801,0.471967,-1.15271,0.236626},
{0.831557,0.282444,-1.07431,1.71817,-0.734158,0.229366,-0.160866,0.308841},
{1.67229,1.31603,0.293472,-0.197319,-1.06919,-0.419804,0.227668,0.0428162},
{0.303228,1.71624,-0.358161,-0.268371,-0.195817,-1.06029,0.960674,0.740735},
{-0.0243176,-0.437698,0.560074,0.623133,-1.19685,0.338863,1.83237,-0.38581},
{0.321907,0.26206,-0.53241,0.333653,1.83928,0.835866,-0.824146,-0.802029},
{0.211784,0.800291,0.709358,-0.378766,-1.52014,-0.020897,1.56839,0.393931},
{0.366185,-0.265221,0.590389,-0.115999,0.792554,-0.708673,1.67893,-0.775966},
{1.28195,-1.02721,0.183826,1.16092,-1.18768,0.459846,0.535175,-0.249723},
{-1.14016,-0.482647,0.505459,0.0983983,-0.343117,0.401372,1.57516,1.12304},
{1.147,1.11561,0.394163,-0.122078,0.260613,0.901099,1.25083,0.81966},
{0.768027,0.243178,1.22026,1.43751,-0.718673,-0.382365,0.146484,-0.794759},
{-0.414121,-0.793798,-0.690751,-0.548501,0.521198,1.19776,1.27359,1.05139},
{-1.4403,0.984987,1.04579,0.395742,-0.227709,-0.671357,1.00093,0.231662},
{1.90324,-0.572199,-0.295628,0.0434357,-0.374636,1.17138,0.160386,-0.55977},
{-0.902381,0.328673,-0.659916,0.942192,0.950924,0.300417,1.31454,-0.692702},
{-0.135489,0.215825,-0.0349114,-0.166635,0.971744,1.77956,0.163175,-1.3044},
{0.354055,-0.18386,0.498562,-0.689249,-0.526319,0.350329,0.0709671,2.12052},
{0.310379,-1.24166,1.5971,-0.0620976,-0.789719,0.648818,0.574153,0.353708},
{0.278324,0.420068,-0.924391,1.58532,0.698845,-1.2323,0.466832,0.0930395},
{0.0922649,0.141644,0.616552,-0.725447,0.390275,1.3972,-1.48369,0.775951},
{1.49578,0.488467,1.15625,-0.246306,-0.394096,-0.0366536,-1.12881,0.546197},
{-0.22949,-0.0377284,0.791,1.6313,-0.0334894,-1.42421,-0.16135,0.833672},
{1.50483,0.180136,-0.671553,0.0258795,-0.937478,0.239621,1.51201,-0.145836},
{-0.917939,0.211918,1.16754,1.52107,0.920108,-0.37239,-0.584982,-0.320368},
{0.443091,0.952967,1.33541,0.913925,0.368584,-0.125061,-0.976007,-1.10975},
{-0.836282,1.16436,1.23655,-0.16389,0.382563,1.00944,-0.267548,-0.956219},
{0.450199,1.26403,1.51365,0.863748,0.2038,0.071838,0.550359,0.734373},
{1.69725,1.13771,-0.917116,-0.989909,-0.017037,0.45129,0.311994,0.0179304},
{0.522356,-0.994855,1.26051,-0.7553,0.567629,0.169681,-0.711474,1.29976},
{0.8313,1.4569,1.08096,0.570183,-0.3738,-1.06704,-0.701423,-0.101114},
{0.182684,0.678864,0.292765,0.0516973,-0.477198,0.854649,1.3867,-1.52939},
{0.782839,0.396491,0.346153,1.26843,0.85293,-0.919632,-1.30058,0.202098},
{0.491729,0.965974,-1.0623,-0.503872,1.58353,0.041349,-0.840533,0.624768},
{0.578714,-0.571294,-1.07824,1.0102,1.69957,-0.0820554,-0.455154,0.25529},
{-0.109213,0.339693,0.545612,1.29121,1.02418,0.604465,-0.466522,-1.47354},
{1.21173,-0.552475,0.925762,-1.06558,1.06424,0.0606676,0.43038,-0.305992},
{0.325296,-0.181066,0.984011,-0.493916,-0.436878,1.95134,-0.356173,-0.536515},
{0.706166,1.28099,0.855746,-0.551548,-1.17237,-0.700125,0.213986,1.10533},
{1.6009,-0.903145,-0.44534,0.886835,0.0116842,0.0972698,-0.754092,1.02939},
{-0.0328855,0.184091,0.234735,0.510939,0.826887,1.14413,1.4588,1.17285},
{0.771452,1.20782,0.676191,-1.44507,-0.691949,0.881803,-0.13458,0.378777},
{1.36292,-0.356531,-0.0772159,0.66455,0.851554,0.911799,-1.09627,-0.727242},
{-0.0125277,0.49517,-0.688167,-0.770124,-0.261333,0.683965,2.01034,0.323668}};
