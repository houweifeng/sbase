<?php
define(CALL_TIMEOUT, 200000);
define(SOCK_END, "\r\n\r\n"
class CLQFTP
{
    var $host = "127.0.0.1";
    var $port = 61400;
    var $sock = false;
    var $timeout = 0;
    var $req = Array();

    function CLQFTP($host, $port)
    {
        if($host && $port)
        {
            $this->host = $host;
            $this->port = $port;
        }
        $this->sock = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
        return socket_connect($this->sock, $this->host, $this->port);

    }

    function add($file, $destfile)
    {
        $taskid = -1;
        if($file && $destfile && file_exists($file))
        {
            $req = "put $file $destfile\r\n";
            $resp = $this->query($req, strlen($req), CALL_TIMEOUT); 
            if($resp != FALSE)
            {
                return $taskid;
            }
        }
        return FALSE;
    }

    function status($taskid)
    {
        if($taskid >= 0)
        {
        }
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
                else return false;
                $usec = $this->timer($stime);
                if($usec >= $timeout)
                    return false;
                usleep(1);
            }
            return $string;
        }
        return false;
    }
    /* Close socket */
    function close()
    {
        if($this->sock)
        {
            socket_close($this->sock);
            $this->sock = false;
        }
    }
}
?>
