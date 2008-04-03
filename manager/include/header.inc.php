<?php
/* 
 *
 */

include_once('./include/h.inc.php');
?> 
<!-- begin header.inc.php -->
<html > 
<head> 
<title><?php echo SITE_NAME ?></title>
  <meta http-equiv="Expires" content="Fri, Jan 01 1900 00:00:00 GMT" />
  <meta http-equiv="Pragma" content="no-cache" />
  <meta http-equiv="Cache-Control" content="no-cache" />
  <meta name="author" content="Lucent. Inc" />
  <meta http-equiv="content-type" content="text/html; charset=UTF-8">
<link rel="stylesheet" href="css/ie.css" type="text/css">
<script src='js/functions.js'></script>
<script src='js/utf.js'></script>
<script src='js/base64.js'></script>
<script src='js/cTable.js'></script>
<script src='js/cPageSplit.js'></script>
<script src='js/draw.js'></script>
</head>
<body  >
<TABLE width=100% border="0" cellpadding="0" cellspacing="0" >
<TR><TD>
	<table width="100%" height="44" border="0" cellpadding="0" cellspacing="0" align=left >
 	<tr>	
	<td   width=10% valign="bottom"><img src="images/logo.gif" border="0"></td>
	<td   valign="bottom">
	<table width="100%" border="0" cellspacing="0" cellpadding="0" valign="bottom" >
        <tr>
	  <td width="22" rowspan="2" valign="bottom"><img src="images/menu_left.gif" width="22" height="22"></td>
	  <td rowspan="2" align="left" valign="bottom"><img src="images/menu_line.gif" width="3" height="22"></td>
          <td width=100% >
            <table border="0" cellpadding="0" cellspacing="0" background="images/menu_bg.gif" height="22" width=100%>
              <tr>
		<?
$menus	= Array();
if(isset($sess_user) && $sess_user->IS_LOGGED_IN && $sess_user->is_permission(DATA_ADMIN))
{
    $menus['数据查询']      = 'query.php';
}
if(isset($sess_user) && $sess_user->IS_LOGGED_IN && $sess_user->is_permission(DATA_ADMIN))
{
    $menus['数据统计']      = 'stats.php';

}
if(isset($sess_user) && $sess_user->IS_LOGGED_IN && $sess_user->is_permission(SYSTEM_ADMIN))
{
    $menus['下载节点管理']      = 'mclient.php';
}
if(isset($sess_user) && $sess_user->IS_LOGGED_IN && $sess_user->is_permission(SYSTEM_ADMIN))
{
    $menus['系统管理']	= 'system.php';
}
		foreach($menus as $text => $url ){?>
		<td height="22" align="left" valign="bottom">
			<a href='<?echo $url?>' class='menu'><strong><?echo $text?></strong></a>
		</td>
        	<td rowspan="2" align="left" valign="bottom"><img src="images/menu_line.gif" width="3" height="22"></td>
		<?}?>
        	<td height="22" width="26" align="right" valign="bottom" >
			<a href="about.asp" class="menu"><strong>关于</strong></a>
		</td>
              </tr>
	   </table>
	  </td>
	 </tr>
	 </table>
	</td>
	</tr>
	</table>
</TD></TR>
<TR><TD>
<table width="100%" border="0" cellspacing="0" cellpadding="0">
  <tr>
    <td height="33" background="images/menu_bg3.gif">
     <?php
         if (isset($sess_user) && $sess_user->IS_LOGGED_IN ) {
                require_once('./include/logged_in.inc.php');

        }else{
                require_once('./include/not_logged_in.inc.php');
        }
     ?>
    </td>
  </tr>
</table>
</TD></TR>
<TR><TD valign="bottom" align=left>

<!--
<table width="100%" height="100%" border="0" cellpadding="0" cellspacing="0" valign="bottom">
  <tr> 
    <td width="163" valign="top" background="images/left_bg.gif" >
	<table width="150" height="100%" border="0" cellpadding="0" cellspacing="0">
        <tr> 
          <td height="19" align="right" valign="top" background="images/left_bg.gif"></td>
        </tr>
        <tr>
          <td align="right" valign="top" background="images/left_bg.gif">&nbsp;&nbsp;
			</font></td>
        </tr>
      </table>
	-->
<!-- end header.inc.php -->
