background = new Array('#eeeeee','#dddddd','#cccccc','#bbbbbb','#aaaaaa','#999999','#888888','#777777','#666666','#555555');
foreground = new Array('#0000f0','#3366cc','#ff9900','#888888');
foreground_dimmed = new Array('#303040','#303041','#806050','#333330');
var dimmy_val = -1;
var undimmy_val = -1;

function dimfg() {
    if (document.styleSheets) {
        for (var i=0; i<document.styleSheets.length; i++) {
            var styleSheet=document.styleSheets[i];
            var ii=0;
            var cssRule=false;
            do {
                cssRule = styleSheet.cssRules[ii];
                if(!cssRule && styleSheet.rules)
                    cssRule = styleSheet.rules[ii];
                if (cssRule)
                    if (cssRule.style){
                        if(cssRule.style.color){
                            for (var j=0; j<foreground.length; j++) {
                                var temp = cssRule.style.color;
                                cssRule.style.color = foreground[j];
                                if (temp == cssRule.style.color){
                                    cssRule.style.color = foreground_dimmed[j];
                                    break;
                                }
                                cssRule.style.color = temp;
                            }
                        }
                    }
                ii++;
            } while (cssRule);
        }

        document.getElementById("xiphimage").style.opacity=".5";
        document.getElementById("donate").style.opacity=".25";
    }
}

function undimfg() {
    if (document.styleSheets) {

        document.getElementById("xiphimage").style.opacity="1";
        document.getElementById("donate").style.opacity="1";

        for (var i=0; i<document.styleSheets.length; i++) {
            var styleSheet=document.styleSheets[i];
            var ii=0;
            var cssRule=false;
            do {
                cssRule = styleSheet.cssRules[ii];
                if(!cssRule && styleSheet.rules)
                    cssRule = styleSheet.rules[ii];
                if (cssRule)
                    if (cssRule.style){
                        if(cssRule.style.color){
                            for (var j=0; j<foreground.length; j++) {
                                var temp = cssRule.style.color;
                                cssRule.style.color = foreground_dimmed[j];
                                if (temp == cssRule.style.color){
                                    cssRule.style.color = foreground[j];
                                    break;
                                }
                                cssRule.style.color = temp;
                            }
                        }
                    }
                ii++;
            } while (cssRule);
        }
    }
}


function dimmore(){
    if(dimmy_val>=0){
        dimmy_val++;
        document.body.style.background = background[dimmy_val];
        if(dimmy_val < 9) setTimeout(dimmore,20);
    }
}

function dimless(){
    if(undimmy_val>=0){
        document.body.style.background = background[undimmy_val];
        setTimeout(dimless,20);
        undimmy_val--;
    }else{
        document.body.style.background = "#ffffff";
    }
}

function dimpage()
{
    if(dimmy_val == -1){
        undimmy_val = -1;
        dimmy_val = 0;
        dimfg();
        dimmore();
    }
}

function undimpage()
{
    if(dimmy_val != -1){
        undimmy_val = dimmy_val;
        dimmy_val = -1;
        undimfg();
        dimless();
    }
}
