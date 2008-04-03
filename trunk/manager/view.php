<?php
include_once('./include/header.inc.php');
if(!isset($sess_user) || !$sess_user->is_permission(SYSTEM_ADMIN)){
        echo USER_ADMIN_PERMISSION_NOT_EXISTS;
        exit;
}
$op         =  (isset($_POST['op'])) ? $_POST['op'] : '';
$url        =  (isset($_POST['url'])) ? $_POST['url'] : '';
$keyword    =  (isset($_POST['keyword'])) ? $_POST['keyword'] : '';
$data_type  =  (isset($_POST['data_type'])) ? $_POST['data_type'] : '';
$level      =  (isset($_POST['level'])) ? $_POST['level'] : '';

//query URL 
if($op == 'url_query')
{
    $url_data_array = Array(
            'UrlMD5' => 'dskfjalkdsfjdlkf',
            'Url' => 'http://www.china.com',
            'DownloadTime' => '2008-02-15 05:22:34',
            'SubmitTime' => '2008-02-15 03:22:34',
            'EndTime' => '2008-02-15 05:22:34',
            'UseTime' => '2008-02-15 05:22:34',
            'ParserTime' => '2008-02-15 05:22:34',
            'ParserEndTime' => '2008-02-15 05:22:34',
            'DownlCount' => '1',
            'ParserCount' => '1',
            'Priority' => '1',
            'DomainID' => '100',
            'Status' => '1',
            'Flag' => '1',
            'Bytes' => '1024'
            ); 
}

//add URL
if($op == 'url_add')
{
}

//data query
if($op == 'data_query')
{
    $data_list = Array (
    '' => '',
    );
}

?>
<script Language='JavaScript'>
function ActionUrlForm(form, opvalue)
{
    if(form.url.value == '')
    {
        alert('请输入URL地址后提交!');
        return false;
    }
    form.op.value = opvalue;
    form.add.disabled = true;
    form.query.disabled = true;
    form.submit();
}
function ActionDataForm(form)
{
    if(form.keyword.value == '')
    {
        alert('请输入关键词查询数据!');
        return false;
    }
    form.submit();
}
</script>
<TABLE cellspacing=0 bgcolor='#000000' width='100%' align=center >
<FORM action='view.php' method='POST' name='URL' >
<input type=hidden name='op' value='url_query'>
<TR align=left>
<TD bgcolor='#CCCC90' align=left width=100 ><b>URL查询:</b></TD>
<TD bgcolor='#CCCC90' align=left  >
URL地址<input type=text name='url' size=64 value='<? echo $url;?>'>
    <input type=button name='query' value='查询' 
        onclick="javascript:ActionUrlForm(this.form, 'url_query');">
    <input type=button name='add' value='添加'
        onclick="javascript:ActionUrlForm(this.form, 'url_add');">
	</TD>	
</TR>
</FORM>
</TABLE>

<TABLE cellspacing=0 bgcolor='#000000' width='100%' align=center >
<FORM action='view.php' method='POST' name='data' 
    OnAction='javascript:return ActionDataForm(this);' >
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
