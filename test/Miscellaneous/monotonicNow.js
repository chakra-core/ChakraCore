const now = WScript.monotonicNow;

// Using <= instead of < because output number precision might be lost
if (now() <= now() && now() <= now()) {
    print("Passed");
} else {
    print("Failed");
}
