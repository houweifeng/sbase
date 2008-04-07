<?php
class cStoredb
{
	var $db;

	function cStoredb(&$db)
	{
        $this->db = $db;
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
