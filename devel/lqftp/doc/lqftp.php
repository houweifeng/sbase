#!/usr/bin/php
<?php
define('CALL_TIMEOUT', 200000);
define('SOCK_END', "\r\n\r\n");
define('RESP_OK', 300);
define('READYFLAG', ".xxxxxx.flag");
define('OVERFLAG', ".xxxxxx.end");
class CLQFTP
{
    var $host = "127.0.0.1";
    var $port = 63800;
    var $sock = false;
    var $timeout = 0;
    var $respcode = 0;
    var $is_connected = 0;

    function CLQFTP($host='127.0.0.1', $port=63800)
    {
        if($host && $port)
        {
            $this->host = $host;
            $this->port = $port;
        }
        $this->sock = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
        if(socket_connect($this->sock, $this->host, $this->port))
            $this->is_connected = 1;
    }

    function reconnect()
    {
        $this->sock = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
        if(socket_connect($this->sock, $this->host, $this->port))
            $this->is_connected = 1;
    }

    function parse($string)
    {
        $list = Array();
        $arr = explode("\r\n", $string); 
        for($i = 0; $i < count($arr); $i++) 
        {
            if($arr[$i])
            {
                if(is_array($line = explode(":", $arr[$i])))
                {
                    $key  = trim($line[0]);        
                    $val  = trim($line[1]);        
                    $list[$key] = $val;    
                }
            }
        }
        return $list;
    }

    function get_respcode($string)
    {
        $respid = -1;

    }

    function add($file, $destfile)
    {
        $taskid = -1;
        if($file && $destfile)
        {
            $req = "put ".urlencode($file)." ".urlencode($destfile)."\r\n";
            $resp = $this->query($req, strlen($req), CALL_TIMEOUT); 
            if($resp != FALSE)
            {
                $this->respcode = trim(substr($resp, 0, strpos($resp, " ")));
                if($this->respcode == RESP_OK)
                {
                    $list = $this->parse(substr($resp, strpos($resp, "\r\n") + 2));
                    if(array_key_exists("taskid", $list))
                        return $list['taskid'];
                }
            }
        }
        return FALSE;
    }

    function status($taskid)
    {
        if($taskid >= 0)
        {
            $req = "status $taskid\r\n";
            $resp = $this->query($req, strlen($req), CALL_TIMEOUT); 
            if($resp != FALSE)
            {
                $this->respcode = trim(substr($resp, 0, strpos($resp, " ")));
                if($this->respcode == RESP_OK)
                {
                    $list = $this->parse(substr($resp, strpos($resp, "\r\n") + 2));
                    if(count($list) > 0)
                    {
                        return $list;
                    }
                }
            }
        }
        return FALSE;
    }

    /* set timeout */
    function set_timeout($timeout)
    {
        if($timeout)
            $this->timeout = $timeout;
    }

    /* timer */
    function timer(&$stime)
    {
        if(isset($stime["sec"]) && isset($stime["usec"]))
        {
            $now = gettimeofday();
            return ($now["sec"] * 1000000 + $now["usec"])
                - ($stime["sec"] * 1000000 + $stime["usec"]);
        }
        else
        {
            $stime = gettimeofday();
            return 0;
        }
    }

    /* Query */
    function query($query_string, $timeout)
    {
        $tmp = "";
        $string = "";
        $stime = array();
        $this->timer($stime);
        $n = strlen($query_string);

        if($this->is_connected == 0)
            $this->reconnect();

        if($this->sock  && $n > 0
            && socket_write($this->sock, $query_string, $n) == $n)
        {
            while(1)

            {
                if(($tmp = socket_read($this->sock, 81920, PHP_BINARY_READ)))
                {
                    if( ($n = strpos($tmp, SOCK_END)) === FALSE)
                        $string .= $tmp;
                    else
                    {
                        $string .= substr($tmp, 0, $n + strlen(SOCK_END));
                        break;
                    }
                }
                else
                {
                    $this->close();
                    return false;
                }
                $usec = $this->timer($stime);
                if($usec >= $timeout)
                {
                    $this->close();
                    return false;
                }
                usleep(1);
            }
            return $string;
        }
        $this->close(); 
        return false;
    }
    /* lookup dir */
    function lookup($dir)
    {
       $j = 0;
       $pdir = NULL;
       $dirp = NULL;
       $file = NULL;
       $ent = NULL;
       $path = NULL;
       $p = NULL;
       $overlist = Array();

        if($dir && is_dir($dir) && ($pdir = opendir($dir)))
        {
            while(($file = readdir($pdir)))
            {
                if($file == '.' || $file == '..') continue;
                $path = "$dir/$file";
                if(is_dir($path) && file_exists($path."/".READYFLAG) 
                    && !file_exists($path."/".OVERFLAG))
                {
                    if(filesize($path."/".READYFLAG) > 0)
                        $overlist = file($path."/".READYFLAG);
                    //print_r($overlist);
                    //echo "Ready for transport dir[$path]\n";
                    if(($dirp = opendir($path)))
                    {
                        while(($ent = readdir($dirp)))
                        {
                            $p = $path."/".$ent;
                            if(is_file($p) && $ent != READYFLAG 
                                && filesize($p) > 0 && !in_array($ent."\n", $overlist))
                            {
                                if($this->add($p, "/$file/".$ent) === FALSE)
                                {
                                    closedir($dirp);
                                    closedir($pdir);
                                    return ;
                                }
                                else 
                                {
                                    echo "Added $p to queue\n";
                                    error_log($ent."\n", 3, "$path/".READYFLAG);
                                }
                            }
                        }
                        if($this->add("$path/".READYFLAG, "/$file/".READYFLAG) === FALSE)
                        {
                                    closedir($dirp);
                                    closedir($pdir);
                                    return ;
                        }
                        else
                        {
                            copy("$path/".READYFLAG, "$path/".OVERFLAG);
                        }
                        closedir($dirp);
                    }
                }
            }
            closedir($pdir);
        }
        return ;
    }

    /* Close socket */
    function close()
    {
        socket_close($this->sock);
        $this->sock = false;
        $this->is_connected = 0;
    }
}

if($_SERVER['argc'] < 2)
{
    $self = $_SERVER['argv'][0];
    echo "Usage:$self add file destfile\n";
    echo "\t OR status taskid\n";
    echo "\t OR addlist filelist statuslist\n";
    echo "\t OR statuslist statusfile\n";
    echo "\t OR lookup dir\n";
    exit;
}
$op = $_SERVER['argv'][1];
$lqftp = new CLQFTP();
if($lqftp->is_connected)
{
    //add new task 
    if($op == 'add' && $_SERVER['argc'] >= 4)
    {
        $file = $_SERVER['argv'][2];
        $destfile = $_SERVER['argv'][3];
        if($lqftp)
        {
            $taskid = $lqftp->add($file, $destfile);
            if($taskid === FALSE )
            {
                echo "adding task[put $file  $destfile] failed\n";
            }
            else
            {
                echo "taskid[$taskid] for [put $file to $destfile]\n";
            }
        }     
    }

    //view single task status 
    if($op == 'status')
    {
        $taskid = $_SERVER['argv'][2];
        if(is_numeric($taskid))
        {
            $list = $lqftp->status($taskid);
            if(is_array($list) && count($list) > 0)
            {
                echo "Status for task[$taskid]\n";
                foreach($list as $key => $val)
                {
                    echo "\t$key:$val\n";
                }
                echo "\n";
                exit;
            }
        }
        echo "status task[$taskid] failed\n";
    }

    //add task from filelist
    if($op == 'addlist')
    {
        $filelist = $_SERVER['argv']['2'];
        $statuslist = $_SERVER['argv']['3'];
        if(($fp = fopen($filelist, "r")))
        {
            while(($line = rtrim(fgets($fp, 8192))))
            {
                if(($arr = explode("\t", $line)))
                {
                   $file = $arr[0]; 
                   $destfile = $arr[1]; 
                   if(($taskid = $lqftp->add($file, $destfile)) === FALSE)
                   {
                        echo "task[put $file $destfile] failed, try again later\n";
                        break;
                   }
                   else
                   {
                       $log = "$taskid\t[put $file $destfile]\n";
                       error_log($log, 3, $statuslist);
                       echo $log;
                   }
                }
            }
            fclose($fp);
        }
    }

    //status list
    if($op == 'statuslist')
    {
        $statusfile = $_SERVER['argv']['2'];
        if(($fp = fopen($statusfile, "r")))
        {
            while(($line = fgets($fp, 8192)))
            {
                $taskid = substr($line, 0, strpos($line, "\t"));
                $status = $lqftp->status($taskid);
                if(is_array($status))
                {
                    echo "Status for task[put ". trim($line). "]\n";
                    print_r($status);
                    echo "\n";
                }
            }
            fclose($fp);
        }
    }
    //lookup
    if($op == "lookup")
    {
        $lookdir = $_SERVER['argv']['2'];
        while(1)
        {
                $lqftp->lookup($lookdir);
                sleep(60);
        }
    }
    $lqftp->close();
}
?>
