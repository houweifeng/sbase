<?php
/*
 *
 */
include_once('./include/h.inc.php');
//login
if(isset($_POST['op']) && $_POST['op'] == 'login') {
	$username	= (isset($_POST['username']))?mysql_escape_string($_POST['username']):'';
	$password	= (isset($_POST['password']))?mysql_escape_string($_POST['password']):'';
	$obj_user	= new cUser($db,DB_PREFIX);
	if(!$obj_user->authenticate($username,$password)){ 
		$action	= USER_LOGIN_FAILED.",username:".$username.",password:".$password;
		$obj_user->action_log($action,$username);
		die(red(USER_LOGIN_FAILED));
	}else{
		$_SESSION['sess_user']=$obj_user;
		$action = USER_LOGIN_SUCCESSED.",username:".$username.",password:".$password;
		$obj_user->action_log($action,$username);
		go2url('view.php');
	}
}
//logout
if(isset($_GET['op']) && $_GET['op'] == 'logout'){
	$sess_user->action_log(USER_LOGOUT);
	$sess_user->logout();
	session_unregister('sess_user');
	go2url('index.php');
}
//change password
if(isset($_POST['op']) && $_POST['op'] == 'change'){
	$old_password	= (isset($_POST['old_password']))?mysql_escape_string($_POST['old_password']):'';	
	$password1	= (isset($_POST['password1']))?mysql_escape_string($_POST['password1']):'';	
	$password2	= (isset($_POST['password2']))?mysql_escape_string($_POST['password2']):'';	
	if($old_password && $password1 && $password2){
		if($password1 != $password2){
			die(MSG_USER_PASSWORD_CONFIRM_FAILED);
		}else{
			if( !$sess_user->verify($sess_user->user_name,$old_password) ){
				echo red(MSG_OLDPASSWORD_ERROR);
			}elseif(!$sess_user->password_update($sess_user->user_id,
					$sess_user->user_name, $password1) ){
				echo red(MSG_UPDATE_PASSWORD_FAILED);	
			}else{
				//echo red(MSG_UPDATE_PASSWORD_SUCCESSED.":[$password1]");
				go2url('index.php');
			}
		}
	}else{
		die(MSG_ENTER_FULL_DETAIL);
	}
}
//change password form
if(isset($_GET['op']) && $_GET['op'] == 'change'){ 
	require_once('./include/header.inc.php');
	?>
		<div class="form">
		<form action="login.php" method="post">
		<table>
		<tr>
		<td align="right"><?php echo TEXT_USER_OLD_PASSWORD; ?></td>
		<td><input type="password" name="old_password" /></td>
		</tr>
		<tr>
		<td align="right"><?php echo TEXT_USER_NEW_PASSWORD; ?></td>
		<td><input type="password" name="password1" /></td>
		</tr>
		<tr>
		<td align="right"><?php echo TEXT_USER_CONFIRM_NEW_PASSWORD; ?></td>
		<td><input type="password" name="password2" /></td>
		</tr>
		<tr>
		<td>
		<input type="hidden" name="op" value="change" />
		</td>
		<td><input type="submit" name="submit" value="<? echo BUTTON_UPDATE_PASSWORD_SUBMIT; ?>" /></td>
		</tr>
		</table>
		</form>
		</div>
		<?
}
include_once('./include/header.inc.php');
go2url('view.php');
require_once('./include/footer.inc.php');
?>
