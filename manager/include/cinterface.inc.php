<?php
class cInterface
{
    var $host = URL_SVR_HOST;
    var $port = URL_SVR_PORT;
    var $sock = false;
    var $timeout = 0;
    var $respcode = 0;
    var $urllist = Array();
    var $is_connected = 0;

    function cInterface($host=URL_SVR_HOST, $port=URL_SVR_PORT)
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

    function addurl($priority, $url)
    {
        $data[$url] = $priority;
    }

    function send()
    {
        $count = $this->urllist;
        if($count > 0)
        {
            $req = $count.SOCK_END;
            foreach($this->urllist AS $url => $pri)
            {
                $req .= $url.FEILD_DELIMITER.$pri.SOCK_END;
            }
            $resp = $this->query($req, strlen($req), CALL_TIMEOUT); 
            if($resp === RESP_OK)
                return true;
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

    /* Close socket */
    function close()
    {
        socket_close($this->sock);
        $this->sock = false;
        $this->is_connected = 0;
    }
}
?>
