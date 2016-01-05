var seconds = 0;
function finish() {
    print('pass');
    if (++seconds < 65) {
        WScript.SetTimeout(finish, 1000);
    }
}
WScript.SetTimeout(finish, 1000);
