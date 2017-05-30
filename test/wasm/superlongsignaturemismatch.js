//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


var module1src =`
(module
    (func (export "longsig")
        (param $x f32)
        (param $y f32)
        (param $unused0x0000 f32)
        (param $unused0x0001 f32)
        (param $unused0x0002 f32)
        (param $unused0x0003 f32)
        (param $unused0x0004 f32)
        (param $unused0x0005 f32)
        (param $unused0x0006 f32)
        (param $unused0x0007 f32)
        (param $unused0x0008 f32)
        (param $unused0x0009 f32)
        (param $unused0x000a f32)
        (param $unused0x000b f32)
        (param $unused0x000c f32)
        (param $unused0x000d f32)
        (param $unused0x000e f32)
        (param $unused0x000f f32)
        (param $unused0x0010 f32)
        (param $unused0x0011 f32)
        (param $unused0x0012 f32)
        (param $unused0x0013 f32)
        (param $unused0x0014 f32)
        (param $unused0x0015 f32)
        (param $unused0x0016 f32)
        (param $unused0x0017 f32)
        (param $unused0x0018 f32)
        (param $unused0x0019 f32)
        (param $unused0x001a f32)
        (param $unused0x001b f32)
        (param $unused0x001c f32)
        (param $unused0x001d f32)
        (param $unused0x001e f32)
        (param $unused0x001f f32)
        (param $unused0x0020 f32)
        (param $unused0x0021 f32)
        (param $unused0x0022 f32)
        (param $unused0x0023 f32)
        (param $unused0x0024 f32)
        (param $unused0x0025 f32)
        (param $unused0x0026 f32)
        (param $unused0x0027 f32)
        (param $unused0x0028 f32)
        (param $unused0x0029 f32)
        (param $unused0x002a f32)
        (param $unused0x002b f32)
        (param $unused0x002c f32)
        (param $unused0x002d f32)
        (param $unused0x002e f32)
        (param $unused0x002f f32)
        (param $unused0x0030 f32)
        (param $unused0x0031 f32)
        (param $unused0x0032 f32)
        (param $unused0x0033 f32)
        (param $unused0x0034 f32)
        (param $unused0x0035 f32)
        (param $unused0x0036 f32)
        (param $unused0x0037 f32)
        (param $unused0x0038 f32)
        (param $unused0x0039 f32)
        (param $unused0x003a f32)
        (param $unused0x003b f32)
        (param $unused0x003c f32)
        (param $unused0x003d f32)
        (param $unused0x003e f32)
        (param $unused0x003f f32)
        (param $unused0x0040 f32)
        (param $unused0x0041 f32)
        (param $unused0x0042 f32)
        (param $unused0x0043 f32)
        (param $unused0x0044 f32)
        (param $unused0x0045 f32)
        (param $unused0x0046 f32)
        (param $unused0x0047 f32)
        (param $unused0x0048 f32)
        (param $unused0x0049 f32)
        (param $unused0x004a f32)
        (param $unused0x004b f32)
        (param $unused0x004c f32)
        (param $unused0x004d f32)
        (param $unused0x004e f32)
        (param $unused0x004f f32)
        (param $unused0x0050 f32)
        (param $unused0x0051 f32)
        (param $unused0x0052 f32)
        (param $unused0x0053 f32)
        (param $unused0x0054 f32)
        (param $unused0x0055 f32)
        (param $unused0x0056 f32)
        (param $unused0x0057 f32)
        (param $unused0x0058 f32)
        (param $unused0x0059 f32)
        (param $unused0x005a f32)
        (param $unused0x005b f32)
        (param $unused0x005c f32)
        (param $unused0x005d f32)
        (param $unused0x005e f32)
        (param $unused0x005f f32)
        (param $unused0x0060 f32)
        (param $unused0x0061 f32)
        (param $unused0x0062 f32)
        (param $unused0x0063 f32)
        (param $unused0x0064 f32)
        (param $unused0x0065 f32)
        (param $unused0x0066 f32)
        (param $unused0x0067 f32)
        (param $unused0x0068 f32)
        (param $unused0x0069 f32)
        (param $unused0x006a f32)
        (param $unused0x006b f32)
        (param $unused0x006c f32)
        (param $unused0x006d f32)
        (param $unused0x006e f32)
        (param $unused0x006f f32)
        (param $unused0x0070 f32)
        (param $unused0x0071 f32)
        (param $unused0x0072 f32)
        (param $unused0x0073 f32)
        (param $unused0x0074 f32)
        (param $unused0x0075 f32)
        (param $unused0x0076 f32)
        (param $unused0x0077 f32)
        (param $unused0x0078 f32)
        (param $unused0x0079 f32)
        (param $unused0x007a f32)
        (param $unused0x007b f32)
        (param $unused0x007c f32)
        (param $unused0x007d f32)
        (param $unused0x007e f32)
        (param $unused0x007f f32)
        (param $unused0x0080 f32)
        (param $unused0x0081 f32)
        (param $unused0x0082 f32)
        (param $unused0x0083 f32)
        (param $unused0x0084 f32)
        (param $unused0x0085 f32)
        (param $unused0x0086 f32)
        (param $unused0x0087 f32)
        (param $unused0x0088 f32)
        (param $unused0x0089 f32)
        (param $unused0x008a f32)
        (param $unused0x008b f32)
        (param $unused0x008c f32)
        (param $unused0x008d f32)
        (param $unused0x008e f32)
        (param $unused0x008f f32)
        (param $unused0x0090 f32)
        (param $unused0x0091 f32)
        (param $unused0x0092 f32)
        (param $unused0x0093 f32)
        (param $unused0x0094 f32)
        (param $unused0x0095 f32)
        (param $unused0x0096 f32)
        (param $unused0x0097 f32)
        (param $unused0x0098 f32)
        (param $unused0x0099 f32)
        (param $unused0x009a f32)
        (param $unused0x009b f32)
        (param $unused0x009c f32)
        (param $unused0x009d f32)
        (param $unused0x009e f32)
        (param $unused0x009f f32)
        (param $unused0x00a0 f32)
        (param $unused0x00a1 f32)
        (param $unused0x00a2 f32)
        (param $unused0x00a3 f32)
        (param $unused0x00a4 f32)
        (param $unused0x00a5 f32)
        (param $unused0x00a6 f32)
        (param $unused0x00a7 f32)
        (param $unused0x00a8 f32)
        (param $unused0x00a9 f32)
        (param $unused0x00aa f32)
        (param $unused0x00ab f32)
        (param $unused0x00ac f32)
        (param $unused0x00ad f32)
        (param $unused0x00ae f32)
        (param $unused0x00af f32)
        (param $unused0x00b0 f32)
        (param $unused0x00b1 f32)
        (param $unused0x00b2 f32)
        (param $unused0x00b3 f32)
        (param $unused0x00b4 f32)
        (param $unused0x00b5 f32)
        (param $unused0x00b6 f32)
        (param $unused0x00b7 f32)
        (param $unused0x00b8 f32)
        (param $unused0x00b9 f32)
        (param $unused0x00ba f32)
        (param $unused0x00bb f32)
        (param $unused0x00bc f32)
        (param $unused0x00bd f32)
        (param $unused0x00be f32)
        (param $unused0x00bf f32)
        (param $unused0x00c0 f32)
        (param $unused0x00c1 f32)
        (param $unused0x00c2 f32)
        (param $unused0x00c3 f32)
        (param $unused0x00c4 f32)
        (param $unused0x00c5 f32)
        (param $unused0x00c6 f32)
        (param $unused0x00c7 f32)
        (param $unused0x00c8 f32)
        (param $unused0x00c9 f32)
        (param $unused0x00ca f32)
        (param $unused0x00cb f32)
        (param $unused0x00cc f32)
        (param $unused0x00cd f32)
        (param $unused0x00ce f32)
        (param $unused0x00cf f32)
        (param $unused0x00d0 f32)
        (param $unused0x00d1 f32)
        (param $unused0x00d2 f32)
        (param $unused0x00d3 f32)
        (param $unused0x00d4 f32)
        (param $unused0x00d5 f32)
        (param $unused0x00d6 f32)
        (param $unused0x00d7 f32)
        (param $unused0x00d8 f32)
        (param $unused0x00d9 f32)
        (param $unused0x00da f32)
        (param $unused0x00db f32)
        (param $unused0x00dc f32)
        (param $unused0x00dd f32)
        (param $unused0x00de f32)
        (param $unused0x00df f32)
        (param $unused0x00e0 f32)
        (param $unused0x00e1 f32)
        (param $unused0x00e2 f32)
        (param $unused0x00e3 f32)
        (param $unused0x00e4 f32)
        (param $unused0x00e5 f32)
        (param $unused0x00e6 f32)
        (param $unused0x00e7 f32)
        (param $unused0x00e8 f32)
        (param $unused0x00e9 f32)
        (param $unused0x00ea f32)
        (param $unused0x00eb f32)
        (param $unused0x00ec f32)
        (param $unused0x00ed f32)
        (param $unused0x00ee f32)
        (param $unused0x00ef f32)
        (param $unused0x00f0 f32)
        (param $unused0x00f1 f32)
        (param $unused0x00f2 f32)
        (param $unused0x00f3 f32)
        (param $unused0x00f4 f32)
        (param $unused0x00f5 f32)
        (param $unused0x00f6 f32)
        (param $unused0x00f7 f32)
        (param $unused0x00f8 f32)
        (param $unused0x00f9 f32)
        (param $unused0x00fa f32)
        (param $unused0x00fb f32)
        (param $unused0x00fc f32)
        (param $unused0x00fd f32)
        (param $unused0x00fe f32)
        (param $unused0x00ff f32)
        (param $unused0x0100 f32)
        (param $unused0x0101 f32)
        (param $unused0x0102 f32)
        (param $unused0x0103 f32)
        (param $unused0x0104 f32)
        (param $unused0x0105 f32)
        (param $unused0x0106 f32)
        (param $unused0x0107 f32)
        (param $unused0x0108 f32)
        (param $unused0x0109 f32)
        (param $unused0x010a f32)
        (param $unused0x010b f32)
        (param $unused0x010c f32)
        (param $unused0x010d f32)
        (param $unused0x010e f32)
        (param $unused0x010f f32)
        (param $unused0x0110 f32)
        (param $unused0x0111 f32)
        (param $unused0x0112 f32)
        (param $unused0x0113 f32)
        (param $unused0x0114 f32)
        (param $unused0x0115 f32)
        (param $unused0x0116 f32)
        (param $unused0x0117 f32)
        (param $unused0x0118 f32)
        (param $unused0x0119 f32)
        (param $unused0x011a f32)
        (param $unused0x011b f32)
        (param $unused0x011c f32)
        (param $unused0x011d f32)
        (param $unused0x011e f32)
        (param $unused0x011f f32)
        (param $unused0x0120 f32)
        (param $unused0x0121 f32)
        (param $unused0x0122 f32)
        (param $unused0x0123 f32)
        (param $unused0x0124 f32)
        (param $unused0x0125 f32)
        (param $unused0x0126 f32)
        (param $unused0x0127 f32)
        (param $unused0x0128 f32)
        (param $unused0x0129 f32)
        (param $unused0x012a f32)
        (param $unused0x012b f32)
        (param $unused0x012c f32)
        (param $unused0x012d f32)
        (param $unused0x012e f32)
        (param $unused0x012f f32)
        (param $unused0x0130 f32)
        (param $unused0x0131 f32)
        (param $unused0x0132 f32)
        (param $unused0x0133 f32)
        (param $unused0x0134 f32)
        (param $unused0x0135 f32)
        (param $unused0x0136 f32)
        (param $unused0x0137 f32)
        (param $unused0x0138 f32)
        (param $unused0x0139 f32)
        (param $unused0x013a f32)
        (param $unused0x013b f32)
        (param $unused0x013c f32)
        (param $unused0x013d f32)
        (param $unused0x013e f32)
        (param $unused0x013f f32)
        (param $unused0x0140 f32)
        (param $unused0x0141 f32)
        (param $unused0x0142 f32)
        (param $unused0x0143 f32)
        (param $unused0x0144 f32)
        (param $unused0x0145 f32)
        (param $unused0x0146 f32)
        (param $unused0x0147 f32)
        (param $unused0x0148 f32)
        (param $unused0x0149 f32)
        (param $unused0x014a f32)
        (param $unused0x014b f32)
        (param $unused0x014c f32)
        (param $unused0x014d f32)
        (param $unused0x014e f32)
        (param $unused0x014f f32)
        (param $unused0x0150 f32)
        (param $unused0x0151 f32)
        (param $unused0x0152 f32)
        (param $unused0x0153 f32)
        (param $unused0x0154 f32)
        (param $unused0x0155 f32)
        (param $unused0x0156 f32)
        (param $unused0x0157 f32)
        (param $unused0x0158 f32)
        (param $unused0x0159 f32)
        (param $unused0x015a f32)
        (param $unused0x015b f32)
        (param $unused0x015c f32)
        (param $unused0x015d f32)
        (param $unused0x015e f32)
        (param $unused0x015f f32)
        (param $unused0x0160 f32)
        (param $unused0x0161 f32)
        (param $unused0x0162 f32)
        (param $unused0x0163 f32)
        (param $unused0x0164 f32)
        (param $unused0x0165 f32)
        (param $unused0x0166 f32)
        (param $unused0x0167 f32)
        (param $unused0x0168 f32)
        (param $unused0x0169 f32)
        (param $unused0x016a f32)
        (param $unused0x016b f32)
        (param $unused0x016c f32)
        (param $unused0x016d f32)
        (param $unused0x016e f32)
        (param $unused0x016f f32)
        (param $unused0x0170 f32)
        (param $unused0x0171 f32)
        (param $unused0x0172 f32)
        (param $unused0x0173 f32)
        (param $unused0x0174 f32)
        (param $unused0x0175 f32)
        (param $unused0x0176 f32)
        (param $unused0x0177 f32)
        (param $unused0x0178 f32)
        (param $unused0x0179 f32)
        (param $unused0x017a f32)
        (param $unused0x017b f32)
        (param $unused0x017c f32)
        (param $unused0x017d f32)
        (param $unused0x017e f32)
        (param $unused0x017f f32)
        (param $unused0x0180 f32)
        (param $unused0x0181 f32)
        (param $unused0x0182 f32)
        (param $unused0x0183 f32)
        (param $unused0x0184 f32)
        (param $unused0x0185 f32)
        (param $unused0x0186 f32)
        (param $unused0x0187 f32)
        (param $unused0x0188 f32)
        (param $unused0x0189 f32)
        (param $unused0x018a f32)
        (param $unused0x018b f32)
        (param $unused0x018c f32)
        (param $unused0x018d f32)
        (param $unused0x018e f32)
        (param $unused0x018f f32)
        (param $unused0x0190 f32)
        (param $unused0x0191 f32)
        (param $unused0x0192 f32)
        (param $unused0x0193 f32)
        (param $unused0x0194 f32)
        (param $unused0x0195 f32)
        (param $unused0x0196 f32)
        (param $unused0x0197 f32)
        (param $unused0x0198 f32)
        (param $unused0x0199 f32)
        (param $unused0x019a f32)
        (param $unused0x019b f32)
        (param $unused0x019c f32)
        (param $unused0x019d f32)
        (param $unused0x019e f32)
        (param $unused0x019f f32)
        (param $unused0x01a0 f32)
        (param $unused0x01a1 f32)
        (param $unused0x01a2 f32)
        (param $unused0x01a3 f32)
        (param $unused0x01a4 f32)
        (param $unused0x01a5 f32)
        (param $unused0x01a6 f32)
        (param $unused0x01a7 f32)
        (param $unused0x01a8 f32)
        (param $unused0x01a9 f32)
        (param $unused0x01aa f32)
        (param $unused0x01ab f32)
        (param $unused0x01ac f32)
        (param $unused0x01ad f32)
        (param $unused0x01ae f32)
        (param $unused0x01af f32)
        (param $unused0x01b0 f32)
        (param $unused0x01b1 f32)
        (param $unused0x01b2 f32)
        (param $unused0x01b3 f32)
        (param $unused0x01b4 f32)
        (param $unused0x01b5 f32)
        (param $unused0x01b6 f32)
        (param $unused0x01b7 f32)
        (param $unused0x01b8 f32)
        (param $unused0x01b9 f32)
        (param $unused0x01ba f32)
        (param $unused0x01bb f32)
        (param $unused0x01bc f32)
        (param $unused0x01bd f32)
        (param $unused0x01be f32)
        (param $unused0x01bf f32)
        (param $unused0x01c0 f32)
        (param $unused0x01c1 f32)
        (param $unused0x01c2 f32)
        (param $unused0x01c3 f32)
        (param $unused0x01c4 f32)
        (param $unused0x01c5 f32)
        (param $unused0x01c6 f32)
        (param $unused0x01c7 f32)
        (param $unused0x01c8 f32)
        (param $unused0x01c9 f32)
        (param $unused0x01ca f32)
        (param $unused0x01cb f32)
        (param $unused0x01cc f32)
        (param $unused0x01cd f32)
        (param $unused0x01ce f32)
        (param $unused0x01cf f32)
        (param $unused0x01d0 f32)
        (param $unused0x01d1 f32)
        (param $unused0x01d2 f32)
        (param $unused0x01d3 f32)
        (param $unused0x01d4 f32)
        (param $unused0x01d5 f32)
        (param $unused0x01d6 f32)
        (param $unused0x01d7 f32)
        (param $unused0x01d8 f32)
        (param $unused0x01d9 f32)
        (param $unused0x01da f32)
        (param $unused0x01db f32)
        (param $unused0x01dc f32)
        (param $unused0x01dd f32)
        (param $unused0x01de f32)
        (param $unused0x01df f32)
        (param $unused0x01e0 f32)
        (param $unused0x01e1 f32)
        (param $unused0x01e2 f32)
        (param $unused0x01e3 f32)
        (param $unused0x01e4 f32)
        (param $unused0x01e5 f32)
        (param $unused0x01e6 f32)
        (param $unused0x01e7 f32)
        (param $unused0x01e8 f32)
        (param $unused0x01e9 f32)
        (param $unused0x01ea f32)
        (param $unused0x01eb f32)
        (param $unused0x01ec f32)
        (param $unused0x01ed f32)
        (param $unused0x01ee f32)
        (param $unused0x01ef f32)
        (param $unused0x01f0 f32)
        (param $unused0x01f1 f32)
        (param $unused0x01f2 f32)
        (param $unused0x01f3 f32)
        (param $unused0x01f4 f32)
        (param $unused0x01f5 f32)
        (param $unused0x01f6 f32)
        (param $unused0x01f7 f32)
        (param $unused0x01f8 f32)
        (param $unused0x01f9 f32)
        (param $unused0x01fa f32)
        (param $unused0x01fb f32)
        (param $unused0x01fc f32)
        (param $unused0x01fd f32)
        (param $unused0x01fe f32)
        (param $unused0x01ff f32)
        (param $unused0x0200 f32)
        (param $unused0x0201 f32)
        (param $unused0x0202 f32)
        (param $unused0x0203 f32)
        (param $unused0x0204 f32)
        (param $unused0x0205 f32)
        (param $unused0x0206 f32)
        (param $unused0x0207 f32)
        (param $unused0x0208 f32)
        (param $unused0x0209 f32)
        (param $unused0x020a f32)
        (param $unused0x020b f32)
        (param $unused0x020c f32)
        (param $unused0x020d f32)
        (param $unused0x020e f32)
        (param $unused0x020f f32)
        (param $unused0x0210 f32)
        (param $unused0x0211 f32)
        (param $unused0x0212 f32)
        (param $unused0x0213 f32)
        (param $unused0x0214 f32)
        (param $unused0x0215 f32)
        (param $unused0x0216 f32)
        (param $unused0x0217 f32)
        (param $unused0x0218 f32)
        (param $unused0x0219 f32)
        (param $unused0x021a f32)
        (param $unused0x021b f32)
        (param $unused0x021c f32)
        (param $unused0x021d f32)
        (param $unused0x021e f32)
        (param $unused0x021f f32)
        (param $unused0x0220 f32)
        (param $unused0x0221 f32)
        (param $unused0x0222 f32)
        (param $unused0x0223 f32)
        (param $unused0x0224 f32)
        (param $unused0x0225 f32)
        (param $unused0x0226 f32)
        (param $unused0x0227 f32)
        (param $unused0x0228 f32)
        (param $unused0x0229 f32)
        (param $unused0x022a f32)
        (param $unused0x022b f32)
        (param $unused0x022c f32)
        (param $unused0x022d f32)
        (param $unused0x022e f32)
        (param $unused0x022f f32)
        (param $unused0x0230 f32)
        (param $unused0x0231 f32)
        (param $unused0x0232 f32)
        (param $unused0x0233 f32)
        (param $unused0x0234 f32)
        (param $unused0x0235 f32)
        (param $unused0x0236 f32)
        (param $unused0x0237 f32)
        (param $unused0x0238 f32)
        (param $unused0x0239 f32)
        (param $unused0x023a f32)
        (param $unused0x023b f32)
        (param $unused0x023c f32)
        (param $unused0x023d f32)
        (param $unused0x023e f32)
        (param $unused0x023f f32)
        (param $unused0x0240 f32)
        (param $unused0x0241 f32)
        (param $unused0x0242 f32)
        (param $unused0x0243 f32)
        (param $unused0x0244 f32)
        (param $unused0x0245 f32)
        (param $unused0x0246 f32)
        (param $unused0x0247 f32)
        (param $unused0x0248 f32)
        (param $unused0x0249 f32)
        (param $unused0x024a f32)
        (param $unused0x024b f32)
        (param $unused0x024c f32)
        (param $unused0x024d f32)
        (param $unused0x024e f32)
        (param $unused0x024f f32)
        (param $unused0x0250 f32)
        (param $unused0x0251 f32)
        (param $unused0x0252 f32)
        (param $unused0x0253 f32)
        (param $unused0x0254 f32)
        (param $unused0x0255 f32)
        (param $unused0x0256 f32)
        (param $unused0x0257 f32)
        (param $unused0x0258 f32)
        (param $unused0x0259 f32)
        (param $unused0x025a f32)
        (param $unused0x025b f32)
        (param $unused0x025c f32)
        (param $unused0x025d f32)
        (param $unused0x025e f32)
        (param $unused0x025f f32)
        (param $unused0x0260 f32)
        (param $unused0x0261 f32)
        (param $unused0x0262 f32)
        (param $unused0x0263 f32)
        (param $unused0x0264 f32)
        (param $unused0x0265 f32)
        (param $unused0x0266 f32)
        (param $unused0x0267 f32)
        (param $unused0x0268 f32)
        (param $unused0x0269 f32)
        (param $unused0x026a f32)
        (param $unused0x026b f32)
        (param $unused0x026c f32)
        (param $unused0x026d f32)
        (param $unused0x026e f32)
        (param $unused0x026f f32)
        (param $unused0x0270 f32)
        (param $unused0x0271 f32)
        (param $unused0x0272 f32)
        (param $unused0x0273 f32)
        (param $unused0x0274 f32)
        (param $unused0x0275 f32)
        (param $unused0x0276 f32)
        (param $unused0x0277 f32)
        (param $unused0x0278 f32)
        (param $unused0x0279 f32)
        (param $unused0x027a f32)
        (param $unused0x027b f32)
        (param $unused0x027c f32)
        (param $unused0x027d f32)
        (param $unused0x027e f32)
        (param $unused0x027f f32)
        (param $unused0x0280 f32)
        (param $unused0x0281 f32)
        (param $unused0x0282 f32)
        (param $unused0x0283 f32)
        (param $unused0x0284 f32)
        (param $unused0x0285 f32)
        (param $unused0x0286 f32)
        (param $unused0x0287 f32)
        (param $unused0x0288 f32)
        (param $unused0x0289 f32)
        (param $unused0x028a f32)
        (param $unused0x028b f32)
        (param $unused0x028c f32)
        (param $unused0x028d f32)
        (param $unused0x028e f32)
        (param $unused0x028f f32)
        (param $unused0x0290 f32)
        (param $unused0x0291 f32)
        (param $unused0x0292 f32)
        (param $unused0x0293 f32)
        (param $unused0x0294 f32)
        (param $unused0x0295 f32)
        (param $unused0x0296 f32)
        (param $unused0x0297 f32)
        (param $unused0x0298 f32)
        (param $unused0x0299 f32)
        (param $unused0x029a f32)
        (param $unused0x029b f32)
        (param $unused0x029c f32)
        (param $unused0x029d f32)
        (param $unused0x029e f32)
        (param $unused0x029f f32)
        (param $unused0x02a0 f32)
        (param $unused0x02a1 f32)
        (param $unused0x02a2 f32)
        (param $unused0x02a3 f32)
        (param $unused0x02a4 f32)
        (param $unused0x02a5 f32)
        (param $unused0x02a6 f32)
        (param $unused0x02a7 f32)
        (param $unused0x02a8 f32)
        (param $unused0x02a9 f32)
        (param $unused0x02aa f32)
        (param $unused0x02ab f32)
        (param $unused0x02ac f32)
        (param $unused0x02ad f32)
        (param $unused0x02ae f32)
        (param $unused0x02af f32)
        (param $unused0x02b0 f32)
        (param $unused0x02b1 f32)
        (param $unused0x02b2 f32)
        (param $unused0x02b3 f32)
        (param $unused0x02b4 f32)
        (param $unused0x02b5 f32)
        (param $unused0x02b6 f32)
        (param $unused0x02b7 f32)
        (param $unused0x02b8 f32)
        (param $unused0x02b9 f32)
        (param $unused0x02ba f32)
        (param $unused0x02bb f32)
        (param $unused0x02bc f32)
        (param $unused0x02bd f32)
        (param $unused0x02be f32)
        (param $unused0x02bf f32)
        (param $unused0x02c0 f32)
        (param $unused0x02c1 f32)
        (param $unused0x02c2 f32)
        (param $unused0x02c3 f32)
        (param $unused0x02c4 f32)
        (param $unused0x02c5 f32)
        (param $unused0x02c6 f32)
        (param $unused0x02c7 f32)
        (param $unused0x02c8 f32)
        (param $unused0x02c9 f32)
        (param $unused0x02ca f32)
        (param $unused0x02cb f32)
        (param $unused0x02cc f32)
        (param $unused0x02cd f32)
        (param $unused0x02ce f32)
        (param $unused0x02cf f32)
        (param $unused0x02d0 f32)
        (param $unused0x02d1 f32)
        (param $unused0x02d2 f32)
        (param $unused0x02d3 f32)
        (param $unused0x02d4 f32)
        (param $unused0x02d5 f32)
        (param $unused0x02d6 f32)
        (param $unused0x02d7 f32)
        (param $unused0x02d8 f32)
        (param $unused0x02d9 f32)
        (param $unused0x02da f32)
        (param $unused0x02db f32)
        (param $unused0x02dc f32)
        (param $unused0x02dd f32)
        (param $unused0x02de f32)
        (param $unused0x02df f32)
        (param $unused0x02e0 f32)
        (param $unused0x02e1 f32)
        (param $unused0x02e2 f32)
        (param $unused0x02e3 f32)
        (param $unused0x02e4 f32)
        (param $unused0x02e5 f32)
        (param $unused0x02e6 f32)
        (param $unused0x02e7 f32)
        (param $unused0x02e8 f32)
        (param $unused0x02e9 f32)
        (param $unused0x02ea f32)
        (param $unused0x02eb f32)
        (param $unused0x02ec f32)
        (param $unused0x02ed f32)
        (param $unused0x02ee f32)
        (param $unused0x02ef f32)
        (param $unused0x02f0 f32)
        (param $unused0x02f1 f32)
        (param $unused0x02f2 f32)
        (param $unused0x02f3 f32)
        (param $unused0x02f4 f32)
        (param $unused0x02f5 f32)
        (param $unused0x02f6 f32)
        (param $unused0x02f7 f32)
        (param $unused0x02f8 f32)
        (param $unused0x02f9 f32)
        (param $unused0x02fa f32)
        (param $unused0x02fb f32)
        (param $unused0x02fc f32)
        (param $unused0x02fd f32)
        (param $unused0x02fe f32)
        (param $unused0x02ff f32)
        (param $unused0x0300 f32)
        (param $unused0x0301 f32)
        (param $unused0x0302 f32)
        (param $unused0x0303 f32)
        (param $unused0x0304 f32)
        (param $unused0x0305 f32)
        (param $unused0x0306 f32)
        (param $unused0x0307 f32)
        (param $unused0x0308 f32)
        (param $unused0x0309 f32)
        (param $unused0x030a f32)
        (param $unused0x030b f32)
        (param $unused0x030c f32)
        (param $unused0x030d f32)
        (param $unused0x030e f32)
        (param $unused0x030f f32)
        (param $unused0x0310 f32)
        (param $unused0x0311 f32)
        (param $unused0x0312 f32)
        (param $unused0x0313 f32)
        (param $unused0x0314 f32)
        (param $unused0x0315 f32)
        (param $unused0x0316 f32)
        (param $unused0x0317 f32)
        (param $unused0x0318 f32)
        (param $unused0x0319 f32)
        (param $unused0x031a f32)
        (param $unused0x031b f32)
        (param $unused0x031c f32)
        (param $unused0x031d f32)
        (param $unused0x031e f32)
        (param $unused0x031f f32)
        (param $unused0x0320 f32)
        (param $unused0x0321 f32)
        (param $unused0x0322 f32)
        (param $unused0x0323 f32)
        (param $unused0x0324 f32)
        (param $unused0x0325 f32)
        (param $unused0x0326 f32)
        (param $unused0x0327 f32)
        (param $unused0x0328 f32)
        (param $unused0x0329 f32)
        (param $unused0x032a f32)
        (param $unused0x032b f32)
        (param $unused0x032c f32)
        (param $unused0x032d f32)
        (param $unused0x032e f32)
        (param $unused0x032f f32)
        (param $unused0x0330 f32)
        (param $unused0x0331 f32)
        (param $unused0x0332 f32)
        (param $unused0x0333 f32)
        (param $unused0x0334 f32)
        (param $unused0x0335 f32)
        (param $unused0x0336 f32)
        (param $unused0x0337 f32)
        (param $unused0x0338 f32)
        (param $unused0x0339 f32)
        (param $unused0x033a f32)
        (param $unused0x033b f32)
        (param $unused0x033c f32)
        (param $unused0x033d f32)
        (param $unused0x033e f32)
        (param $unused0x033f f32)
        (param $unused0x0340 f32)
        (param $unused0x0341 f32)
        (param $unused0x0342 f32)
        (param $unused0x0343 f32)
        (param $unused0x0344 f32)
        (param $unused0x0345 f32)
        (param $unused0x0346 f32)
        (param $unused0x0347 f32)
        (param $unused0x0348 f32)
        (param $unused0x0349 f32)
        (param $unused0x034a f32)
        (param $unused0x034b f32)
        (param $unused0x034c f32)
        (param $unused0x034d f32)
        (param $unused0x034e f32)
        (param $unused0x034f f32)
        (param $unused0x0350 f32)
        (param $unused0x0351 f32)
        (param $unused0x0352 f32)
        (param $unused0x0353 f32)
        (param $unused0x0354 f32)
        (param $unused0x0355 f32)
        (param $unused0x0356 f32)
        (param $unused0x0357 f32)
        (param $unused0x0358 f32)
        (param $unused0x0359 f32)
        (param $unused0x035a f32)
        (param $unused0x035b f32)
        (param $unused0x035c f32)
        (param $unused0x035d f32)
        (param $unused0x035e f32)
        (param $unused0x035f f32)
        (param $unused0x0360 f32)
        (param $unused0x0361 f32)
        (param $unused0x0362 f32)
        (param $unused0x0363 f32)
        (param $unused0x0364 f32)
        (param $unused0x0365 f32)
        (param $unused0x0366 f32)
        (param $unused0x0367 f32)
        (param $unused0x0368 f32)
        (param $unused0x0369 f32)
        (param $unused0x036a f32)
        (param $unused0x036b f32)
        (param $unused0x036c f32)
        (param $unused0x036d f32)
        (param $unused0x036e f32)
        (param $unused0x036f f32)
        (param $unused0x0370 f32)
        (param $unused0x0371 f32)
        (param $unused0x0372 f32)
        (param $unused0x0373 f32)
        (param $unused0x0374 f32)
        (param $unused0x0375 f32)
        (param $unused0x0376 f32)
        (param $unused0x0377 f32)
        (param $unused0x0378 f32)
        (param $unused0x0379 f32)
        (param $unused0x037a f32)
        (param $unused0x037b f32)
        (param $unused0x037c f32)
        (param $unused0x037d f32)
        (param $unused0x037e f32)
        (param $unused0x037f f32)
        (param $unused0x0380 f32)
        (param $unused0x0381 f32)
        (param $unused0x0382 f32)
        (param $unused0x0383 f32)
        (param $unused0x0384 f32)
        (param $unused0x0385 f32)
        (param $unused0x0386 f32)
        (param $unused0x0387 f32)
        (param $unused0x0388 f32)
        (param $unused0x0389 f32)
        (param $unused0x038a f32)
        (param $unused0x038b f32)
        (param $unused0x038c f32)
        (param $unused0x038d f32)
        (param $unused0x038e f32)
        (param $unused0x038f f32)
        (param $unused0x0390 f32)
        (param $unused0x0391 f32)
        (param $unused0x0392 f32)
        (param $unused0x0393 f32)
        (param $unused0x0394 f32)
        (param $unused0x0395 f32)
        (param $unused0x0396 f32)
        (param $unused0x0397 f32)
        (param $unused0x0398 f32)
        (param $unused0x0399 f32)
        (param $unused0x039a f32)
        (param $unused0x039b f32)
        (param $unused0x039c f32)
        (param $unused0x039d f32)
        (param $unused0x039e f32)
        (param $unused0x039f f32)
        (param $unused0x03a0 f32)
        (param $unused0x03a1 f32)
        (param $unused0x03a2 f32)
        (param $unused0x03a3 f32)
        (param $unused0x03a4 f32)
        (param $unused0x03a5 f32)
        (param $unused0x03a6 f32)
        (param $unused0x03a7 f32)
        (param $unused0x03a8 f32)
        (param $unused0x03a9 f32)
        (param $unused0x03aa f32)
        (param $unused0x03ab f32)
        (param $unused0x03ac f32)
        (param $unused0x03ad f32)
        (param $unused0x03ae f32)
        (param $unused0x03af f32)
        (param $unused0x03b0 f32)
        (param $unused0x03b1 f32)
        (param $unused0x03b2 f32)
        (param $unused0x03b3 f32)
        (param $unused0x03b4 f32)
        (param $unused0x03b5 f32)
        (param $unused0x03b6 f32)
        (param $unused0x03b7 f32)
        (param $unused0x03b8 f32)
        (param $unused0x03b9 f32)
        (param $unused0x03ba f32)
        (param $unused0x03bb f32)
        (param $unused0x03bc f32)
        (param $unused0x03bd f32)
        (param $unused0x03be f32)
        (param $unused0x03bf f32)
        (param $unused0x03c0 f32)
        (param $unused0x03c1 f32)
        (param $unused0x03c2 f32)
        (param $unused0x03c3 f32)
        (param $unused0x03c4 f32)
        (param $unused0x03c5 f32)
        (param $unused0x03c6 f32)
        (param $unused0x03c7 f32)
        (param $unused0x03c8 f32)
        (param $unused0x03c9 f32)
        (param $unused0x03ca f32)
        (param $unused0x03cb f32)
        (param $unused0x03cc f32)
        (param $unused0x03cd f32)
        (param $unused0x03ce f32)
        (param $unused0x03cf f32)
        (param $unused0x03d0 f32)
        (param $unused0x03d1 f32)
        (param $unused0x03d2 f32)
        (param $unused0x03d3 f32)
        (param $unused0x03d4 f32)
        (param $unused0x03d5 f32)
        (param $unused0x03d6 f32)
        (param $unused0x03d7 f32)
        (param $unused0x03d8 f32)
        (param $unused0x03d9 f32)
        (param $unused0x03da f32)
        (param $unused0x03db f32)
        (param $unused0x03dc f32)
        (param $unused0x03dd f32)
        (param $unused0x03de f32)
        (param $unused0x03df f32)
        (param $unused0x03e0 f32)
        (param $unused0x03e1 f32)
        (param $unused0x03e2 f32)
        (param $unused0x03e3 f32)
        (param $unused0x03e4 f32)
        (param $unused0x03e5 f32)
        (param $unused0x03e6 f32)
        (param $unused0x03e7 f32)
        (result f32)
        (f32.add (get_local $x) (get_local $y)))
)
`

var module2src = `
(module
    (import "declmod" "longsig"
        (func $foo 
            (param f32)
            (result f32)
        )
    )
)
`

var buf1 = WebAssembly.wabt.convertWast2Wasm(module1src);
var mod1 = new WebAssembly.Module(buf1);
var ins1 = new WebAssembly.Instance(mod1, {});

var buf2 = WebAssembly.wabt.convertWast2Wasm(module2src);
var mod2 = new WebAssembly.Module(buf2);
try {
var ins2 = new WebAssembly.Instance(mod2, {"declmod":{"longsig":ins1.exports.longsig}});
} catch (err) {
    if(err.message != "Cannot link import longsig(f32)->f32 to longsig(f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f...)->f32 in link table due to a signature mismatch") {
        throw err;
    }
}

var module3src = `
(module
    (import "declmod" "longsig"
        (func $foo 
            (param
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
                f32
            )
            (result f32)
        )
    )
)
`

var buf3 = WebAssembly.wabt.convertWast2Wasm(module3src);
var mod3 = new WebAssembly.Module(buf3);
try {
var ins3 = new WebAssembly.Instance(mod3, {"declmod":{"longsig":ins1.exports.longsig}});
} catch (err) {
    if(err.message != "Cannot link import longsig(f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f3...)->f32 to longsig(f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f32, f...)->f32 in link table due to a signature mismatch") {
        throw err;
    }
}
print("PASSED");
