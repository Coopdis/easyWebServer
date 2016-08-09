var i = 0;
function update() {
    'use strict';
    document.getElementById("main").innerHTML = i;
    i = i + 1;
}
setInterval(update, 500);