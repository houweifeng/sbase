<?php
/* 
 *
 */
    require_once('./include/header.inc.php');
    echo "<div class=\"form\">\n";
	if (!isset($sess_user) || !$sess_user->IS_LOGGED_IN) {
		echo "您需要登陆以后才有相关权限！\n";
		echo "如果您没有帐号,请您联系<A HREF=\"mailto:" . WEBMASTER_EMAIL . "\">" . WEBMASTER_EMAIL . "</a>\n<br>\n";
    		echo "</div><br />\n";
	}else{
		echo "您已经登录入系统";
		echo "\n</div><br />\n";
	}
    require_once('./include/footer.inc.php');
?>
