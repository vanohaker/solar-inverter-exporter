window.onload = function(){
    var seconds = 5;
    var rebootUrl = window.location.protocol + "//" + window.location.host + "/" + "doreboot"
    var counterinterval = setInterval(dateinconeole, 1000);
    function dateinconeole() {
        var el = document.getElementById('counter');
        seconds -= 1
        el.innerHTML = seconds;
        console.log(seconds);
        if (seconds == 0 ){
            clearInterval(counterinterval);
            console.log(rebootUrl);
            httpGet(rebootUrl);
        }
    }
};

function httpGet(theUrl)
{
    var xmlHttp = new XMLHttpRequest();
    xmlHttp.open( "GET", theUrl, false ); // false for synchronous request
    xmlHttp.send( null );
    xmlHttp.ontimeout = (e) => {
        
    }
}