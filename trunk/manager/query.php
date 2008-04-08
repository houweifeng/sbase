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
//add URL
if($op == 'url_add')
{
    if($url)
    {
        $uri = new cInterface(URL_SVR_HOST, URL_SVR_PORT);
        $uri->addurl($url, $priority);
        if($uri->send())
        {
            $strings = "添加URL[$url]优先级成功!";
        }
        else
        {
            $strings = red("添加URL[$url]优先级失败!");
        }
    }
}

//change priority 
if($op == 'pri_change')
{
    if($url)
    {
        $uri = new cInterface(URL_SVR_HOST, URL_SVR_PORT);
        $uri->addurl($url, $priority);
        if($uri->send())
        {
            $strings = "修改URL[$url]优先级成功!";
        }
        else
        {
            $strings = red("修改URL[$url]优先级失败!");
        }
    }
}

//redownload
if($op == 'redownload')
{
    if($url)
    {
        $uri = new cInterface(URL_SVR_HOST, URL_SVR_PORT);
        $uri->addurl($url, $priority);
        if($uri->send())
        {
            $data = "";
        }
        else
        {
            $data = "";
        }
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
    <?php 
    foreach($query_data_type_list as $k => $v)
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
                foreach($priority_list AS $opv => $optxt)
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
    $storedb = new cStoredb($db);
    if($data_type == 'product')
    {
        $arr = Array();
        $product_feilds_list = Array(
            'INFOID' => '产品ID', 
            'PRODUCTCNAME' => '产品中文名称', 
            'PRODUCTENAME' => '产品英文名称',
            'URL' => 'URL');
        $data = $storedb->find_product($keyword);
            foreach($product_feilds_list AS $k => $v)
                $arr['title'][$k] = '^'.jsescape($v);
        if(is_array($data))
        {
            for($i = 0; $i < count($data); $i++)
            {
                $id = $data[$i]['INFOID'];
                foreach($product_feilds_list AS $k => $v)
                    $arr[$id][$k] = jsescape($data[$i][$k]);
            }
        }
    }
    if($data_type == 'company')
    {
        $arr = Array();
        $company_feilds_list = Array(
                'INFOID' => '公司ID', 
                'PRODUCTCNAME' => '公司中文名称', 
                'PRODUCTENAME' => '公司英文名称',
                'URL' => 'URL',
                'OP' => '操作');
        $data = $storedb->find_company($keyword);    
        foreach($company_feilds_list AS $k => $v)
            $arr['title'][$k] = '^'.jsescape($v);
        if(is_array($data))
        {
            for($i = 0; $i < count($data); $i++)
            {
                $id = $data[$i]['INFOID'];
                foreach($company_feilds_list AS $k => $v)
                    $arr[$id][$k] = jsescape($data[$i][$k]);
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
