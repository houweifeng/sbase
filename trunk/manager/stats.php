<?php
include_once('./include/header.inc.php');
if(!isset($sess_user) || !$sess_user->is_permission(DATA_ADMIN)){
        echo DATA_ADMIN_PERMISSION_NOT_EXISTS;
        exit;
}
$op         = (isset($_POST['op'])) ? $_POST['op'] : '';
$view_type  = (isset($_POST['view_type'])) ? $_POST['view_type'] : '';
$date       =  (isset($_POST['date'])) ? $_POST['date'] : '';
?>
<TABLE cellspacing=0 bgcolor='#000000' width=100% align=center class='form'>
<FORM action='' method='POST' >
<input type=hidden name='op' value='view'>
<TR>
<TD bgcolor='#CCCCCC' align=left width=200 ><b>下载统计查看</b></TD>
	<TD bgcolor='#CCCCCC' align=left >查看方式:
    <select name='view_type' >
    <? foreach($data_view_type_list AS $opv => $optxt)
    {
        if($opv == $view_type)
            echo "<option value='$opv' selected>$optxt</option>";
        else 
            echo "<option value='$opv' >$optxt</option>";
    }?>
    </select>
    日期<input type=text name='date' value='<? echo $date ?>'>
    <input type=submit name='submit' >
    </TD>
</TR>
</FORM>
</TABLE>
<?
//view URL stat
if($op == 'view')
{
    $obj = new cDatadb($datadb);

    //client stats
    if($view_type == 'client')
    {
        $arr = Array();
        $data = $obj->get_client_stat($date);
        foreach($client_day_feilds AS $ktitle => $vfeild)
            $arr['title'][$vfeild] = '^'.jsescape($ktitle);
        if(is_array($data))
        {
            for($i = 0; $i < count($data); $i++)
            {
                $client = $data[$i]['ClientID'];
                foreach($client_day_feilds AS $ktitle => $vfeild)
                {
                    $arr[$client][$vfeild] = jsescape($data[$i][$vfeild]);
                }
            }
        }
    }

    //domain stats
    if($view_type == 'domain')
    {
        $arr = Array();
        $data = $obj->get_domain_stat($date);
        foreach($domain_day_feilds AS $ktitle => $vfeild)
            $arr['title'][$vfeild] = '^'.jsescape($ktitle);
        if(is_array($data))
        {
            for($i = 0; $i < count($data); $i++)
            {
                $domain = $data[$i]['DomainID'];
                foreach($domain_day_feilds AS $ktitle => $vfeild)
                {
                    if($vfeild == 'DomainID') 
                        $arr[$domain][$vfeild] = '^'.jsescape($data[$i][$vfeild]);
                    else
                        $arr[$domain][$vfeild] = jsescape($data[$i][$vfeild]);
                }
            }
        }
    }

    //total stats 
    if($view_type == 'total')
    {
        $arr = Array();
        $data = $obj->get_client_total($date);
        foreach($client_day_feilds AS $ktitle => $vfeild)
            $arr['title'][$vfeild] = '^'.jsescape($ktitle);
        if(is_array($data))
        {
            for($i = 0; $i < count($data); $i++)
            {
                $client = $data[$i]['ClientID'];
                foreach($client_day_feilds AS $ktitle => $vfeild)
                {
                    $arr[$client][$vfeild] = jsescape($data[$i][$vfeild]);
                }
            }
        }
    }

    $string =  "<script language='JavaScript'>\n";
    $string .= " var DataListArray = new Array();\n";
    //html array
    foreach($arr AS $KID => $VARR)
        $string .= " DataListArray['".$KID."'] = '".implode(',',$VARR)."';\n";
    $string .= "DataInit(DataListArray,'content',";
    $string .= "'pagesplit',10,20,".count($arr['title']).");\n";
    $string .= "</script>\n";

    echo "<div id='content' valign='top' ></div>\n";
    echo "<div id='pagesplit' valign='top' ></div><br>\n";
    echo $string;
    unset($string);
}
?>
<?
include_once('./include/footer.inc.php');
?>
