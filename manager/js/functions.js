current_func	= '';
function func_change(func_id,div_name){
	if(func_id != current_func){
		current_func = func_id;
	}
	var HTML = document.getElementById(func_id).innerHTML;
	if(document.getElementById(div_name)){
		document.getElementById(div_name).innerHTML = HTML;
	}
	
}
function encodeHex(str){
    var result = "";
    for (var i=0; i<str.length; i++){
        result += pad(toHex(str.charCodeAt(i)&0xff),2,'0');
    }
    return result;
}
function decodeHex(str){
    str = str.replace(new RegExp("s/[^0-9a-zA-Z]//g"));
    var result = "";
    var nextchar = "";
    for (var i=0; i<str.length; i++){
        nextchar += str.charAt(i);
        if (nextchar.length == 2){
            result += ntos(eval('0x'+nextchar));
            nextchar = "";
        }
    }
    return result;
    
}
function ntos(n){
    n=n.toString(16);
    if (n.length == 1) n="0"+n;
    n="%"+n;
    return unescape(n);
}
function toHex(n){
    var result = ''
    var start = true;
    for (var i=32; i>0;){
        i-=4;
        var digit = (n>>i) & 0xf;
        if (!start || digit != 0){
            start = false;
            result += digitArray[digit];
        }
    }
    return (result==''?'0':result);
}

function pad(str, len, pad){
    var result = str;
    for (var i=str.length; i<len; i++){
        result = pad + result;
    }
    return result;
}
var Base64Alphabet = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 
    'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/']; 
 
function EncodeBase64(string) 
{ 
    var hexString = string.getGBCode(); 
    window.status = hexString;  
    var base64 = ''; 
    var B64A = Base64Alphabet; 
    var padCount = hexString.length%3; 
    var count = parseInt(hexString.length/6); 
    var i = 0;  
    for ( i=0 ; i < count ; ++i ) 
    { 
         var intA = parseInt(hexString.charAt(i*6+0)+hexString.charAt(i*6+1), 16); 
         var intB = parseInt(hexString.charAt(i*6+2)+hexString.charAt(i*6+3), 16); 
         var intC = parseInt(hexString.charAt(i*6+4)+hexString.charAt(i*6+5), 16);  
         var part1 = intA >> 2; 
         var part2 = ((intA & 0x03) << 4) | (intB >> 4); 
         var part3 = ((intB & 0x0f) << 2) | (intC >> 6); 
         var part4 = intC & 0x3f; 
         base64 += B64A[part1] + B64A[part2] + B64A[part3] + B64A[part4];  
    } 
    if ( padCount == 1 ) 
    { 
         var intA = parseInt(hexString.charAt(i*6+0)+hexString.charAt(i*6+1), 16); 
         var intB = parseInt(hexString.charAt(i*6+2)+hexString.charAt(i*6+3), 16); 
         var part1 = intA >> 2; 
         var part2 = ((intA & 0x03) << 4) | (intB >> 4); 
         var part3 = ((intB & 0x0f) << 2); 
         base64 += B64A[part1] + B64A[part2] + B64A[part3] + '='; 
    } 
    if ( padCount == 2 ) 
    { 
         var intA = parseInt(hexString.charAt(i*6+0)+hexString.charAt(i*6+1), 16); 
         var part1 = intA >> 2; 
         var part2 = ((intA & 0x03) << 4); 
         base64 += B64A[part1] + B64A[part2] + '=='; 
    }         
    return base64; 
} 
String.prototype.getGBCode = function() 
{ 
    return vbGetGBCode(this); 
} 

//input base64 encode
function strdecode(str){
	return utf8to16(base64decode(str));
}
function strencode(str){
	return base64encode(utf16to8(str));
}
