#include <amlich.h>


struct MONTH_INFO{
	unsigned int N_AL_DT_DL		:5;
	unsigned int T_AL_DT_DL		:4;
	unsigned int SN_CT_AL		:1;
	unsigned int TN_B_THT		:1;
	unsigned int SN_CT_DL		:2;
};

union LUNAR_RECORD
{
 	unsigned int Word;
	struct MONTH_INFO Info;
};


const unsigned int LUNAR_CALENDAR_LOOKUP_TABLE[] =
{
	//2017
	0x1B84,0x0025,0x1A44,0x1065,0x1A86,0x10A7,0x18C8,0x1ACA,0x10EB,0x1B0C,0x112D,0x1B4E,

	//2018
	0x1B6F,0x0390,0x182E,0x1250,0x1870,0x1292,0x18B2,0x18D4,0x12F6,0x1916,0x1138,0x1B59,

	//2019
	0x1B7A,0x039B,0x1839,0x125B,0x1A7B,0x109C,0x1ABD,0x18E1,0x1303,0x1923,0x1145,0x1B66,

	//2020
	0x1B87,0x0828,0x1A48,0x1269,0x1A89,0x108A,0x1AAB,0x18CC,0x10EE,0x1B0F,0x1130,0x1951,

	//2021
	0x1B73,0x0394,0x1832,0x1254,0x1A74,0x1095,0x1AB6,0x18D7,0x12F9,0x1919,0x133B,0x195B,

	//2022
	0x1B7D,0x0021,0x1A3D,0x1061,0x1A81,0x12A3,0x1AC3,0x18E4,0x1306,0x1926,0x1348,0x1968,

	//2023
	0x1B8A,0x002B,0x1A4A,0x104B,0x186C,0x128E,0x1AAE,0x18CF,0x12F1,0x1B11,0x1132,0x1B53,

	//2024
	0x1974,0x0B96,0x1835,0x1257,0x1877,0x1099,0x1ABA,0x18DB,0x12FD,0x1B1D,0x1141,0x1B61,

	//2025
	0x1982,0x0224,0x1842,0x1264,0x1884,0x10A6,0x1AC7,0x18C8,0x12EA,0x190A,0x132C,0x1B4C,

	//2026
	0x1B6D,0x018E,0x1A2D,0x104E,0x1A6F,0x1090,0x18B1,0x1AD3,0x10F4,0x1915,0x1337,0x1B57,

	//2027
	0x1B78,0x0199,0x1A38,0x1259,0x1879,0x129B,0x18BB,0x18DD,0x1301,0x1922,0x1344,0x1B64,

	//2028
	0x1985,0x0A27,0x1A46,0x1267,0x1887,0x12A9,0x18A9,0x18CB,0x12ED,0x190D,0x112F,0x1B50,

	//2029
	0x1B71,0x0192,0x1A31,0x1252,0x1872,0x1294,0x18B4,0x1AD6,0x10F7,0x1B18,0x1139,0x195A,

	//2030
	0x1B7C,0x019D,0x1A3C,0x125D,0x187D,0x12A1,0x1AC1,0x1AE3,0x1104,0x1B25,0x1146,0x1B67,

	//2031
	0x1988,0x002A,0x1A49,0x106A,0x1A6B,0x128C,0x18AC,0x1ACE,0x12EF,0x190F,0x1331,0x1951,

	//2032
	0x1B73,0x0994,0x1834,0x1256,0x1876,0x1298,0x18B8,0x1ADA,0x12FB,0x191B,0x133D,0x1B5D,

	//2033
	0x1981,0x0022,0x1841,0x1062,0x1883,0x12A5,0x18C5,0x1AE7,0x1108,0x1B29,0x134A,0x1B6A,

	//2034
	0x196B,0x038D,0x182B,0x124D,0x186D,0x108F,0x1AB0,0x18D1,0x10F3,0x1B14,0x1335,0x1B55,

	//2035
	0x1976,0x0398,0x1A36,0x1057,0x1A78,0x1099,0x18BA,0x1ADC,0x10FD,0x1921,0x1342,0x1962,

	//2036
	0x1B84,0x0A25,0x1A44,0x1065,0x1A86,0x10A7,0x18C8,0x1ACA,0x10EB,0x190C,0x132E,0x194E,

	//2037
	0x1B70,0x0391,0x1A2F,0x1050,0x1A71,0x1292,0x18B2,0x18D4,0x12F6,0x1916,0x1138,0x1B59,

	//2038
	0x197A,0x039C,0x1A3A,0x105B,0x1A7C,0x129D,0x18BD,0x1AE1,0x1303,0x1923,0x1145,0x1B66,

	//2039
	0x1987,0x0229,0x1A47,0x1068,0x1A89,0x12AA,0x18AA,0x1ACC,0x10ED,0x1B0E,0x112F,0x1950,

  //2040
	0x1B72,0x0993,0x1A33,0x1054,0x1A75,0x1296,0x18B6,0x1AD8,0x10F9,0x1B1A,0x133B,0x195B,

  //2041
	0x1B7D,0x0021,0x183D,0x1261,0x1A82,0x10A3,0x1AC4,0x18E5,0x1307,0x1B27,0x1348,0x1968,

	//2042
	0x1B8A,0x002B,0x1A4A,0x104B,0x186C,0x128E,0x18AE,0x1AD0,0x10F1,0x1B12,0x1333,0x1953,

	//2043
	0x1B75,0x0396,0x1834,0x1256,0x1876,0x1098,0x1AB9,0x18DA,0x10FC,0x1B1D,0x133E,0x1961,

	//2044
	0x1B82,0x0A23,0x1842,0x1264,0x1884,0x10A6,0x1AC7,0x18E8,0x10EA,0x1B0B,0x112C,0x1B4D,

	//2045
	0x1B6E,0x038F,0x1A2D,0x104E,0x1A6F,0x1090,0x18B1,0x1AD3,0x10F4,0x1915,0x1337,0x1957,

	//2046
	0x1B79,0x039A,0x1A38,0x1059,0x1A7A,0x109B,0x1ABC,0x18DD,0x1301,0x1922,0x1344,0x1964,

	//2047
	0x1B86,0x0227,0x1845,0x1267,0x1A87,0x10A8,0x1AA9,0x18CA,0x12EC,0x190C,0x112E,0x1B4F,

	//2048
	0x1970,0x0B92,0x1831,0x1253,0x1A73,0x1094,0x1AB5,0x1AD6,0x10F7,0x1B18,0x1139,0x195A,

	//2049
	0x1B7C,0x019D,0x1A3C,0x105D,0x1A7E,0x12A2,0x1AC2,0x18E3,0x1305,0x1925,0x1347,0x1B67,

	//2050
	0x1988,0x002A,0x1A49,0x106A,0x1A6B,0x108C,0x1AAD,0x18CE,0x12F0,0x1B10,0x1131,0x1B52,

	//2051
	0x1B73,0x0194,0x1833,0x1255,0x1875,0x1297,0x18B7,0x18D9,0x12FB,0x1B1B,0x113C,0x1B5D,

	//2052
	0x1B7E,0x0A21,0x1841,0x1062,0x1883,0x12A5,0x18C5,0x18E7,0x1309,0x1909,0x132B,0x1B4B,

	//2053
	0x1B6C,0x018D,0x1A2C,0x124D,0x186D,0x108F,0x1AB0,0x18D1,0x10F3,0x1B14,0x1135,0x1B56,

	//2054
	0x1B77,0x0398,0x1836,0x1258,0x1878,0x129A,0x18BA,0x1ADC,0x10FD,0x1921,0x1142,0x1B63,

	//2055
	0x1B84,0x0025,0x1A44,0x1265,0x1885,0x12A7,0x18C7,0x1AC9,0x10EA,0x190B,0x132D,0x194D,

	//2056
	0x1B6F,0x0990,0x1A30,0x1251,0x1871,0x1293,0x1AB3,0x18D4,0x12F6,0x1916,0x1138,0x1B59,

	//2057
	0x197A,0x039C,0x183A,0x125C,0x187C,0x129E,0x1ABE,0x1AE2,0x1103,0x1B24,0x1145,0x1B66,

	//2058
	0x1987,0x0229,0x1847,0x1269,0x1889,0x128B,0x18AB,0x1ACD,0x12EE,0x190E,0x1330,0x1950,

	//2059
	0x1B72,0x0193,0x1A32,0x1053,0x1A74,0x1095,0x1AB6,0x18D7,0x12F9,0x1919,0x133B,0x1B5B,

	//2060
	0x197C,0x0B9E,0x183D,0x1261,0x1A82,0x10A3,0x18C4,0x1AE6,0x1107,0x1B28,0x1349,0x1B69,

	//2061
	0x198A,0x022C,0x1A4A,0x106B,0x186C,0x128E,0x18AE,0x18D0,0x12F2,0x1912,0x1334,0x1B54,

	//2062
	0x1B75,0x0196,0x1A35,0x1256,0x1876,0x1098,0x1AB9,0x18DA,0x10FC,0x1B1D,0x1141,0x1B61,

	//2063
	0x1B83,0x0224,0x1A42,0x1063,0x1A84,0x10A5,0x1AC6,0x18E7,0x10E9,0x1B0A,0x112B,0x1B4C,

	//2064
	0x196D,0x0B8F,0x1A2E,0x124F,0x186F,0x1291,0x18B1,0x1AD3,0x10F4,0x1915,0x1337,0x1957,

	//2065
	0x1B79,0x019A,0x1A39,0x125A,0x187A,0x129C,0x1ABC,0x18DD,0x1302,0x1922,0x1344,0x1964,

	//2066
	0x1B86,0x0027,0x1A46,0x1067,0x1A88,0x12A9,0x18A9,0x1ACB,0x10EC,0x1B0D,0x112E,0x1B4F,

	//2067
	0x1970,0x0392,0x1830,0x1252,0x1872,0x1294,0x18B4,0x1AD6,0x12F7,0x1917,0x1339,0x1959,

	//2068
	0x1B7B,0x099C,0x1A3C,0x105D,0x1A7E,0x10A2,0x1AC3,0x1AE4,0x1105,0x1B26,0x1347,0x1967,

	//2069
	0x1B89,0x002A,0x1A49,0x106A,0x1A8B,0x108C,0x18AD,0x1ACF,0x10F0,0x1B11,0x1332,0x1B52,

	//2070
	0x1973,0x0395,0x1833,0x1255,0x1875,0x1297,0x18B7,0x18D9,0x12FB,0x191B,0x133D,0x195D,

	//2071
	0x1B81,0x0222,0x1A3E,0x1262,0x1882,0x12A4,0x18C4,0x18E6,0x1308,0x1908,0x132A,0x194A,

	//2072
	0x1B6C,0x0B8D,0x1A2C,0x104D,0x1A6E,0x108F,0x1AB0,0x18D1,0x10F3,0x1B14,0x1135,0x1956,

	//2073
	0x1B78,0x0399,0x1A37,0x1058,0x1A79,0x129A,0x18BA,0x1ADC,0x10FD,0x1921,0x1142,0x1963,

	//2074
	0x1B85,0x0226,0x1844,0x1266,0x1A86,0x10A7,0x1AC8,0x18C9,0x12EB,0x190B,0x132D,0x194D,

	//2075
	0x1B6F,0x0190,0x1A2F,0x1050,0x1A71,0x1092,0x1AB3,0x1AD4,0x10F5,0x1B16,0x1137,0x1B58,

	//2076
	0x1979,0x0B9B,0x183A,0x125C,0x187C,0x129E,0x18C1,0x18E2,0x1304,0x1B24,0x1145,0x1B66,

	//2077
	0x1987,0x0229,0x1847,0x1269,0x1889,0x108B,0x1AAC,0x18CD,0x12EF,0x1B0F,0x1130,0x1B51,

	//2078
	0x1B72,0x0193,0x1A32,0x1053,0x1A74,0x1095,0x18B6,0x1AD8,0x10F9,0x1B1A,0x113B,0x1B5C,

	//2079
	0x1B7D,0x039E,0x183C,0x125E,0x1881,0x10A2,0x18C3,0x1AE5,0x1106,0x1B27,0x1148,0x1B69,

	//2080
	0x1B8A,0x0A2B,0x184A,0x126C,0x186C,0x128E,0x18AE,0x18D0,0x10F2,0x1B13,0x1134,0x1B55,

	//2081
	0x1B76,0x0397,0x1835,0x1257,0x1A77,0x1098,0x1AB9,0x18DA,0x10FC,0x1B1D,0x1141,0x1B62,

	//2082
	0x1B83,0x0024,0x1A43,0x1264,0x1884,0x12A6,0x18C6,0x1AE8,0x10E9,0x1B0A,0x112B,0x194C,

	//2083
	0x1B6E,0x038F,0x182D,0x124F,0x1A6F,0x1090,0x1AB1,0x18D2,0x12F4,0x1914,0x1336,0x1956,

	//2084
	0x1B78,0x0999,0x1A39,0x105A,0x1A7B,0x109C,0x1ABD,0x1ADE,0x1302,0x1922,0x1344,0x1964,

	//2085
	0x1B86,0x0027,0x1846,0x1268,0x1888,0x12AA,0x1AAA,0x18CB,0x12ED,0x190D,0x132F,0x1B4F,

	//2086
	0x1970,0x0392,0x1830,0x1052,0x1A73,0x1094,0x1AB5,0x18D6,0x12F8,0x1B18,0x1139,0x1B5A,

	//2087
	0x1B7B,0x019C,0x1A3B,0x105C,0x1A7D,0x10A1,0x1AC2,0x18E3,0x1305,0x1925,0x1347,0x1B67,

	//2088
	0x1B88,0x0829,0x1A49,0x106A,0x1A8B,0x108C,0x18AD,0x18CF,0x12F1,0x1911,0x1333,0x1B53,

	//2089
	0x1B74,0x0195,0x1A34,0x1255,0x1875,0x1297,0x18B7,0x18D9,0x10FB,0x1B1C,0x113D,0x1B5E,

	//2090
	0x1B81,0x0223,0x1A41,0x1262,0x1882,0x12A4,0x18C4,0x18E6,0x1308,0x1908,0x112A,0x1B4B,

	//2091
	0x196C,0x038E,0x1A2C,0x124D,0x186D,0x128F,0x18AF,0x1AD1,0x10F2,0x1B13,0x1134,0x1955,

	//2092
	0x1B77,0x0998,0x1A38,0x1259,0x1879,0x129B,0x1ABB,0x18DC,0x12FE,0x1921,0x1142,0x1963,

	//2093
	0x1B85,0x0026,0x1A45,0x1066,0x1A87,0x12A8,0x18C8,0x1ACA,0x12EB,0x190B,0x132D,0x194D,

	//2094
	0x1B6F,0x0190,0x182F,0x1251,0x1871,0x1293,0x18B3,0x1AD5,0x12F6,0x1916,0x1338,0x1B58,

	//2095
	0x1979,0x039B,0x1839,0x105B,0x1A7C,0x109D,0x1ABE,0x1AE2,0x1103,0x1B24,0x1345,0x1B65,

	//2096
	0x1986,0x0A28,0x1847,0x1269,0x1889,0x108B,0x1AAC,0x18CD,0x10EF,0x1B10,0x1331,0x1B51,

	//2097
	0x1972,0x0394,0x1A32,0x1053,0x1A74,0x1095,0x18B6,0x18D8,0x12FA,0x191A,0x133C,0x1B5C,

	//2098
	0x197D,0x0221,0x1A3D,0x125E,0x1881,0x10A2,0x18C3,0x18E5,0x1307,0x1927,0x1349,0x1969,

	//2099
	0x1B8B,0x022C,0x1A4A,0x104B,0x1A6C,0x108D,0x1AAE,0x18CF,0x10F1,0x1B12,0x1133,0x1B54

};

void am_lich(unsigned char SolarDay, unsigned char SolarMonth, unsigned int SolarYear,char* ngay_al,char* thang_al)
{
	unsigned char N_AL_DT_DL;
	unsigned char T_AL_DT_DL;
	unsigned char SN_CT_AL;
	unsigned char TN_B_THT;
	unsigned char N_AL_DT_DL_TT;
	unsigned char T_AL_DT_DL_TT;

	union LUNAR_RECORD lr;
        SolarYear+=2000;

	lr.Word = LUNAR_CALENDAR_LOOKUP_TABLE[(SolarYear-BEGINNING_YEAR)*12+SolarMonth -1];
	N_AL_DT_DL = lr.Info.N_AL_DT_DL;
	T_AL_DT_DL = lr.Info.T_AL_DT_DL;
	SN_CT_AL = lr.Info.SN_CT_AL + 29;
	TN_B_THT =  lr.Info.TN_B_THT;

	lr.Word = LUNAR_CALENDAR_LOOKUP_TABLE[(SolarYear-BEGINNING_YEAR)*12+SolarMonth];
	N_AL_DT_DL_TT = lr.Info.N_AL_DT_DL;
	T_AL_DT_DL_TT = lr.Info.T_AL_DT_DL;

	// Tinh ngay & thang
	if(N_AL_DT_DL == SN_CT_AL && N_AL_DT_DL_TT == 2)
	{
	 	if(SolarDay==1)
		{
		 	*ngay_al = N_AL_DT_DL;
			*thang_al = T_AL_DT_DL;
		}
		else if(SolarDay==31)
		{
		 	*ngay_al = 1;
			*thang_al = T_AL_DT_DL_TT;
		}
		else
		{
		 	*ngay_al = SolarDay - 1;
			if(TN_B_THT)
			{
				*thang_al = T_AL_DT_DL;
			}
			else
			{
			 	*thang_al = T_AL_DT_DL==12?1:(T_AL_DT_DL + 1);
			}
		}
	}
	else
	{
	 	*ngay_al = SolarDay + N_AL_DT_DL - 1;
		if(*ngay_al<= SN_CT_AL)
		{
		 	*thang_al = T_AL_DT_DL;
		}
		else
		{
		 	*ngay_al -= SN_CT_AL;

			*thang_al = T_AL_DT_DL + 1 - TN_B_THT;
			if(*thang_al == 13) *thang_al = 1;
		}
	}
}
