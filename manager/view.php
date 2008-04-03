<?php
include_once('./include/header.inc.php');
if(!isset($sess_user) || !$sess_user->is_permission(SYSTEM_ADMIN)){
        echo USER_ADMIN_PERMISSION_NOT_EXISTS;
        exit;
}
$op         = (isset($_POST['op'])) ? $_POST['op'] : '';
$view_type  = (isset($_POST['view_type'])) ? $_POST['view_type'] : '';
$date_type  =  (isset($_POST['date_type'])) ? $_POST['date_type'] : '';
$url        =  (isset($_POST['url'])) ? $_POST['url'] : '';
$search_submit = '查询';
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
<TABLE cellspacing=0 bgcolor='#000000' width='100%' align=center >
<FORM action='view.php' method='POST' >
<input type=hidden name='op' value='url_search'>
<TR align=left>
<TD bgcolor='#CCC000' align=left width=100 ><b>URL查询:</b></TD>
<TD bgcolor='#CCC000' align=left  >
URL地址<input type=text name='url' size=64 value='<? echo $url;?>'>
    <input type=submit name='submit' value='<? echo $search_submit ?>' >
	</TD>	
</TR>
</FORM>
</TABLE>

<TABLE cellspacing=0 bgcolor='#000000' width='100%' align=center >
<FORM action='view.php' method='POST' >
<input type=hidden name='op' value='data_search'>
<TR>
<TD bgcolor='#CCCCCC' align=left width=100 >
   <b>数据查询:</b>
   </TD>
	<TD bgcolor='#CCCCCC' align=left width >
    关键词<input type=text name='keyword' size=32 value='<? echo $keyword;?>'>
    类别<select name='data_type' >
    <option value='product' >产品</option>
    <option value='site' >公司</option>
    </select>
    <input type=submit name='submit' value='查询' >
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
