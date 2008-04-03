<?php
/* 
 *
 */
require_once('./include/header.inc.php');
require_once('./include/ctable.inc.php');   
require_once('./include/cform.inc.php');
if(!isset($sess_user) || !$sess_user->is_permission(SYSTEM_ADMIN)){
        echo USER_ADMIN_PERMISSION_NOT_EXISTS;
        exit;
}
//add user
if( isset($_POST['op']) && $_POST['op'] == 'user_add' ){
	$username	= (isset($_POST['username']))?mysql_escape_string($_POST['username']):die(red(FULL_ENTER_USER_DATA));
	$password	= (isset($_POST['password']))?mysql_escape_string($_POST['password']):die(red(FULL_ENTER_USER_DATA));
	$password1	= (isset($_POST['password1']))?mysql_escape_string($_POST['password1']):die(red(FULL_ENTER_USER_DATA));
	$email		= (isset($_POST['email']))?mysql_escape_string($_POST['email']):'';
	$tel      = (isset($_POST['tel']))?mysql_escape_string($_POST['tel']):'';
    $desc		= (isset($_POST['desc']))?mysql_escape_string($_POST['desc']):'';
    $status       = (isset($_POST['status']))?mysql_escape_string($_POST['status']):'';
	($password != $password1)?die(red(TWO_PASSWORD_NOT_IDENTICAL)):null;
	($sess_user->user_exists($username))?die(red(USER_REG_EXISTS)):null;
	(!$sess_user->user_add($username,$password,$email,$desc))?die(USER_REG_FAILED):print(red(USER_REG_SUCCESSED));
	
}
//update user
if( isset($_POST['op']) && $_POST['op'] == 'user_update' ){
        $userid       = (isset($_POST['userid']))?mysql_escape_string($_POST['userid']):die(red(FULL_ENTER_USER_DATA));
        $password       = (isset($_POST['password']))?mysql_escape_string($_POST['password']):'';
        $password1      = (isset($_POST['password1']))?mysql_escape_string($_POST['password1']):'';
        $email          = (isset($_POST['email']))?mysql_escape_string($_POST['email']):'';
        $desc           = (isset($_POST['desc']))?mysql_escape_string($_POST['desc']):'';
        (($password || $password1) && $password != $password1)?die(red(TWO_PASSWORD_NOT_IDENTICAL)):null;
	$data 		=	Array();
	$data['data']	=	Array(
		'user_email'	=> $email,
		'user_desc'	=> $desc,
	);
	($password)?($data['data']['user_password']=md5($password)):null;
	$data['where'] = Array('user_id'	=> $userid);
        (!$sess_user->user_update($data))?die(USER_UPDATE_FAILED):print(red(USER_UPDATE_SUCCESSED));

}

//delete user
if( isset($_GET['op']) && $_GET['op'] == 'user_drop' ){
	if(isset($_GET['user_id'])){
		$user_id	= mysql_escape_string($_GET['user_id']);
	}else{
		die(red(INVALID_USER_ID));
	}
	if(!$sess_user->user_drop($user_id)){
		print(red(USER_DROP_FAILED));
	}else{
		print(red(USER_DROP_SUCCESSED));
	}
}
//add permission 
if( isset($_GET['op']) && $_GET['op'] == 'permission_add' ){
	if(isset($_GET['user_id'])){
		$user_id	=mysql_escape_string($_GET['user_id']);
	}else{
		die(red(INVALID_USER_ID));
	}
	$_user_		= $sess_user->user_exists('',$user_id);
	if(!$_user_){
		die(red(INVALID_USER_ID));
	}else{
		$user_name=$_user_['user_name'];
	}
	if(isset($_GET['permission_id'])){
		$permission_id = mysql_escape_string($_GET['permission_id']);
	}else{
		die(red(INVALID_PERMISSION_ID));
	}
	if(isset($____user_permissions_lists[$permission_id])){
		$permission_name=$____user_permissions_lists[$permission_id]['permission_name'];
	}else{
		die(red(INVALID_PERMISSION_ID));
	}
	if(!$sess_user->user_permission_add($user_id,$user_name,$permission_id,$permission_name)){
		die(PERMISSION_ADD_FAILED);
	}else{
		print(red(PERMISSION_ADD_SUCCESSED));
	}
}
//delete permission
if( isset($_GET['op']) && $_GET['op'] == 'permission_drop' ){
	if(isset($_GET['user_id'])){
		$user_id        = mysql_escape_string($_GET['user_id']);
	}else{
		die(red(INVALID_USER_ID));
	}
	if(isset($_GET['permission_id'])){
		$permission_id = mysql_escape_string($_GET['permission_id']);
	}else{
		die(red(INVALID_PERMISSION_ID));
	}
	if(!isset($____user_permissions_lists[$permission_id])){
		die(red(INVALID_PERMISSION_ID));
	}
	if(!$sess_user->user_permission_drop($user_id,'',$permission_id,'')){
		print(red(PERMISSION_DROP_FAILED));
	}else{
		print(red(PERMISSION_DROP_SUCCESSED));
	}	
}


/***************************************user management ********************************/
$_users_	= $sess_user->get_all_users();
$title	= Array(
	USER_ID_TITLE,
	USER_NAME_TITLE,
	USER_DESC_TITLE,
	USER_PERMISSION_ADMIN_TITLE,
	USER_MANAGEMENT_TITLE
);
foreach($title AS $key => $val){
	$arr['title'][$key] = '^'.base64_encode($val);
}
if(is_array($_users_)){
 foreach($_users_ as $user_id => $_user){
	$user_admin		=  "<a href=\"?op=user_update&user_id=".$user_id."\" >";
	$user_admin             .= USER_UPDATE_TEXT." </a>";
	$user_admin		.= "<a href=\"?op=user_drop&user_id=".$user_id."\" > ";
	$user_admin             .= USER_DROP_TEXT." </a> ";
	$user_perm_list		=  "<table width=100% bgcolor='#C0C0C0' cellspacing =0 height=26>";
	$user_perm_list         .= "<tr><td width='100%' align=center ><b>";
	$user_perm_list         .= PERMISSION_LIST_TEXT."</b></td></tr></table><hr>";
	$user_perm_select	=  PERMISSION_ADD_TEXT;
	$user_perm_select       .= " <SELECT onchange=\"javascript:if(this.value){";
	$user_perm_select 	.= "window.location='?op=permission_add&permission_id='";
	$user_perm_select       .= "+this.value+'&user_id=".$user_id."';} \" >";
	$user_perm_select       .= "<option selected> ".PERMISSION_SELECT_TEXT." </option>";
	$selectoption		= '';
	$perm_list		= '';
	foreach($____user_permissions_lists AS $perm_id => $_perm){
		if(is_array($_user['user_permissions']) 
		&& array_key_exists($perm_id,$_user['user_permissions']) ){
			$perm_list   	.= $_perm['permission_name']; 
			$perm_list 	.= "&nbsp;&nbsp;<a href='?op=permission_drop";
			$perm_list         .= "&permission_id=".$perm_id;
			$perm_list        	.= "&user_id=".$user_id."' >";
			$perm_list 	.= PERMISSION_DROP_TEXT . "</a><hr>";
		}else{
			$selectoption 		.= "<option value='".$perm_id."'>";
			$selectoption 		.= $_perm['permission_name']."</option>";
		}
	}	
	if($selectoption){
		$user_perm_select       .= $selectoption."</SELECT>";
	}else{
		$user_perm_select 	= '';
	}
	if($perm_list){
		$user_perm_list		.= $perm_list ;
	}else{
		$user_perm_list		= '';
	}
	$user_permission	= $user_perm_list." ".$user_perm_select;
	$arr[$user_id] = Array(
		'user_id'	=>	jsescape($_user['user_id']), 
		'user_name'	=>	jsescape($_user['user_name']),
		'user_desc'	=>	jsescape($_user['user_desc']),
		'user_perm'	=>	jsescape($user_permission),
		'user_admin'	=>	jsescape($user_admin),
	);
 }
}
$string	=  "<script language='JavaScript'>\n";
$string .= " var DataListArray = new Array();\n";
foreach($arr AS $UID => $USER){
	$string	.= "  DataListArray['".$UID."'] = '".implode(',',$USER)."';\n";
}
$string .= "DataInit(DataListArray,'content','pagesplit',10,20,".count($title).");\n";
$string .= "</script>\n";
echo "<div id='content' valign='top' ></div>\n";
echo "<div id='pagesplit' valign='top' ></div><br>\n";
echo $string;
unset($string);
/****************************************user management ******************************************/

/****************************************user UPDATE form******************************************/
if(isset($_GET['op']) && $_GET['op'] == 'user_update'){
	if(isset($_GET['user_id'])){
		$user_id        = mysql_escape_string($_GET['user_id']);
	}else{
		die(red(INVALID_USER_ID));
	}
	$_user_		= $sess_user->user_exists('',$user_id);
	if(!$_user_){
		die(red(INVALID_USER_ID));
	}else{
		@extract($_user_);
	}
?>
<script language='javascript'>
function check_update_input(form){
        if((form.password.value || form.password1.value) && form.password.value != form.password1.value){
                alert('您输入的两次密码不一致');
                return false;
        }
        return true;
}
</script>
<TABLE cellspacing=1 bgcolor='#000000' width=100% align=center class='form'>
<FORM method=post onsubmit="return check_update_input(this)" >
<INPUT type=hidden name='op' value='user_update'>
<INPUT type=hidden name='userid' value='<?isset($user_id)?print($user_id):null;?>'>
<TR><TD  bgcolor='#C0C0C0' align=left ></TD><TD bgcolor='#C0C0C0'>用户资料修改</TD></TR>
<TR><TD  bgcolor='#FFFFFF' align=left >用户名   </TD><TD bgcolor='#FFFFFF'><?isset($user_name)?print($user_name):null;?></TD></TR>
<TR><TD  bgcolor='#FFFFFF' align=left >密码     </TD><TD bgcolor='#FFFFFF'><input type=password name='password' ></TD></TR>
<TR><TD  bgcolor='#FFFFFF' align=left >确认密码</TD><TD bgcolor='#FFFFFF'><input type=password name='password1' ></TD></TR>
<TR><TD  bgcolor='#FFFFFF' align=left >EMAIL    </TD><TD bgcolor='#FFFFFF'><input type=text name='email' value='<?isset($user_email)?print($user_email):null;?>' ></TD></TR>
<TR><TD  bgcolor='#FFFFFF' align=left >电话    </TD><TD bgcolor='#FFFFFF'><input type=text name='email' value='<?isset($user_email)?print($user_email):null;?>' ></TD></TR>
<TR><TD  bgcolor='#FFFFFF' align=left >状态    </TD><TD bgcolor='#FFFFFF'<select name="状态" size="1"><option value="激活"><option value="禁止"></select>  <input type=text name='email' value='<?isset($user_email)?print($user_email):null;?>' ></TD></TR>
<TR><TD  bgcolor='#FFFFFF' align=left >说明     </TD><TD bgcolor='#FFFFFF'><textarea name='desc' rows=6 cols=100 ><?isset($user_desc)?print($user_desc):null;?></textarea></TD></TR>
<TR><TD  bgcolor='#FFFFFF' align=left ></TD><TD bgcolor='#FFFFFF'><input type=submit value="提交" ></TD></TR>
</FORM>
</TABLE>
<?
}
/****************************************user UPDATE form******************************************/

//gen user add form
/****************************************user add form 	 ******************************************/
?>
<script language='javascript'>
function check_input(form){
	if(!form.username.value){
		alert('请输入用户名');
		return false;
	}
	if(!form.password.value || !form.password.value ){
		alert('请输入您的密码');
		return false;
	}
	if(form.password.value != form.password1.value){
		alert('您输入的两次密码不一致');
		return false;
	}
	return true;
}
</script>
<TABLE cellspacing=1 bgcolor='#000000' width=100% align=center class='form'>
<FORM method=post onsubmit="return check_input(this)" >
<INPUT type=hidden name='op' value='user_add'>
<TR><TD  bgcolor='#C0C0C0' align=left ></TD><TD bgcolor='#C0C0C0'>添加新用户</TD></TR>
<TR><TD  bgcolor='#FFFFFF' align=left >用户名	</TD><TD bgcolor='#FFFFFF'><input type=text name='username' ></TD></TR>
<TR><TD  bgcolor='#FFFFFF' align=left >密码	</TD><TD bgcolor='#FFFFFF'><input type=password name='password' ></TD></TR>
<TR><TD  bgcolor='#FFFFFF' align=left >确认密码</TD><TD bgcolor='#FFFFFF'><input type=password name='password1' ></TD></TR>
<TR><TD  bgcolor='#FFFFFF' align=left >EMAIL    </TD><TD bgcolor='#FFFFFF'><input type=text name='email' ></TD></TR>
<TR><TD  bgcolor='#FFFFFF' align=left >电话 </TD><TD bgcolor='#FFFFFF'><input type=text name='email' ></TD></TR>
<TR><TD  bgcolor='#FFFFFF' align=left >状态	</TD><TD bgcolor='#FFFFFF'><input type=text name='email' ></TD></TR>
<TR><TD  bgcolor='#FFFFFF' align=left >说明 </TD><TD bgcolor='#FFFFFF'><textarea name='desc' rows=6 cols=100 ></textarea></TD></TR>
<TR><TD  bgcolor='#FFFFFF' align=left ></TD><TD bgcolor='#FFFFFF'><input type=submit value="提交" ></TD></TR>
</FORM>
</TABLE>
<?
/****************************************user add form 	 ******************************************/

require_once('./include/footer.inc.php');
?>
