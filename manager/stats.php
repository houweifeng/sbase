<?php
include_once('./include/header.inc.php');
if(!isset($sess_user) || !$sess_user->is_permission(DATA_ADMIN)){
        echo DATA_ADMIN_PERMISSION_NOT_EXISTS;
        exit;
}
$op         = (isset($_POST['op'])) ? $_POST['op'] : '';
$view_type  = (isset($_POST['view_type'])) ? $_POST['view_type'] : '';
$date       =  (isset($_POST['date'])) ? $_POST['date'] : '';
//view URL stat
if($op == 'view')
{
    if($view_type == 'day')
    {
        $data = "data"; 
    }
    if($view_type == 'domain')
    {
    }
}
?>
<TABLE cellspacing=0 bgcolor='#000000' width=100% align=center class='form'>
<FORM action='' method='POST' >
<input type=hidden name='op' value='view'>
<TR>
<TD bgcolor='#CCCCCC' align=left width=200 ><b>下载统计查看</b></TD>
	<TD bgcolor='#CCCCCC' align=left >查看方式:
    <select name='view_type' >
    <option value='client' >节点按天</option>
    <option value='domain' >站点按天</option>
    <option value='total' >节点汇总</option>
    </select>
    日期<input type=text name='date' >
    <input type=submit name='submit' >
    </TD>
</TR>
</FORM>
</TABLE>

<?
include_once('./include/footer.inc.php');
?>
