<?php
/* 
 *
 *
 */

function jsescape($str){
	return base64_encode(iconv('GB18030','UTF-8',$str));
}

//print between "<pre>" and "</pre>"
function pre($data){
    echo "<pre>";
    print_r($data);
    echo "</pre>";
    return FALSE;
}

//user permission  online verify
//在线校验用户权限
function is_permission($permission_id){
    global $sess_user;
    if(!$sess_user){return FALSE;}
    //print_r($sess_user) ; exit;
    $permissions = $sess_user->user_permissions;
    //print_r($permissions);
    //echo $permission_id;
    if(array_key_exists($permission_id,$permissions)){
        return  TRUE;
    }
    return FALSE;
}
//let text font color is red
function red($text){
    return  "<b><font color=red>".$text."</font></b>";
}
// makes a random password
function make_rand_number($length = 8){
    // thanks to benjones@superutility.net for this code
    mt_srand((double) microtime() * 1000000);
    for ($i=0; $i < $length; $i++) {
        //$which = rand(1, 3);
        // character will be a digit 2-9
        //if ( $which == 1 )
        $password .= mt_rand(0,9);
        // character will be a lowercase letter
        //elseif ( $which == 2 ) $password .= chr(mt_rand(65, 90));
        // character will be an uppercase letter
        //elseif ( $which == 3 ) $password .= chr(mt_rand(97, 122));
    }
    return $password;
}


#是否联通手机号码
function is_chinaunicom($sms_no){
    if(ereg("13([0-3]{1})([0-9]{8})",$sms_no)){
        return TRUE;
    }
    return FALSE;
}
function is_chinamobile($sms_no){
    if(ereg("13([4-9]{1})([0-9]{8})",$sms_no)){
        return TRUE;
    }
    return FALSE;
}

function unicode2UTF8($string){
    preg_match_all("/&#(.*?);/is",$string,$matches);
    if(count($matches[1])){
        $unicode=$matches[1];
        for($i=0;$i<count($unicode);$i++){
            $pattern[$i]="/&#".$unicode[$i].";/";
            $replace_str[$i]=iconv("UNICODE","GB2312",code2utf($unicode[$i]));
        }
        return preg_replace($pattern,$replace_str,$string);
    }else{
        return $string;
    }
    return $string;
}
function code2utf($num){
  if($num<128)return chr($num);
  if($num<1024)return chr(($num>>6)+192).chr(($num&63)+128);
  if($num<32768)return chr(($num>>12)+224).chr((($num>>6)&63)+128).chr(($num&63)+128);
  if($num<2097152)return chr($num>>18+240).chr((($num>>12)&63)+128).chr(($num>>6)&63+128). chr($num&63+128);
  return '';
 }

function utf8Encode ($source) {
   $utf8Str = '';
   $entityArray = explode ("&#", $source);
   $size = count ($entityArray);
   for ($i = 0; $i < $size; $i++) {
       $subStr = $entityArray[$i];
       $nonEntity = strstr ($subStr, ';');
       if ($nonEntity !== false) {
           $unicode = intval (substr ($subStr, 0, (strpos ($subStr, ';') + 1)));
           // determine how many chars are needed to reprsent this unicode char
           if ($unicode < 128) {
               $utf8Substring = chr ($unicode);
           }
           else if ($unicode >= 128 && $unicode < 2048) {
               $binVal = str_pad (decbin ($unicode), 11, "0", STR_PAD_LEFT);
               $binPart1 = substr ($binVal, 0, 5);
               $binPart2 = substr ($binVal, 5);
          
               $char1 = chr (192 + bindec ($binPart1));
               $char2 = chr (128 + bindec ($binPart2));
               $utf8Substring = $char1 . $char2;
           }
           else if ($unicode >= 2048 && $unicode < 65536) {
               $binVal = str_pad (decbin ($unicode), 16, "0", STR_PAD_LEFT);
               $binPart1 = substr ($binVal, 0, 4);
               $binPart2 = substr ($binVal, 4, 6);
               $binPart3 = substr ($binVal, 10);
          
               $char1 = chr (224 + bindec ($binPart1));
               $char2 = chr (128 + bindec ($binPart2));
               $char3 = chr (128 + bindec ($binPart3));
               $utf8Substring = $char1 . $char2 . $char3;
           }
           else {
               $binVal = str_pad (decbin ($unicode), 21, "0", STR_PAD_LEFT);
               $binPart1 = substr ($binVal, 0, 3);
               $binPart2 = substr ($binVal, 3, 6);
               $binPart3 = substr ($binVal, 9, 6);
               $binPart4 = substr ($binVal, 15);
      
               $char1 = chr (240 + bindec ($binPart1));
               $char2 = chr (128 + bindec ($binPart2));
               $char3 = chr (128 + bindec ($binPart3));
               $char4 = chr (128 + bindec ($binPart4));
               $utf8Substring = $char1 . $char2 . $char3 . $char4;
           }
          
           if (strlen ($nonEntity) > 1)
               $nonEntity = substr ($nonEntity, 1); // chop the first char (';')
           else
               $nonEntity = '';

           $utf8Str .= $utf8Substring . $nonEntity;
       }
       else {
           $utf8Str .= $subStr;
       }
   }

   return $utf8Str;
}
function unhtmlspecialchars( $string ){
       $string = str_replace ( '&amp;', '&', $string );
       $string = str_replace ( '&#039;', '\'', $string );
       $string = str_replace ( '&quot;', '\"', $string );
       $string = str_replace ( '&lt;', '<', $string );
       $string = str_replace ( '&gt;', '>', $string );
       $string = str_replace ( '&nbsp;', ' ', $string );
       $string = str_replace ( '<br>', '\n', $string ); 
       $string = nl2br($string);  
     return $string;
   }
function connectdb($dbname,$dbuser,$dbkey,$db_host=FALSE){
        global $db_use_pconnect;
        $db=new cDatabase;
        if($db_host){
            $db->db_host = $db_host;
        }else{
            $db->db_host = DB_HOST;
        }
        $db->db_username = $dbuser;
        $db->db_password = $dbkey;
        $db->default_db = $dbname;
        $db->db_type = DB_TYPE;
        $db->debug = FALSE;
        $db->connect($db->default_db);
    return $db;
}
#resize image as given source
#更具指定源图片及大小生成缩略图
function imgresize($srcimg,$destimg,$destwidth=false,$destheight=false){
        $pic_filetype=get_filetype($srcimg);
        if(!$pic_filetype){return FALSE;}
        switch ($pic_filetype){
        case '.gif':
            $read_function = 'imagecreatefromgif';
            break;
        case '.jpg':
            $read_function = 'imagecreatefromjpeg';
            break;
        case '.png':
            $read_function = 'imagecreatefrompng';
            break;
        case '.bmp':
            $read_function = 'imagecreatefromwbmp';
            break;
        }
        $imgsize=getimagesize($srcimg);
        #print_r($imgsize);
        $srcwidth=$imgsize[0];
        $srcheight=$imgsize[1];
        if($destwidth && $destheight){
            $imgscale=($destwidth/$destheight);
        }else{
            $destwidth=120;
            $destheight=168;
            $imgscale=(128/168);
        }
        if(($srcwidth/$srcheight)>$imgscale){
            $srcwidth=$srcheight*$imgscale;
        }else{
            $srcheight=$srcwidth/$imgscale;
        }
        #if(!$destwidth){
        #$destwidth=138;
        #$destheight=160;
        $src=$read_function($srcimg);
        if(!$src){return FALSE;}
        $dest=imagecreatetruecolor($destwidth,$destheight);
        @imagecopyresampled($dest,$src,0, 0, 0, 0,$destwidth,$destheight,$srcwidth,$srcheight);
        #echo  $destfullname="$minibasedir/$imgfile";
        #echo "\n";
        switch ($pic_filetype){
                case '.gif':
                   // echo $destfullname;
                   // echo "\n";
                    if(!imagepng($dest,$destimg)){return FALSE;}
                    break;
                case '.jpg':
                    if(!imagejpeg($dest,$destimg)){return FALSE;}
                    break;
                case '.png':
                    if(!imagepng($dest,$destimg)){return FALSE;}
                    break;
                case '.bmp':
                    if(!imagepng($dest,$destimg)){return FALSE;}
                    break;
        }
    return TRUE;
}
#echo warning or error messages as red color
#以红色表示出错或者警告信息
function error($error_string){
    echo "<font color=red ><strong>$error_string</strong></font>";
}
function makedir($dir){
        system("mkdir -p $dir;chmod 777 $dir;");
        return TRUE;
   }
//make logfile write the logs to the given logfile
function makelog($logdir,$log){
        makedir($logdir);
        $logfile=$logdir.date("Y-m-d").".log";
        $fp=fopen($logfile,"a");
        if($fp){
        fputs($fp,$log);
        }
        fclose($fp);
        return TRUE;
   }
function go2url($url)
{   
echo "<script language=JavaScript >
      window.location = '$url';
      </script>";
}
function goback()
{
echo "<script language=JScript >     history.back(-2);
      </script>";
}

function msg_box($msg){ 
        echo "<script language='JScript' >\n
        window.alert('$msg');
         </script>\n";  
        } 
function check_string($str){
	//check if all space
	if(substr_count($str," ")==strlen($str)){
                return FALSE;
        }
	return  TRUE;
}
function is_int_num($i){
	if(!is_numeric($i)){
		return FALSE;
	}
	if(substr_count($i,".")){
		return FALSE;
	}
	if($i<0){
		return FALSE;
	}
	return TRUE;
}

function is_date($date){
    if(!ereg("([0-9]{4})-([0-9]{1,2})-([0-9]{1,2})",$date)){
        return FALSE;
    }
    return TRUE;
}
//Random password generator.
function random_passwd() {
  mt_srand((double) microtime() * 1000000);
  return substr(crypt(mt_rand(1,1000000)),1,8);
}

//format Timestamp
function format_date($timestamp) {
  $Y=substr($timestamp,0,4);
  $M=substr($timestamp,4,2);
  $D=substr($timestamp,6,2);
  $H=substr($timestamp,8,2);
  $I=substr($timestamp,10,2);
  $S=substr($timestamp,12,2);   
  $stamp=mktime($H,$I,$S,$M,$D,$Y);
  return date("F d, Y", $stamp)." at ".date("g:i a T", $stamp);
} 
//取得apache log 用户信息
function get_user_agent($user_agent){
    //$user_agent = $_SERVER['HTTP_USER_AGENT'];
    //Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; TencentTraveler )
    //echo $user_agent;
    if(!preg_match("/(.*?)\((.*?)\)/is",$user_agent,$agent_matches)){
        //echo "no matches";
        return FALSE;
    }
    $agent_head = $agent_matches[1];
    $agent_content = $agent_matches[2];
    $agent_arr = explode(';',$agent_content);
    $agents['head']         = $agent_head;
    $agents['compatible']   = $agent_arr[0];
    $agents['browser']      = $agent_arr[1];
    $agents['os']           = $agent_arr[2];
    $agents['other']        = $agent_arr[3];
    return $agents;
}
//get apache log user agent
//取得apache log 用户信息
function get_user_agents($user_agent){
    //$user_agent = $_SERVER['HTTP_USER_AGENT'];
    //Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; TencentTraveler )
    //echo $user_agent;
    if(!preg_match("/(.*?)\((.*?)\)/is",$user_agent,$agent_matches)){
        //echo "no matches";
        return FALSE;
    }
    $agent_head = $agent_matches[1];
    $agent_content = $agent_matches[2];
    $agent_arr = explode(';',$agent_content);
    $agents['head']         = $agent_head;
    $agents['compatible']   = $agent_arr[0];
    $agents['browser']      = $agent_arr[1];
    $agents['os']           = $agent_arr[2];
    $agents['other']        = $agent_arr[3];
    return $agents;
}

// html <a href> wrapper
function make_href($url, $text, $title = '', $target='')
{
	$link = "<a href=\"$url\"";
    if ($title) $link .= " title=\"$title\"";
    if ($target) $link .= " target=$target";
    $link .= ">$text</a>";
    return $link;
}

// http redirect wrapper
function redirect($url)
{
    header('Location: ' . ABSOLUTE_PATH . $url);
    exit;
}

// http redirect wrapper
function pure_redirect($url)
{
    header('Location: ' . $url);
    exit;
}


// alternates returning table_light and table_dark
function table_highlight($counter)
{
    if (($counter % 2) == 0) return TABLE_LIGHT;
    else return TABLE_DARK;
}


// self explanatory I hope
function am_pm($hour)
{   
    return ($hour < 12) ? 'am' : 'pm';
}

// converts dates between different formats.
// It expects a date that strototime() understands. 
function convert_date($date, $format = DATE_FORMAT)
{   
    // $date_format is defined in config.inc.php.
    if (empty($date)) return;

    switch ($format) {
        case 'us12':
            return date('m/d/y g:i a', strtotime($date));
            break;
        case 'us24':
			return date('m/d/y H:i', strtotime($date));
            break;
        case 'eu12':
            return date('d/m/y g:i a', strtotime($date));
            break;
        case 'eu24':
            return date('d/m/y H:i', strtotime($date)); 
            break;
        case 'verbose':
            return date('D M jS, Y @ g:ia', strtotime($date));
            break;
        case 'mysql':
            return date('Y-m-d H:i:s', strtotime($date));
            break;
        case 'mysql_hours':
            return date('H:i a', strtotime($date));
            break;
		case 'unix':
            return date('U', strtotime($date));
            break;
        default:
            // none specified, just return us12.
            return date('m/d/y h:i a', strtotime($date));
    }

}

// returns an array with a list of all ip
function get_all_ip()
{   
    global $db,$strings;
    $count = 0;
    $result = $db->query('SELECT ip_id,ipaddr FROM ipmap ORDER BY ipaddr ASC');

    while (@extract($db->fetch_array($result), EXTR_PREFIX_ALL, 'db')) {
       	$iplist[$db_ip_id] = $db_ipaddr;
		$count ++;
    }

	$iplist[-1] = $strings[IPMAP_NOTASSIGNED];
    
    return $iplist;
}
// returns an array with a list of all users
function get_all_users($not_include_admin = 0)
{   
    global $db,$strings;
    $count = 0;
    $result = $db->query('SELECT user_id,username FROM users ORDER BY username ASC');

    while (@extract($db->fetch_array($result), EXTR_PREFIX_ALL, 'db')) {
		if(($not_include_admin == 0) || ($db_username != "admin")) {
        	$users[$db_user_id] = $db_username;
			$count ++;
		}
    }

	$users[-1] = $strings[IPMAP_NOTASSIGNED];
    return $users;
}

// returns an array with a list of all users
function get_all_access_level($not_include_admin = 0)
{
	global $strings;
    $users[ACCESS_AD_CUSTOMER] = $strings['ACL_AD_CUSTOMER'];
    $users[ACCESS_BIG_CUSTOMER] = $strings['ACL_BIG_CUSTOMER'];
    $users[ACCESS_VIP_USER] = $strings['ACL_VIP_USER'];
    if(!$not_include_admin)$users[ACCESS_ADMIN] = $strings['ACL_ADMIN'];
    return $users;
}


// test for ip addresses between 1.0.0.0 and 255.255.255.255
function testIP($a, $allowzero=FALSE) {
    $t = explode(".", $a);
    
    if (sizeof($t) != 4)
       return 1;
    
    for ($i = 0; $i < 4; $i++) {
        // first octet may not be 0
        if ($t[0] == 0 && $allowzero == FALSE)
           return 1;
        if ($t[$i] < 0 or $t[$i] > 255)
           return 1;
        if (!is_numeric($t[$i]))
           return 1;
    };
    return 0;
}


// test for port between 0 - 65535
function testPort($a, $allowzero=FALSE) {
        // first octet may not be 0
    if ($a == 0 && $allowzero == FALSE)
        return 1;
    if ($a < 0 or $a > 65535)
        return 1;
    if (!is_numeric($a))
		return 1;
    return 0;
}

// convert ip to long
function myip2long($a) {
    $inet = 0.0;
    $t = explode(".", $a);
    for ($i = 0; $i < 4; $i++) {
        $inet *= 256.0;
        $inet += $t[$i];
    };
    return $inet;
}

// convert revert ip to long
function myip2long_r($a) {
    $inet = 0.0;
    $t = explode(".", $a);
    for ($i = 3; $i >= 0; $i --) {
        $inet *= 256.0;
        $inet += $t[$i];
    };
    return $inet;
}
// convert long to ip
function mylong2ip($n) {
    $t=array(0,0,0,0);
    $msk = 16777216.0;
    $n += 0.0;
    if ($n < 1)
        return('&nbsp;');
    for ($i = 0; $i < 4; $i++) {
        $k = (int) ($n / $msk);
        $n -= $msk * $k;
        $t[$i]= $k;
        $msk /=256.0;
    };
    //$a=join('.', $t);
	$a = $t[3] . "." . $t[2] . "." . $t[1] . "." . $t[0];
    return($a);
}

function mysystem($command, $hide=true) {
	if (!($p=popen("($command)2>&1","r"))) return 126;
	while (!feof($p)) {
		$l=fgets($p,1000);
		if (!$hide) print $l;
	}
	return pclose($p);
}

function pophead() {
?>
<STYLE TYPE="text/css">

.D11
  {
   POSITION:absolute;
   VISIBILITY:hidden;
   Z-INDEX:200;
  }
.D22
  {
   POSITION:absolute;
   VISIBILITY:hidden;
   Z-INDEX:200;
  }
</STYLE>

<SCRIPT LANGUAGE="javaScript" TYPE="text/javascript" >
Xoffset= 5;
Yoffset= 20;
var isNS4=document.layers?true:false;
var isIE=document.all?true:false;
var isNS6=!isIE&&document.getElementById?true:false;
var old=!isNS4&&!isNS6&&!isIE;

var skn;
var pop;

function initThis()
{
  if(isNS4){skn=document.d11;skn=document.d22;}
  if(isIE){skn=document.all.d11.style;pop=document.all.d22.style;}
  if(isNS6){skn=document.getElementById("d11").style;pop=document.getElementById("d22").style;}

}

function staticPopup(_m)
{
    if (isIE)
	{
	  documentWidth =
	    document.body.offsetWidth / 2 + document.body.scrollLeft - 20;
	  documentHeight =
	    document.body.offsetHeight / 2 + document.body.scrollTop - 20;
	}
    else if (isNS6)
	{
	  documentWidth = window.innerWidth / 2 + window.pageXOffset - 20;
	  documentHeight = window.innerHeight / 2 + window.pageYOffset - 20;
	}
    else if (isNS4)
	{
	  documentWidth = self.innerWidth / 2 + window.pageXOffset - 20;
	  documentHeight = self.innerHeight / 2 + window.pageYOffset - 20;
	}
    pop.left = 0;
    pop.top = 0;

  var content="<TABLE BORDER=1 BORDERCOLOR=#FF0000 BGCOLOR=#E0E0E0 CELLPADDING=1 CELLSPACING=0><TD ALIGN=center><FONT COLOR=black SIZE=2>"+_m+"</FONT></TD></TABLE>";
  if(old)
  {
    alert("You have an old web browser:\n"+_m);
	return;
  }
  else
  {
	if(isNS4)
	{
	  pop.document.open();
	  pop.document.write(content);
	  pop.document.close();
	  pop.visibility="visible";
	}
	if(isNS6)
	{
	  document.getElementById("d22").style.position="absolute";
	  document.getElementById("d22").style.left=x;
	  document.getElementById("d22").style.top=y;
	  document.getElementById("d22").innerHTML=content;
	  pop.visibility="visible";
	}
	if(isIE)
	{
	  document.all("d22").innerHTML=content;
	  pop.visibility="visible";
	}
  }
}


function popup(_m,_b)
{
  var content="<TABLE BORDER=1 BORDERCOLOR=#FF0000 CELLPADDING=2 CELLSPACING=0 "+"BGCOLOR="+_b+"><TD ALIGN=center><FONT COLOR=black SIZE=2>"+_m+"</FONT></TD></TABLE>";
  if(old)
  {
    alert("You have an old web browser:\n"+_m);
	return;
  }
  else
  {
	if(isNS4)
	{
	  skn.document.open();
	  skn.document.write(content);
	  skn.document.close();
	  skn.visibility="visible";
	}
	if(isNS6)
	{
	  document.getElementById("d11").style.position="absolute";
	  document.getElementById("d11").style.left=x;
	  document.getElementById("d11").style.top=y;
	  document.getElementById("d11").innerHTML=content;
	  skn.visibility="visible";
	}
	if(isIE)
	{
	  document.all("d11").innerHTML=content;
	  skn.visibility="visible";
	}
  }
}

var x;
var y;
function get_mouse(e)
{
  x=(isNS4||isNS6)?e.pageX:event.clientX+document.body.scrollLeft; 
  y=(isNS4||isNS6)?e.pageY:event.clientY+document.body.scrollLeft; 
  if(isIE&&navigator.appVersion.indexOf("MSIE 4")==-1)
	  y+=document.body.scrollTop;
  skn.left=x+Xoffset;
  skn.top=y+Yoffset;
  if(y>400){skn.top=y-4*Yoffset;skn.left=x+2*Xoffset;}
}


function removeBox()
{
  if(!old)
  {
	skn.visibility="hidden";
  }
}

function removeStaticBox()
{
  if(!old)
  {
	pop.visibility="hidden";
  }
}

if(isNS4)
  document.captureEvents(Event.MOUSEMOVE); 
if(isNS6)
  document.addEventListener("mousemove", get_mouse, true);
if(isNS4||isIE)
  document.onmousemove=get_mouse;

</SCRIPT>
<DIV ID="d11" CLASS="d11"></DIV>
<DIV ID="d22" CLASS="d22"></DIV>
<script>
initThis();
</script>
<?

}

function random_string($length=16, $list="0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"){
   mt_srand((double)microtime()*1000000);
   $newstring="";               
   $max = strlen($list)-1;
    
   if($length>0){
       while(strlen($newstring)<$length){
           $newstring.=$list[mt_rand(0, $max)];
       } 
   }
   return $newstring;
}   

function check_user_input($user_input,$type,$len)
    {
    $user_input=htmlspecialchars($user_input);
    switch($type)
        {
        case "int" : if(!is_numeric($user_input)){
			unset($user_input);
		     }
		     elseif(substr_count($user_input,".")){
			unset($user_input);
		     }
		     elseif(strlen($user_input)>$len){
			$user_input=substr($user_input,0,$len);
		     }
		    settype($user_input,"int");
        break;
        case "float" :if(is_float($user_input) && strlen($user_input)<=$len)
                      {$user_input=mysql_escape_string($user_input);}
                      else{$user_input="";}
        break;
        case "string":if(strlen($user_input)>$len)
                      {$user_input=mysql_escape_string(substr($user_input,0,$len));}
        break;
        }
    return $user_input;
    }
	function get_filetype($filename){
	return $filetype=strtolower(strrchr($filename,"."));;
	}

 function mbox_mystr_rot13($str,$direction = 0) {
    $from = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';
    $to =   'k6h8OGr5PzXQ3DVnbTdfoswpujWSFxmMK0ZBy2qvLlaeYgcR7A9E4HJi1UNtCI';
   if($direction == 0)
      return strtr($str, $from, $to);
   else
      return strtr($str, $to, $from);
   }

   function mbox_encrypt($str) {
      return mbox_mystr_rot13(base64_encode($str));
   }

   function mbox_decrypt($str) {
      return base64_decode(mbox_mystr_rot13($str,1));
  }
  function mk_koboourl($url,$size,$name,$type){
    $string="koboo://hash=";
    $string.=mbox_encrypt($url);
    $string.="&size=".$size;
    $string.="&type=".$type;
    $string.="&name=".$name;
    return $string;
  }
  function get_koboourl($url_strings){
    preg_match("/.*?hash=(.*?)&.*?/is",$url_strings,$matches);
    $hash=$matches[1];
    $url=($hash)?mbox_decrypt($hash):$url_strings;
    return $url;
  }
  function mk_mboxurl($url,$size,$name){
    $string="mbox://hash=";
    $string.=mbox_encrypt($url);
    $string.="&size=$size";
    $string.="&name=$name";
    return $string;
  }
 function get_mboxurl($url_strings){
    preg_match("/.*?hash=(.*?)&.*?/is",$url_strings,$matches);
    $hash=$matches[1];
    $url=($hash)?mbox_decrypt($hash):$url_strings;
    return $url;
 }

?>
