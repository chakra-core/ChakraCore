function isKnownGlobal(name) {
  var knownGlobals = [
    "isKnownGlobal",
    "SCA",
    "ImageData",
    "read",
    "WScript",
    "print",
    "read",
    "readbuffer",
    "readline",
    "console",
  ];
  return knownGlobals.includes(name);
}
