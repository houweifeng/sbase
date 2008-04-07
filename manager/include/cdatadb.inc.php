<?php
class cDatadb
{
	var $db;

	function cDatadb($db_host, $db_username, $db_passwd, $db_name, $db_type)
	{
        $this->db = new cDatabase($db_host, $db_username, $db_passwd,
            $db_type, $db_name, false, false);
    }

	function checkdb()
	{
        return is_resource($this->db->link_id);
	}

	function find_url($url)
	{
        if($this->checkdb() == false) 
            return false;
        $urlmd5 = hexdec(md5(ltrim(rtrim($url))));
        $table = "";
        $SQL = "SELECT * FROM $table WHERE UrlMD5 = '$urlmd5';";
        if(($result = $this->db->query($SQL)))
        {
            return $this->db->fetch_array($result);
        }
        return false;
	}

    //get client day statics table 
    function get_client_stat($date)
    {
        if($this->checkdb() == false) return false;
        $table  = "ClientDayQuery".$date; 
        $SQL    = "SELECT * FROM $table";
        if(($result = $this->db->query($SQL)))
        {   
            return $this->db->fetch_array($result);
        }
        return false;
    }

    //get client day statics table 
    function get_domain_stat($date)
    {
        if($this->checkdb() == false) return false;
        $table  = "DomainDayQuery".$date; 
        $SQL    = "SELECT * FROM $table";
        if(($result = $this->db->query($SQL)))
        {   
            return $this->db->fetch_array($result);
        }
        return false;
    }

    //get total static 
    function get_client_total()
    {
        if($this->checkdb() == false) return false;
        $table  = "DomainDayQuery".$date;
        $SQL    = "SELECT * FROM $table";
        if(($result = $this->db->query($SQL)))
        {
            return $this->db->fetch_array($result);
        }
        return false;
    }
}
?>
