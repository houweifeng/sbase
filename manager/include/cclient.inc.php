<?php
/* 
 *
 */
class cClient{
    var $client_id;
    var $client_name;
    var $client_desc;
    var $client_datetime;
    var $client_email;
    var $client_phone;
    var $debug;
    //client permissions
    var $db_prefix = "";
    //database connection
    var $_db;
    //error
    var $error;
    function cClient(&$db,$db_prefix='',$debug=''){
        $this->_db		= $db;
        $this->db_prefix	= $db_prefix;
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
            $this->_db = new cDatabase($db->db_host,$db->db_clientname,$db->db_password,
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
    //check client is exists
    function client_exists($clientname,$clientid=''){
        if(!$clientname && !$clientid){return '';}
        $SQL    =  " SELECT * FROM ".$this->db_prefix."clients WHERE ";
        $SQL	.= " client_name = '$clientname' ";
        $SQL	.= ($clientid)?" OR client_id = '$clientid' ":'';
        $SQL	.= ";"; 
        $result =       $this->db_fetch_array($SQL);
        if(!is_array($result)){
            return FALSE;
        }
        return $result[0];
    }
    //clientname and password verify
    function verify($clientname,$password){
        $row	= $this->client_exists($clientname);
        if($row && $row['client_password'] == md5($password) ){
            return $row;
        }
        return 0;
    }
    //authenticate and init client data
    function authenticate($clientname,$password){
        $row	= $this->verify($clientname,$password);
        if(!$row){return 0;}
        $this->client_id          = $row['client_id'];
        $this->client_name        = $row['client_name'];
        $this->client_desc        = $row['client_desc'];
        $this->client_datetime    = $row['client_datetime'];
        $this->client_phone       = $row['client_phone'];
        $this->client_email       = $row['client_email'];
        $permissions		= $this->get_client_permissions($row['client_id']);
        $this->client_permissions = ($permissions)?$permissions:false;
        return 1;
    }

    //delete client detail
    //删除用户信息
    function client_drop($client_id, $client_name=''){
        $SQL  = "DELETE FROM ".$this->db_prefix."clients WHERE client_id = '$client_id'";
        $SQL .= ($client_name)?" OR client_name = '$client_name'":'';
        $SQL .= ";";
        $this->client_permission_drop($client_id,$client_name);
        return $this->db_query($SQL);
    }
    function password_change($clientname,$old_password, $new_password){
        if(!$this->verify($clientname,$old_password)) {
            return 2;
        }
        if(!$this->password_update($clientname,$new_password)){
            return 0;
        }
        return TRUE;
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
    //update client detail
    function client_update($data){
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
        $SQL = "UPDATE ".$this->db_prefix."clients SET $data_string WHERE $where_str ;";
        return $this->db_query($SQL);
    }
    //get all clients detail
    //取得用户详细资料
    function get_all_clients(){
        $db	=	$this->db();
        $SQL  	=	"SELECT * FROM ".$this->db_prefix."clients;";
        $result	=	$db->query($SQL);
        if(!$result || !$db->num_rows($result)){
            return FALSE;
        }
        while($row = $db->fetch_array($result)){
            if(!$row){continue;}
            $client_id 		= $row['client_id'];
            $data[$client_id] 	= $row;
        }
        return $data;
    }
    // registers a new client
    function client_add($client_name,$password, $client_phone, $client_email,$client_desc){
        $SQL  = "INSERT INTO ".$this->db_prefix."clients(client_name, client_password,";
        $SQL .= " client_email, client_phone, client_desc, client_datetime) ";
        $SQL .= "VALUES ('$client_name', md5('$password'), '$client_email', '$client_phone',";
        $SQL .= " '$client_desc', NOW())";
        return ($this->db_query($SQL))?($this->_db->insert_id()):'';
    }
    // updates client password
    function password_update($client_id,$clientname,$password){
        $SQL  	= 	"UPDATE ".$this->db_prefix."clients SET client_password  = md5('$password') ";
        $SQL 	.= 	"WHERE client_id = '$client_id' OR client_name = '$clientname';";
        return $this->db_query($SQL);
    }
}
