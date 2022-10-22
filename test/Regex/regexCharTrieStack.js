//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var testString = "tacgattttatcgcgactagttaatcatcatagcaagtaaaatttgaattatgtcattat\
catgctccattaacaggttatttaattgatactgacgaaattttttcacaatgggttttc\
tagaatttaatatcagtaattgaagccttcataggggtcctactagtatcctacacgacg\
caggtccgcagtatcctggagggacgtgttactgattaaaagggtcaaaggaatgaaggc\
tcacaatgttacctgcttcaccatagtgagccgatgagttttacattagtactaaatccc\
aaatcatactttacgatgaggcttgctagcgctaaagagaatacatacaccaccacatag\
aattgttagcgatgatatcaaatagactcctggaagtgtcagggggaaactgttcaatat\
ttcgtccacaggactgaccaggcatggaaaagactgacgttggaaactataccatctcac\
gcccgacgcttcactaattgatgatccaaaaaatatagcccggattcctgattagcaaag\
ggttcacagagaaagatattatcgacgtatatcccaaaaaacagacgtaatgtgcatctt\
cgaatcgggatgaatacttgtatcataaaaatgtgacctctagtatacaggttaatgtta\
ctcacccacgtatttggtctaattatgttttatttagtgacaatccaatagataaccggt\
cctattaagggctatatttttagcgaccacgcgtttaaacaaaggattgtatgtagatgg\
gcttgatataagatttcggatgtatgggttttataatcgttggagagctcaatcatgagc\
taatacatggatttcgctacctcaccgagagaccttgcatgaagaattctaaccaaaagt\
ttaataggccggattggattgagttaattaagaccttgttcagtcatagtaaaaaccctt\n\
aaattttaccgattgacaaagtgagcagtcgcaataccctatgcgaaacgcctcgatagt\n\
gactaggtatacaaggtttttgagttcctttgaaatagttaactaatttaaaattaatta\n\
acgacatggaaatcacagaacctaatgctttgtaggagttatttatgctgtttactgcct\n\
ctacaaccctaataaagcagtcctaagaatgaaacgcatcttttagttcagaaagtggta\n\
tccagggtggtcaatttaataaattcaacatcgggtctcaggatattcggtcatataatt\n\
tattaagggctcttcgagtcttactctgagtgaaattggaaacagtcatccttttcgttg\n\
tgaggcatcttacaccgctatcgatatacaatgcattccaccgcggtgtcccgtacacaa\n\
ggaaacttgttaccttggggatataagaaaactcacacgtctcattattaaactgagtac\n\
tggaacgcacctcggatctgttgcactggattaaaatccgattatttttaaaaatattca\n\
gtgctagagcatatcaggtctacttttttatctggtatgtaaagcccacggagcgatagt\n\
gagatccttacgactcaacgaaaagttataacataactcccgttagccaaagcccaatcc\n\
\n";
testString = testString + testString + testString;
testString = testString + testString + testString;
testString = testString + testString + testString;
testString = testString + testString + testString;
var seqs = [/a|tttaccct/ig];

Array.prototype.push.call(seqs, false, Array.prototype.concat.call(seqs, seqs, testString));
try {
  for (i in seqs) {
    testString.match(seqs[i]);
  }
  print ("Test should produce Stack over flow but didn't, case may need amending")
}
catch(e) {
  if (e == "Error: Out of stack space") {
    print ("pass")
  } else {
    print ("Wrong error thrown, expected \"Error: Out of stack space\" but recieved \"" + e + "\"");
  }
}
