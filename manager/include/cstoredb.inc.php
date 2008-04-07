<?php
class cStoredb
{
	var $db;

	function cStoredb($db_host, $db_username, $db_passwd, $db_name, $db_type)
	{
        $this->db = new cDatabase($db_host, $db_username, $db_passwd,
            $db_type, $db_name, false, false);
    }

	function checkdb()
	{
        return is_resource($this->db->link_id);
	}

    //find product 
	function find_product($keyword)
	{
        if($this->checkdb() == false) 
            return false;
        $table = "";
        $SQL =  "SELECT * FROM $table WHERE  PRODUCTCNAME ";
        $SQL .= " = '$keyword' OR  PRODUCTENAME = '$keyword';";
        if(($result = $this->db->query($SQL)))
        {
            return $this->db->fetch_array($result);
        }
        return false;
	}

    //find company
    function find_company($keyword)
	{
        if($this->checkdb() == false) 
            return false;
        $table = "";
        $SQL =  "SELECT * FROM $table WHERE  PRODUCTCNAME ";
        $SQL .= " = '$keyword' OR  PRODUCTENAME = '$keyword';";
        if(($result = $this->db->query($SQL)))
        {
            return $this->db->fetch_array($result);
        }
        return false;
	}

}
?>
