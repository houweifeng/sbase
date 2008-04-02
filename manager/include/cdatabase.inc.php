<?php
/********************************************************************************************
 *
 *      SounOS WebSite Content Management
 *      This Project Supported By SounOS Development Group <devel@list.sounos.org>
 *
 *
 * This Class is Used for management of database access
 * License              GNU General Public License (GPL)
 * Author               redor <redor@126.com>
 * Copyright            By SounOS <http://cms.sounos.org>
 * Last Modified        Thu Mar 23 2006
 *
 *********************************************************************************************/
function mssql_insert_id($link_id)
 {
    $SQL = 'SELECT @@IDENTITY last_insert_id;';
    if(($result = mssql_query($SQL, $link_id)))
    {
        $array = mssql_fetch_array($result);
        return $array['last_insert_id'];
    }
    return false;
 }
class cDatabase{
    var $db_host		= '';
    var $db_name		= '';
    var $db_username	= '';
    var $db_password	= '';
    var $db_type		= '';
    var $pconnect		= '';
    var $error		    = '';
    var $debug		    = '';
    var $link_id		= '';
    var $db_pconnect = '';
    var $db_connect = '';
    var $db_select_db = '';
    var $db_errno  = '';
    var $db_error = '';
    var $db_query = '';
    var $db_num_rows = '';
    var $db_fetch_row = '';
    var $db_fetch_array = '';
    var $db_fetch_object = '';
    var $db_fetch_assoc = '';
    var $db_insert_id = '';
    var $db_data_seek = '';

    /**
     *
     * Class Construct
     * INIT some member viarable of class
     *
     * @param string , $db_host database hostname OR hostip OR sock 
     * @param string , $db_username username for accessing database
     * @param string , $db_password password for accessing database 
     * @param string , $db_type database type ,mysql pgsql msql etc 
     * @param string , $db_name default database name for connection
     * @param string , $pconnect boolen for mysql_pconnect 
     * @param string , $debug boolen for show debug msg
     * 
     **/
    function cDatabase($db_host,$db_username,$db_password,$db_type,
        $db_name='',$pconnect='',$debug='')
    {
            $this->db_host		= $db_host;
            $this->db_name		= $db_name;
            $this->db_username	= $db_username;
            $this->db_password	= $db_password;
            $this->db_type		= $db_type;
            $this->pconnect		= $pconnect;
            $this->debug		= $debug;
            switch($db_type)
            {
                case 'mysql':
                    $this->db_pconnect = 'mysql_pconnect';
                    $this->db_connect = 'mysql_connect';
                    $this->db_select_db = 'mysql_select_db';
                    $this->db_errno  = 'mysql_errno';
                    $this->db_error = 'mysql_error';
                    $this->db_query = 'mysql_query';
                    $this->db_num_rows = 'mysql_num_rows';
                    $this->db_fetch_row = 'mysql_fetch_rows';
                    $this->db_fetch_array = 'mysql_fetch_array';
                    $this->db_fetch_object = 'mysql_fetch_object';
                    $this->db_fetch_assoc = 'mysql_fetch_assoc';
                    $this->db_insert_id = 'mysql_insert_id';
                    $this->db_data_seek = 'mysql_data_seek';
                    break;
                case 'mssql' :
                    $this->db_pconnect = 'mssql_pconnect';
                    $this->db_connect = 'mssql_connect';
                    $this->db_select_db = 'mssql_select_db';
                    $this->db_errno  = '';
                    $this->db_error = '';
                    $this->db_query = 'mssql_query';
                    $this->db_num_rows = 'mssql_num_rows';
                    $this->db_fetch_row = 'mssql_fetch_row';
                    $this->db_fetch_array = 'mssql_fetch_array';
                    $this->db_fetch_object = 'mssql_fetch_object';
                    $this->db_fetch_assoc = 'mssql_fetch_assoc';
                    $this->db_insert_id = 'mssql_insert_id';
                    $this->db_data_seek = 'mssql_data_seek';
                    break;
            }
        return $this->connect();
    }

    /**
     *
     * db connecting
     * 
     *
     **/	
    function connect()
    {

        if($this->pconnect)
        {
            if($this->db_pconnect)
            {
                $this->link_id = call_user_func($this->db_pconnect,$this->db_host,
                    $this->db_username, $this->db_password);
            }
        }
        else
        {
            if($this->db_connect)
            {
                $this->link_id = call_user_func($this->db_connect,$this->db_host,
                    $this->db_username,$this->db_password);
            }
        }
        if(!is_resource($this->link_id))
        {
            $this->error();
            return '';	
        }
        return ($this->db_name)?@call_user_func($this->db_select_db,$this->db_name,$this->link_id):'';
    }

    /**
     *
     * get db connection error
     *
     *
     **/ 
    function error()
    {
        if($this->db_error && $this->db_errno)
        {
            if(is_resource($this->link_id)){
                $errno  = call_user_func($this->db_errno,$this->link_id);
                $error  = call_user_func($this->db_error,$this->link_id);
            }else{
                $errno  = call_user_func($this->db_errno);
                $error  = call_user_func($this->db_error);
            }
            if($this->debug)
            {
                echo "<pre>$errno::$error</pre><br>\n";
                return 0;
            }
            $this->error	.= "<pre>$errno::$error</pre><br>\n";
            return 0;
        }
        return 0;
    }

    /**
     *
     * Excute SQL Query on default db connection
     * @param string , $SQL SQL for querying 
     *
     **/
    function query($SQL)
    {
        if($this->db_query)
        {
            $result = @call_user_func($this->db_query,$SQL,$this->link_id);
            if(!$result)
                return $this->error();
            return $result;
        }
    }

    /**
     *
     * get ALL data as Array
     * @param resource , $result result of SQL query 
     * @param INSTANT  , $type data fetch type , MYSQL_ASSOC default
     * 
     **/
    function fetch_all_array(&$result,$type=MYSQL_ASSOC)
    {
        if($this->db_fetch_array && $this->db_num_rows)
        {
            $data	= Array();
            if(!$result || !@call_user_func($this->db_num_rows,$result))
            {
                return $this->error();
            }
            while(($row	= call_user_func($this->db_fetch_array,$result,$type)))
                $data[]	= $row;
            return $data;
        }
    }

    /**
     *
     * get rows number of result
     * @param resource , $result result of SQL query
     *
     **/
    function num_rows(&$result)
    {
        if($this->db_num_rows)
            return @mysql_num_rows($result);
        return false;
    }

    /**
     *
     * //fetch data as Array constant : MYSQL_ASSOC,MYSQL_NUM , MYSQL_BOTH
     * @param resource , $result result of SQL query
     * @param INSTANT  , $type data fetch type , MYSQL_ASSOC default
     *
     **/
    function fetch_array(&$result,$type=MYSQL_ASSOC)
    {
        if($this->db_fetch_array)
            return @call_user_func($this->db_fetch_array,$result,$type);
        return false;
    }

    /**
     *
     * fetch data Array as ASSOC
     * @param resource , $result result of SQL query
     *
     **/	
    function fetch_assoc(&$result)
    {
        if($this->db_fetch_assoc)
        {
            return @call_user_func($this->db_fetch_assoc,$result);
        }
        return false;
    }

    /**
     *
     * fetch data Array as NUM 
     * @param resource , $result result of SQL query
     *
     **/
    function fetch_row(&$result)
    {
        if($this->db_fetch_row)
        {
            return @call_user_func($this->db_fetch_row,$result);
        }
        return false;
    }

    /**
     *
     * create new database
     * @param string , $db_name database name
     *
     **/
    function create_db($db_name)
    {
        if($dbname)
        {
            $SQL 	= " CREATE DATABASE $db_name ;";
            return $this->query($SQL);
        }	
        return false;
    }

    /**
     *
     * drop database
     * @param string , $db_name database name
     * 
     **/
    function drop_db($db_name)
    {
        if($db_name)
        {
            $SQL	= "DROP DATABASE $db_name ;";
            return $this->query($SQL);
        }
        return false;
    }

    /**
     *
     * list all db
     *
     **/
    function list_dbs()
    {
        $SQL    = "SHOW  DATABASES;";
        if(($result	= $this->query($SQL)))
            return $this->fetch_all_array($result);
        return false;
    }

    /**
     *
     * list tables of $db_name
     * @param string , $db_name database name ,if null use default database name
     * 
     **/
    function list_tables($db_name='')
    {
        if($this->db_fetch_row == '') return false;
        $sdb_name	= ($db_name) ? $sdb_name : $this->db_name;
        $SQL    	= " SHOW  TABLES FROM $sdb_name;";
        if(($result 	= $this->query($SQL)))
        {
            $lists		= Array();
            while(($row = call_user_func($this->db_fetch_row,$result)))
            {
                $lists[] = $row[0];
            }
            return $lists;
        }
    }

    /**
     *
     * data seek
     * @param resource , $result result of SQL Query
     * @param int	   , $offset point of data
     *
     **/
    function data_seek(&$result,$offset)
    {
        if($this->db_data_seek == '') return false;
        return @call_user_func($this->db_data_seek,$result,$offset);
    }

    /**
     *
     * fetch length
     * @param resource , $result result of SQL Query
     *
     **/
    function fetch_object(&$result)
    {
        if($this->db_fetch_object == '') return false;
        return @call_user_func($this->db_fetch_object,$result);
    }

    /**
     *
     * get autoincrement id
     * @param resource , $link_id DB connection ID , if null used default connection ID
     *
     **/
    function insert_id(&$link_id)
    {
        if($this->db_insert_id == '') return false;
        if(($insert_id = @call_user_func($this->db_insert_id,$link_id)) == '')
            $insert_id = @call_user_func($this->db_insert_id,$this->link_id);
        return $insert_id;
    }

    /**
     *
     *
     *
     **/
    function affected_rows(&$result)
    {
    }

    /**
     *
     * get server system stat
     * @param resource , $link_id DB connection ID , if null used default connection ID
     *
     **/
    function status(&$link_id)
    {
        if($this->db_query && $this->db_num_rows && $this->db_fetch_array)
        {
            $link_id	= ($link_id)?$link_id:$this->link_id;
            $SQL 		= "SHOW STATUS;";
            $result 	= @call_user_func($this->db_query,$SQL, $link_id);
            $stat		= Array();
            $num = call_user_func($this->db_num_rows,$result);
            while ($num > 0 && (list($key,$val) = call_user_func($this->db_fetch_array,$result)))
                $stat[$key]	= $val;
            return $stat;
        }
        return false;
    }

    /**
     *
     * get server info
     * @param resource , $link_id DB connection ID , if null used default connection ID
     *
     **/
    function server_info(&$link_id)
    {
    }

    /**
     *
     * get server info
     * @param resource , $link_id DB connection ID , if null used default connection ID
     *
     **/
    function processlist(&$link_id)
    {
        if($this->db_query && $this->db_num_rows && $this->db_fetch_array)
        {
            $link_id        = ($link_id)?$link_id:$this->link_id;
            $SQL	= "SHOW PROCESSLIST;";
            $result         = @call_user_func($this->db_query,$SQL,$link_id);
            $processlit	= Array();
            while(@call_user_func($this->db_num_rows,$result) 
            && $row = call_user_func($this->db_fetch_array,$result, MYSQL_ASSOC)){
                    $processlist[]	= $row;
                }	
            return $processlist;
            break;
        }
    }
}
?>
