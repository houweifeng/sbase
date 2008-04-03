<?php
include_once('./include/header.inc.php');
if(!isset($sess_user) || !$sess_user->is_permission(DATA_ADMIN)){
        echo red(DATA_ADMIN_PERMISSION_NOT_EXISTS);
        exit;
}
$op         =  (isset($_POST['op'])) ? $_POST['op'] : '';
$url        =  (isset($_POST['url'])) ? $_POST['url'] : '';
$keyword    =  (isset($_POST['keyword'])) ? $_POST['keyword'] : '';
$data_type  =  (isset($_POST['data_type'])) ? $_POST['data_type'] : '';
$priority   =  (isset($_POST['priority'])) ? $_POST['priority'] : '';
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
    if($data_type == 'product')
    {
        $product_data_array = Array (
                'NFOID' => '',
                'GETINFOTIME' => '',
                'WebName' => '',
                'URL' => '',
                'PRODUCTCNAME' => '',
                'PRODUCTENAME' => '',
                'CATEGORIES' => '',
                'BRANDNAME' => '',
                'KEYWORD ' => '',
                'DESCRIPTION' => '',
                'MODEL' => '',
                'PUBDATE' => '',
                'EXPDATE' => '',
                'ORIGIN' => '',
                'PACKAGING' => '',
                'STANDARD' => '',
                'PAYMENT' => '',
                'DELIVERY' => '',
                'PRICE' => '',
                'INSPECTION' => '',
                'MINIMUMORDER' => '',
                'QUALITYCERTIFICATION' => '',
                'PICURL' => '',
                'BYTES' => '',
                'LPATH' => '',
                'PICLPATH' => '',
                'CONTACTUSURL' => '',
                'SHOWROOMURL' => '',
                'SHOWROOMMD5' => '',
                'ISPICEXIST' => '',
                'DOMAIN_ID' => '',
                'TYPEID' => '',
                'SiteId' => '',
                'WordID' => '',
                'OrderNo' => '',
                'Word' => '');
    }
    if($data_type == 'company')
    {
        $company_data_array = Array(
        'INFOID' => '',
        'URL' => '',
        'SITEID' => '',
        'GETINFOTIME' => '',
        'IDENTITYS' => '',
        'COMPANYCNAME' => '',
        'COMPANYENAME' => '',
        'BUSINESSTYPE' => '',
        'CONTECTPERSON' => '',
        'EMAIL' => '',
        'INDUSTRY' => '',
        'STREETADDRESS' => '',
        'CITY' => '',
        'PROVINCE_STATE' => '',
        'BUSINESSPHONE' => '',
        'FAX' => '',
        'MOBILEPHONE' => '',
        'PRUDUCTKEYWORDS' => '',
        'CATEGORIES' => '',
        'MAINMARCKETS' => '',
        'EMPLOYEENUMBER' => '',
        'COMPANYINTRODUCTION' => '',
        'ANNUALSALES' => '',
        'YEARESTABLISHED' => '',
        'CETTIFICATES' => '',
        'BANKINFORMATION' => '',
        'TRADEINFORMATION' => '',
        'CAPITALASSETS' => '',
        'TRADEMARK' => '',
        'OEMSERVICE' => '',
        'WEBSITE' => '',
        'ABOUTFACTORY' => '',
        'RD' => '',
        'ZIP' => '',
        'CONTACTUSURL' => '',
        'COUNTRYENAME' => '',
        'T_COUNTRYID' => '',
        'PICURL' => '',
        'BYTES' => '',
        'LPATH' => '',
        'PICLPATH' => '',
        'SHOWROOMURL' => '',
        'SHOWROOMMD5' => '',
        'DOMAIN_ID' => '',
        'ISPICEXIST' => '',
        'QR' => '');
    }
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
function ActionUrlinfoForm(form, opvalue)
{
    form.op.value = opvalue;
    form.opsubmit.disabled = true;
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
<FORM action='' method='POST' name='URL' >
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
<FORM action='' method='POST' name='data' 
    OnAction='javascript:return ActionDataForm(this);' >
<input type=hidden name='op' value='data_query'>
<TR>
<TD bgcolor='#CCCCCC' align=left width=100 >
   <b>数据查询:</b>
   </TD>
	<TD bgcolor='#CCCCCC' align=left width >
    关键词<input type=text name='keyword' size=32 value='<? echo $keyword;?>'>
    类别<select name='data_type' >
    <?php $type_list = Array('product' => '产品', 'company' => '公司');
    foreach($type_list as $k => $v)
    {
        if($k == $data_type)
            echo "<option value='$k' selected>$v</option>";
        else
            echo "<option value='$k' >$v</option>";
    }
    ?>
    </select>
    <input type=submit name='submit' value='查询' >
	</TD>	
</TR>
</FORM>
</TABLE>

<?
if($op == 'url_query')
{
    if(isset($url_data_array) && $url_data_array)
    {
        $result_string = "<TABLE cellspacing=1 bgcolor='#000000' width='100%' align=center>";
        $result_string .= "<FORM method = 'post' name='url_list' >";
        $result_string .= "<input type=hidden name='op' value='pchange' >";
        $result_string .= "<TR align='center'>"; 
        $result_string .= "<TH bgcolor='#C0C0C0' align=left >字段名</TH>"; 
        $result_string .= "<TH bgcolor='#C0C0C0' align=left >字段结果</TH>"; 
        $result_string .= "</TR>";
        foreach($url_data_feilds AS $k => $v)
        {
            if($v == 'Priority')
            {
                $result_string .= "<TR align='center'>"; 
                $result_string .= "<TD bgcolor='#ffffff' align=left width=200 ><b>".$k."</b></TD>"; 
                $result_string .= "<TD bgcolor='#ffffff' align=left>".$url_data_array[$v];
                $result_string .= "<select name='priority' >";
                $priority_list = Array('1' => '紧急', '2' => '尽快');
                foreach($priority_list as $opv => $optxt)
                {
                    $result_string .= "<option value='$opv'>$optxt</option>";
                }
                $result_string .= "</select>";
                $result_string .= "<input type=button name='opsubmit' value='修改优先级' ";
                $result_string .= " onclick='javascript:ActionUrlinfoForm(this, \'pchange\')'>"; 
                $result_string .= "</TD></TR>"; 
            }
            else
            {
                $result_string .= "<TR align='center'>";
                $result_string .= "<TD bgcolor='#ffffff' align=left width=200 ><b>".$k."</b></TD>";
                $result_string .= "<TD bgcolor='#ffffff' align=left>".$url_data_array[$v]."</TD>";
                $result_string .= "</TR>";
            }
        }
            $result_string .= "<TR align='center'>"; 
                $result_string .= "<TD bgcolor='#ffffff' align=left width=200 ></TD>"; 
                $result_string .= "<TD bgcolor='#ffffff' align=left>";
                $result_string .= "<input type=button name='opsubmit' value='重新抓取' ";
                $result_string .= " onclick='javascript:ActionUrlinfoForm(this, \'redownload\')'>"; 
                $result_string .= "</TD></TR>"; 

        $result_string .= "</FORM></TABLE>";
    }
    else
    {
        $result_string = red("没有找到[$url]相关记录");
    }
}
if($op == 'data_query')
{
    if($data_type == 'company')
    {
        if(isset($data_company_array) && $data_company_array)
        {
        
        }
        else
        {
            $result_string = red("没有找到[$keyword]相关的公司数据");
        }
    }
    if($data_type == 'product')
    {
        if(isset($data_product_array) && $data_product_array)
        {
        
        }
        else
        {
            $result_string = red("没有找到[$keyword]相关的产品数据");
        }
    }

}
echo $result_string;
?>
<?
include_once('./include/footer.inc.php');
?>
