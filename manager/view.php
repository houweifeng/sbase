<?php
include_once('./include/header.inc.php');
if(!isset($sess_user) || !$sess_user->is_permission(SYSTEM_ADMIN)){
        echo USER_ADMIN_PERMISSION_NOT_EXISTS;
        exit;
}
$op         = (isset($_POST['op'])) ? $_POST['op'] : '';
$view_type  = (isset($_POST['view_type'])) ? $_POST['view_type'] : '';
$date_type  =  (isset($_POST['date_type'])) ? $_POST['date_type'] : '';
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
<FORM action='view.php' method='POST' >
<input type=hidden name='op' value='view'>
<TR>
<TD bgcolor='#CCCCCC' align=left width=200 ><b>抓取统计</b>
	<TD bgcolor='#CCCCCC' align=left width=200 >查看方式:
    <select name='view_type' >
    <option value='day' >按日期</option>
    <option value='domain' >按站点</option>
    </select>
	</TD>	
    <TD bgcolor='#CCCCCC' align=left >
    <input type=submit name='submit' >
    </TD>
</TR>
</FORM>
</TABLE>

<?
//print_r($_SERVER);
?>
</pre>
<?
include_once('./include/footer.inc.php');
?>
