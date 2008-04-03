<?php
/* 
 *
 */
class cUser{
    var $IS_LOGGED_IN;
    var $user_id;
    var $user_name;
    var $user_desc;
    var $user_datetime;
    var $user_email;
    var $debug;
    //user permissions
    var $user_permissions;
    var $db_prefix = "";
    //database connection
    var $_db;
    //error
    var $error;
    function cUser(&$db,$db_prefix='',$debug=''){
        $this->_db		= $db;
        $this->db_prefix	= $db_prefix;
        $this->IS_LOGGED_IN 	= '';
        $this->debug 		= '';
        if(!is_resource($db->link_id)){
            die('can not connected to db');
        }
    }
    //db connection
    function error($error=''){
        if($this->debug){
            echo "<pre>".$error."</pre><br>\n";
        }else{
            $this->error	.= "<pre>".$error."</pre><br>\n";
        }
        return 0;
    }
    //connect to db or reconnect to db
    function db(){
        if(!is_resource($this->_db->link_id)){
            $db	  = $this->_db;
            $this->_db = new cDatabase($db->db_host,$db->db_username,$db->db_password,
                $db->db_type,$db->db_name,$db->pconnect,$db->debug);
        }
        return $this->_db;
    }
    //db query
    function db_query($SQL){
        $db	= $this->db();
        $result = $db->query($SQL);
        if(!$result){
            return 0;
        }
        return $result;
    }
    //db fetch Array
    function db_fetch_array($SQL){
        $db     = $this->db();
        $result = $db->query($SQL);
        if(!$result || !$db->num_rows($result)){
            return 0;
        }
        while($row = $db->fetch_array($result)){
            if(!is_array($row)){
                continue;
            }
            $data[] = $row;
        }
        return $data;
    }
    //db fetch Array  as defined key
    function db_fetch_key_array($SQL,$key){
        $db     = $this->db();
        $result = $db->query($SQL);
        if(!$result || !$db->num_rows($result)){
            return FALSE;
        }
        while($row = $db->fetch_array($result)){
            if(!$row || empty($row[$key]) ){
                continue;
            }
            $db_key 	= 	$row[$key];
            $data[$db_key]	= 	$row;
        }
        return $data;
    }
    //check user permission
    function is_permission($permission_id){	
        return $this->user_permissions[$permission_id];
    }
    //check user is exists
    function user_exists($username,$userid=''){
        if(!$username && !$userid){return '';}
        $SQL    =  " SELECT * FROM ".$this->db_prefix."users WHERE ";
        $SQL	.= " user_name = '$username' ";
        $SQL	.= ($userid)?" OR user_id = '$userid' ":'';
        $SQL	.= ";"; 
        $result =       $this->db_fetch_array($SQL);
        if(!is_array($result)){
            return FALSE;
        }
        return $result[0];
    }
    //username and password verify
    function verify($username,$password){
        $row	= $this->user_exists($username);
        if($row && $row['user_password'] == md5($password) ){
            return $row;
        }
        return 0;
    }
    //authenticate and init user data
    function authenticate($username,$password){
        $row	= $this->verify($username,$password);
        if(!$row){return 0;}
        $this->user_id          = $row['user_id'];
        $this->user_name        = $row['user_name'];
        $this->user_desc        = $row['user_desc'];
        $this->user_datetime    = $row['user_datetime'];
        $this->user_email       = $row['user_email'];
        $permissions		= $this->get_user_permissions($row['user_id']);
        $this->user_permissions = ($permissions)?$permissions:false;
        $this->IS_LOGGED_IN     = TRUE; 
        return 1;
    }

    //get user permissions
    //取得用户权限
    function get_user_permissions($user_id){
        $SQL 	= 	"SELECT * FROM ".$this->db_prefix."user_permissions WHERE user_id = '$user_id';";
        $data	=	$this->db_fetch_key_array($SQL,'permission_id');
        return $data;
    }
    //permission exits
    function permission_exists($permission_name=''){
        $SQL 	= 	"SELECT * FROM ".$this->db_prefix."permissions ";
        $SQL	.= 	($permission_name)?"WHERE permission_name = '$permission_name';":";";
        $result =	$this->db_fetch_key_array($SQL,'permission_id');
        return $result;
    }
    //permission add
    function permission_add($permission_name,$permission_desc){
        $SQL	=	"INSERT INTO ".$this->db_prefix."permissions(permission_name,permission_desc) ";
        $SQL	.=	" VALUES('$permission_name','$permission_desc');";
        return ($this->db_query($SQL))?($this->_db->insert_id()):'';
    }
    //permission update
    function permission_update($data,$where){
        $where      = $data['where'];
        $_data_     = $data['data'];
        foreach($_data_ AS $feild => $val){
            $_data_arr[]    = " $feild = '$val' ";
        }
        $data_string    = implode(',',$_data_arr);
        foreach($where as $feild => $val){
            $where_arr[] = " $feild = '$val' ";
        }
        $where_str  = implode('AND',$where_arr);
        $SQL 		= "UPDATE ".$this->db_prefix."permissions SET $data_string WHERE $where_str ;";
        $SQL_USER	= "UPDATE ".$this->db_prefix."user_permissions SET $data_string WHERE $where_str;";
        $this->db_query($SQL);
        $this->db_query($SQL_USER);
        return 1;
    }
    //permission drop
    function permission_drop($permission_id){
        $SQL		= "DELETE FROM ".$this->db_prefix."permissions WHERE permission_id = '$permission_id';";
        $SQL_USER	= "DELETE FROM ".$this->db_prefix."user_permissions WHERE permission_id = '$permission_id';";
        $this->db_query($SQL);
        $this->db_query($SQL_USER);
        return TRUE;
    }

    //check user permission is exists
    //检查用户权限时候存在
    function user_permission_exists($permission_id,$permission_name,$user_id,$user_name){
        $SQL  	=  " SELECT * FROM ".$this->db_prefix."user_permissions ";
        $SQL    .= " WHERE (user_id = '$user_id' OR user_name = '$user_name') ";
        $SQL	.= " AND (permission_id = '$permission_id' OR permission_name='$permission_name') ;";
        $result	=	$this->db_fetch_array($SQL);
        return $result[0];
    }
    //add new permission
    //用户权限添加
    function user_permission_add($user_id,$user_name,$permission_id,$permission_name){
        $SQL  	=  "INSERT INTO ".$this->db_prefix."user_permissions(permission_id,";
        $SQL    .= "permission_name,user_id,user_name)";
        $SQL 	.= "VALUES('$permission_id','$permission_name','$user_id','$user_name');";
        return $this->db_query($SQL);
    }
    function user_permission_update($data,$where){
        $where      = $data['where'];
        $_data_     = $data['data'];
        foreach($_data_ AS $feild => $val){
            $_data_arr[]    = " $feild = '$val' ";
        }
        $data_string    = implode(',',$_data_arr);
        foreach($where as $feild => $val){
            $where_arr[] = " $feild = '$val' ";
        }
        $where_str  = implode('AND',$where_arr);
        $SQL = "UPDATE ".$this->db_prefix."user_permissions SET $data_string WHERE $where_str ;";
        return $this->db_query($SQL);
    }
    //delete user permission
    //删除用户权限
    function user_permission_drop($user_id,$user_name,$permission_id='',$permission_name=''){
        $SQL  	=  	"DELETE FROM ".$this->db_prefix."user_permissions WHERE ";
        $SQL 	.= 	"(user_id = '$user_id' OR user_name = '$user_name')";
        if($permission_id  || $permission_name ){
            $SQL 	.= "AND (permission_id = '$permission_id' OR permission_name = '$permission_name') ";
        }
        $SQL 	.= 	";";
        return $this->db_query($SQL);
    }
    //delete user detail
    //删除用户信息
    function user_drop($user_id,$user_name=''){
        $SQL  = "DELETE FROM ".$this->db_prefix."users WHERE user_id = '$user_id'";
        $SQL .= ($user_name)?" OR user_name = '$user_name'":'';
        $SQL .= ";";
        $this->user_permission_drop($user_id,$user_name);
        return $this->db_query($SQL);
    }
    function password_change($username,$old_password, $new_password){
        if(!$this->verify($username,$old_password)) {
            return 2;
        }
        if(!$this->password_update($username,$new_password)){
            return 0;
        }
        return TRUE;
    }
    function logout(){
        $this->IS_LOGGED_IN 	= '';
        $this->user_id	    	= '';
        $this->user_name     	= '';
        $this->user_desc  	= '';
        $this->user_datetime  	= '';
        $this->user_email       = '';
        $this->user_permissions = '';
        return 1;
    }
    // makes a random password
    function make_password($length = 8){
        // thanks to benjones@superutility.net for this code
        mt_srand((double) microtime() * 1000000);
        for ($i=0; $i < $length; $i++) {
            $which = rand(1, 3);
            // character will be a digit 2-9
            if ( $which == 1 ) $password .= mt_rand(0,10);
            // character will be a lowercase letter
            elseif ( $which == 2 ) $password .= chr(mt_rand(65, 90));
            // character will be an uppercase letter
            elseif ( $which == 3 ) $password .= chr(mt_rand(97, 122));
        }
        return $password;
    }
    //update user detail
    function user_update($data){
        $where      = $data['where'];
        $_data_     = $data['data'];
        foreach($_data_ AS $feild => $val){
            $_data_arr[]    = " $feild = '$val' ";
        }
        $data_string    = implode(',',$_data_arr);
        foreach($where as $feild => $val){
            $where_arr[] = " $feild = '$val' ";
        }
        $where_str  = implode('AND',$where_arr);
        $SQL = "UPDATE ".$this->db_prefix."users SET $data_string WHERE $where_str ;";
        return $this->db_query($SQL);
    }
    //get all users detail
    //取得用户详细资料
    function get_all_users(){
        $db	=	$this->db();
        $SQL  	=	"SELECT * FROM ".$this->db_prefix."users;";
        $result	=	$db->query($SQL);
        if(!$result || !$db->num_rows($result)){
            return FALSE;
        }
        while($row = $db->fetch_array($result)){
            if(!$row){continue;}
            $user_id 		= $row['user_id'];
            $data[$user_id] 	= $row;
            $user_permissions 	= $this->get_user_permissions($user_id);
            $data[$user_id]['user_permissions'] = $user_permissions;
        }
        return $data;
    }
    // registers a new user
    function user_add($user_name,$password, $user_email,$user_desc){
        $SQL  = "INSERT INTO ".$this->db_prefix."users(user_name, user_password,";
        $SQL .= " user_email, user_desc, user_datetime) ";
        $SQL .= "VALUES ('$user_name', md5('$password'), '$user_email', '$user_desc', NOW())";
        return ($this->db_query($SQL))?($this->_db->insert_id()):'';
    }
    // updates user password
    function password_update($user_id,$username,$password){
        $SQL  	= 	"UPDATE ".$this->db_prefix."users SET user_password  = md5('$password') ";
        $SQL 	.= 	"WHERE user_id = '$user_id' OR user_name = '$username';";
        return $this->db_query($SQL);
    }
    //action log
    function action_log($action,$username=''){
        $user_name	=	($this->user_name)?$this->user_name:$username;
        $datetime	=	date('Y-m-d H:i:s');
        $ip		=	$_SERVER['REMOTE_ADDR'];
        $SQL		=	" INSERT INTO ".$this->db_prefix."action_log(user_name,action,datetime,ip) ";
        $SQL		.=	" VALUES('$user_name','$action','$datetime','$ip');";
        return $this->db_query($SQL);
    }

}
