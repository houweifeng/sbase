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
if($op == 'pchange')
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
            $strings = "";
        }
        else
        {
            $strings = "";
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
<FORM action='query.php' method='POST' name='URL' >
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
<FORM action='query.php' method='POST' name='data' 
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
        $result_string = "<TABLE cellspacing=1 bgcolor='#000000' width='100%' align=center>\n";
        $result_string .= "<FORM action='query.php' method='post' name='url_list' >\n";
        $result_string .= "<input type=hidden name='op' value='pchange' >\n";
        $result_string .= "<TR align='center'>\n"; 
        $result_string .= "<TH bgcolor='#C0C0C0' align=left >名称</TH>\n"; 
        $result_string .= "<TH bgcolor='#C0C0C0' align=left >结果</TH>\n"; 
        $result_string .= "</TR>\n";
        foreach($url_data_feilds AS $k => $v)
        {
            if($v == 'Priority')
            {
                $result_string .= "<TR align='center'>\n"; 
                $result_string .= "<TD bgcolor='#ffffff' align=left width=200 ><b>".$k."</b></TD>\n"; 
                $result_string .= "<TD bgcolor='#ffffff' align=left>".$url_data_array[$v];
                $result_string .= "<select name='priority' onchange='";
                $result_string .= "javascript:ActionUrlinfoForm(this.form, \"pchange\");'>\n";
                foreach($priority_list AS $opv => $optxt)
                {
                    $result_string .= "<option value='$opv'>$optxt</option>\n";
                }
                $result_string .= "</select>\n";
                $result_string .= "</TD></TR>\n"; 
            }
            else
            {
                $result_string .= "<TR align='center'>\n";
                $result_string .= "<TD bgcolor='#ffffff' align=left width=200 ><b>".$k."</b></TD>\n";
                $result_string .= "<TD bgcolor='#ffffff' align=left>".$url_data_array[$v]."</TD>\n";
                $result_string .= "</TR>\n";
            }
        }
        $result_string .= "<TR align='center'>\n"; 
        $result_string .= "<TD bgcolor='#ffffff' align=left width=200 ></TD>\n"; 
        $result_string .= "<TD bgcolor='#ffffff' align=left>\n";
        $result_string .= "<input type=button name='opsubmit' value='重新抓取' ";
        $result_string .= " onclick='javascript:ActionUrlinfoForm(this.form, \"redownload\");'>\n"; 
        $result_string .= "</TD></TR>\n"; 
        $result_string .= "</FORM></TABLE>\n";
    }
    else
    {
        $result_string = red("没有找到[$url]相关记录");
    }
    echo $result_string;
}
if(isset($_GET['op']) && $_GET['op'] == 'view_info')
{
    $objdata = new cDatadb($dbdata);
    $objstore = new cStoredb($dbstore);
    $url = mysql_escape_string($_GET['url']);
    $pri = $objdata->find_url($url);
    $result_string = "<TABLE cellspacing=1 bgcolor='#000000' width='100%' align=center>\n";
    $result_string .= "<FORM action='query.php' method='post' name='info_list' >\n";
    $result_string .= "<input type=hidden name='op' value='pchange' >\n";
    $result_string .= "<input type=hidden name='url' value='$url'>\n";
    $result_string .= "<TR align='center'>\n";
    $result_string .= "<TH bgcolor='#C0C0C0' align=left >名称</TH>\n";
    $result_string .= "<TH bgcolor='#C0C0C0' align=left >结果</TH>\n";
    $result_string .= "</TR>\n";
    $result_string .= "<TR align='center'>\n";
    $result_string .= "<TD bgcolor='#c0c0c0' align=left width=200 ><b>优先级</b></TD>\n";
    $result_string .= "<TD bgcolor='#ffffff' align=left>\n";
    $result_string .= "<select name='priority' onchange='";
    $result_string .= "javascript:ActionUrlinfoForm(this.form, \"pchange\");'>\n";
    foreach($priority_list AS $opv => $optxt)
    {
        if($opv == $pri['Priority'])
            $result_string .= "<option value='$opv' selected>$optxt</option>\n";
        else
            $result_string .= "<option value='$opv'>$optxt</option>\n";
    }
    $result_string .= "</select>\n";
    $result_string .= "</TD></TR>\n";

    //company info
    if(isset($_GET['companyid']))
    {
        $arr = Array();
        $companyid = mysql_escape_string($_GET['companyid']);
        $data = $objstore->get_company($companyid); 
        foreach($company_data_feilds AS $k => $v)
        {
            $result_string .= "<TR align='center'>\n";
            $result_string .= "<TD bgcolor='#C0C0C0' align=left width=200 ><b>".$k."</b></TD>\n";
            $result_string .= "<TD bgcolor='#ffffff' align=left>".$data[$v]."</TD>\n";
            $result_string .= "</TR>\n";
        }
    }
    //product info
    if(isset($_GET['productid']))
    {
        $arr = Array();
        $productid = mysql_escape_string($_GET['productid']);
        $data = $objstore->get_product($productid); 
        $pri = $objdata->find_url($url);
        foreach($product_data_feilds AS $k => $v)
        {
            $result_string .= "<TR align='center'>\n";
            $result_string .= "<TD bgcolor='#c0c0c0' align=left width=200 ><b>".$k."</b></TD>\n";
            $result_string .= "<TD bgcolor='#ffffff' align=left>".$data[$v]."</TD>\n";
            $result_string .= "</TR>\n";
        }   
    }

    $result_string .= "</FORM></TABLE>\n";
    echo $result_string;
}
if($op == 'data_query')
{
    $objstore = new cStoredb($dbstore);
    if($data_type == 'product')
    {
        $arr = Array();
        $product_feilds_list = Array(
                'INFOID' => '产品ID', 
                'PRODUCTCNAME' => '产品中文名称', 
                'PRODUCTENAME' => '产品英文名称',
                'URL' => 'URL');
        $data = $objstore->find_product($keyword);
        foreach($product_feilds_list AS $k => $v)
            $arr['title'][$k] = '^'.jsescape($v);
        if(is_array($data))
        {
            for($i = 0; $i < count($data); $i++)
            {
                $id = $data[$i]['INFOID'];
                $surl = $data[$i]['URL'];
                foreach($product_feilds_list AS $k => $v)
                {
                    if($k == 'INFOID')
                        $arr[$id][$k] = "^".jsescape($data[$i][$k]);
                    else
                    {           
                        $link = "<a href='query.php?op=view_info";
                        $link .= "&productid=$v&url=$surl'>".$data[$i][$k]."</a>";
                        $arr[$id][$k] = jsescape($link);
                    }
                }
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
                'URL' => 'URL');
        $data = $objstore->find_company($keyword);    
        foreach($company_feilds_list AS $k => $v)
            $arr['title'][$k] = '^'.jsescape($v);
        if(is_array($data))
        {
            for($i = 0; $i < count($data); $i++)
            {
                $id = $data[$i]['INFOID'];
                $surl = $data[$i]['URL'];
                foreach($company_feilds_list AS $k => $v)
                {
                    if($k == 'INFOID')
                        $arr[$id][$k] = "^".jsescape($data[$i][$k]);
                    else
                    {           
                        $link = "<a href='query.php?op=view_info";
                        $link .= "&companyid=$v&url=$surl'>".$data[$i][$k]."</a>";
                        $arr[$id][$k] = jsescape($link);
                    }
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
