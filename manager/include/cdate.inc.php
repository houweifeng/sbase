<?php
/* 
 *  Copyright 2001-2002 by NDA System Inc., all rights reserved.
 *
 *  These coded instructions, statements, and computer programs are the
 *  property of NDA System Inc. and are protected by copyright laws.  
 *  Copying, modification, distribution and use without NDA System 
 *  Inc.'s permission are prohibited. 
 *
 */

/* This function generates a date and or time function 
   based on the format and default time that you enter*/
function create_date_select($In_field_name, $In_date_format_string, $In_unix_time)
{ 
        if (!$In_unix_time)
        	{
                $In_unix_time = time();
        	}

        for ($j=0;$j<strlen($In_date_format_string);$j++)
        	{ 
		// looping through the format string
                $L_format_char = substr($In_date_format_string,$j,1);
                switch ($L_format_char)
                	{ 
			// each character in the format string means something
                        case "m": // numeric month
                                ?>
                                <select name=<?echo $In_field_name . "[m]"?>>
                                <?
                                        for ($i=1;$i<13;$i++)
                                        	{
                                                $L_var_month = date("$L_format_char",mktime(0,0,0,$i,15,2000));
                                                if ($L_var_month == date("$L_format_char",$In_unix_time))
                                                	{
                                                        $L_selected=" SELECTED ";
                                                	}
                                                else
                                                	{
                                                        unset($L_selected);
                                                	}
                                                print ("<option value=$i $L_selected>"  . $L_var_month . "</option> \n");
                                        	}                       
                                ?>
                                </select>
                                <?
                                break;

                        case "F": // textual full month
                                ?>
                                <select name=<?echo $In_field_name . "[m]"?>>
                                <?
                                        for ($i=1;$i<13;$i++)
                                        	{
                                                $L_var_month = date("$L_format_char",mktime(0,0,0,$i,15,2000));
                                                if ($L_var_month == date("$L_format_char",$In_unix_time))
                                                	{
                                                        $L_selected=" SELECTED ";
                                                	}
                                                else
                                                	{
                                                        unset($L_selected);
                                                	}
                                                print ("<option value=$i $L_selected>"  . $L_var_month . "</option>
\n");
                                        	}                       
                                ?>
                                </select>
                                <?
                                break;

                        case "d":               
                                ?>
                                <select name=<?echo $In_field_name . "[d]"?>>
                                <? for ($i=1;$i<32;$i++)
                                	{
                                        if ($i == date("$L_format_char",$In_unix_time))
                                        	{
                                                $L_selected=" SELECTED ";
                                        	}
                                        else
                                        	{
                                                unset($L_selected);
                                        	}
                                        print("<option value=$i $L_selected>$i</option>\n");
                                	}       
                                ?>      
                                </select>
                                <?
                                break;

                        case "D":               
                                ?>
                                <select name=<?echo $In_field_name . "[d]"?>>
                                <? for ($i=1;$i<8;$i++)
                                	{
                                        $L_var_textual_day = date("$L_format_char",mktime(0,0,0,8,$i,1999));
                                        if ($L_var_textual_day == date("$L_format_char",$In_unix_time))
                                        	{
                                                $L_selected=" SELECTED ";
                                        	}
                                        else
                                        	{
                                                unset($L_selected);
                                        	}
                                        print("<option value=$i $L_selected>$L_var_textual_day</option>\n");
                                	}       
                                ?>      
                                </select>
                                <?
                                break;

                        case "y":
                                ?>
                                <select name=<?echo $In_field_name . "[y]"?>>
                                <? for ($i = 1996; $i <= (date($L_format_char) + 8); $i++) // starting in 1997
                                	{
                                        if ($i == date("$L_format_char",$In_unix_time))
                                        	{
                                                $L_selected=" SELECTED ";
                                        	}
                                        else
                                        	{
                                                unset($L_selected);
                                        	}
                                        print("<option value=$i $L_selected>$i</option>\n");
                                	}
                                ?>
                                </select>       
                                <?
                                break;

                        case "Y":
                                ?>
                                <select name=<?echo $In_field_name . "[y]"?>>
                                <? for ($i = 1996; $i <= (date($L_format_char) + 7); $i++) // starting in 1997
                                	{
                                        if ($i == date("$L_format_char",$In_unix_time))
                                        	{
                                                $L_selected=" SELECTED ";
                                        	}
                                        else
                                        	{
                                                unset($L_selected);
                                        	}
                                        print("<option value=$i $L_selected>$i</option>\n");
                               	 	}
                                ?>
                                </select>       
                                <?
                                break;

                        case "h":
                                ?>
                                <select name=<?echo $In_field_name . "[h]"?>>
                                <? for ($i = 1; $i < 13; $i++)
                                	{
                                        if ($i == date("$L_format_char",$In_unix_time))
                                        	{
                                                $L_selected=" SELECTED ";
                                        	}
                                        else
                                        	{
                                                unset($L_selected);
                                        	}
                                        print("<option value=$i $L_selected>$i</option>\n");
                                	}
                                ?>
                                </select>       
                                <?
                                break;

                        case "g":
                                ?>
                                <select name=<?echo $In_field_name . "[h]"?>>
                                <? for ($i = 1; $i < 13; $i++)
                                	{
                                        if ($i == date("$L_format_char",$In_unix_time))
                                        	{
                                                $L_selected=" SELECTED ";
                                        	}
                                        else
                                        	{
                                                unset($L_selected);
                                        	}
                                        print("<option value=$i $L_selected>$i</option>\n");
                                }
                                ?>
                                </select>       
                                <?
                                break;


                        case "H":
                                ?>
                                <select name=<?echo $In_field_name . "[h]"?>>
                                <? for ($i = 0; $i < 24; $i++)
                                	{
                                        if ($i == date("$L_format_char",$In_unix_time))
                                        	{
                                                $L_selected=" SELECTED ";
                                        	}
                                        else
                                        	{
                                                unset($L_selected);
                                        	}
                                        printf("<option value=%d %s>%02d</option>\n",$i,$L_selected,$i);
                                	}
                                ?>
                                </select>       
                                <?
                                break;

                        case "G":
                                ?>
                                <select name=<?echo $In_field_name . "[h]"?>>
                                <? for ($i = 0; $i < 24; $i++)
                                	{
                                        if ($i == date("$L_format_char",$In_unix_time))
                                        	{
                                                $L_selected=" SELECTED ";
                                        	}
                                        else
                                        	{
                                                unset($L_selected);
                                        	}
                                        print("<option value=$i $L_selected>$i</option>\n");
                               	 	}
                                ?>
                                </select>       
                                <?
                                break;

                        case "i":
                                ?>
                                <select name=<?echo $In_field_name . "[i]"?>>
                                <? for ($i = 0; $i < 60; $i++)
                                	{
                                        if ($i == date("$L_format_char",$In_unix_time))
                                        	{
                                                $L_selected=" SELECTED ";
                                        	}
                                        else
                                        	{
                                                unset($L_selected);
                                        	}
                                        printf("<option value=%d %s>%02d</option>\n",$i,$L_selected,$i);
                                	}
                                ?>
                                </select>       
                                <?
                                break;

                        case "s":
                                ?>
                                <select name=<?echo $In_field_name . "[s]"?>>
                                <? for ($i = 0; $i < 60; $i++)
                                	{
                                        if ($i == date("$L_format_char",$In_unix_time))
                                        	{
                                                $L_selected=" SELECTED ";
                                        	}
                                        else
                                        	{
                                                unset($L_selected);
                                        	}
                                        printf("<option value=%d %s>%02d</option>\n",$i,$L_selected,$i);
                                	}
                                ?>
                                </select>       
                                <?
                                break;

                        case "a":
                                ?>
                                <select name=<?echo $In_field_name . "[a]"?>>
                                <? 
                                        // AM
                                        $L_text_time = "am";
                                        if ($L_text_time == date(strtoupper($L_format_char),$In_unix_time))
                                        	{
                                                $L_selected=" SELECTED ";
                                        	}
                                        else
                                        	{
                                                unset($L_selected);
                                        	}
                                        print("<option value=1 $L_selected>$L_text_time</option>\n");

                                        // PM
                                        $L_text_time = "pm";
                                        if ($L_text_time == date(strtoupper($L_format_char),$In_unix_time))
                                        	{
                                                $L_selected=" SELECTED ";
                                        	}
                                        else
                                        	{
                                                unset($L_selected);
                                        	}
                                        print("<option value=2 $L_selected>$L_text_time</option>\n");
                                ?>
                                </select>       
                                <?
                                break;

                        case "A":
                                ?>
                                <select name=<?echo $In_field_name . "[a]"?>>
                                <? 
                                        // AM
                                        $L_text_time = "AM";
                                        if ($L_text_time == date(strtoupper($L_format_char),$In_unix_time))
                                        	{
                                                $L_selected=" SELECTED ";
                                        	}
                                        else
                                        	{
                                                unset($L_selected);
                                        	}
                                        print("<option value=1 $L_selected>$L_text_time</option>\n");

                                        // PM
                                        $L_text_time = "PM";
                                        if ($L_text_time == date(strtoupper($L_format_char),$In_unix_time))
                                        	{
                                                $L_selected=" SELECTED ";
                                        	}
                                        else
                                        	{
                                                unset($L_selected);
                                        	}
                                        print("<option value=2 $L_selected>$L_text_time</option>\n");
                                ?>
                                </select>       
                                <?
                                break;
                        default:
                                echo $L_format_char; // not a recognized character, so echo it out
                	} // end of switch
        	} // end of for
}

function process_date_select($In_time_array)
	{
	// This function processes the date select object on submit
        if ($In_time_array[a]==2 and ($In_time_array[h] < 12))
        	{ 
		// the user chose pm
                $In_time_array[h] = $In_time_array[h] + 12;
        	}
        else
        if ($In_time_array[a] == 1 and $In_time_array[h] == 12)
        	{
                $In_time_array[h] = 0;
        	}
		if(!checkdate($In_time_array[m],$In_time_array[d],$In_time_array[y]))
			return -1;
        return mktime($In_time_array[h],$In_time_array[i],$In_time_array[s],$In_time_array[m],$In_time_array[d],$In_time_array[y]);
	}

function select_date($type,$name,$start_date,$view_date,$limit,$onchange,$select2)
//added "$startday," on 2004-5-18
{
?>

<SCRIPT>

function popupCalendar(URL,option,select) 
	{
	day = new Date();
	id = day.getTime();

	staticPopup(URL);
	//eval("calendar"+id+" = window.open(URL, '" + id + "', 'toolbar=0,scrollbars=0,location=0,statusbar=0,menubar=0,resizable=0,width=200,height=100');");
	//eval("if (!calendar"+id+".opener) calendar"+id+".opener = self; ");
	self.dateselect = option;
	self.dateselectselect = select;
	}
</SCRIPT>
<?php
    /*
    if ($onchange) 
	if ($onchange=="select")
		{
		$onchange_js="true";
		$onchange_select = "onchange=\"$select2\"";
		}
	else
		{
		$onchange_select = "onchange=\"$onchange\"";
		}
    
    $today = date("Y-m-d",time());   
    if (strpos($date," ") > 0) $date = substr($date,0,10);
    if(!$start_date)
	{
	$start_date=$today;
	}
    $date_time = strtotime($start_date);
    
    
	if ($startday==$today) 
	{
	$date_unix -= 60*60*24;
	}
   */ 
$today = date("Y-m-d",time());
        if(!$start_date)
                {
                $start_date=$today;
                }
//$select = "<select id='$name' name='$name' $onchange_select>\n";
//$p_msg = "<IFRAME frameBorder=1 id=heads src=\"popup_calendar.php?\" ></IFRAME>";
//$popup_msg = addslashes(htmlspecialchars($p_msg));
if($type==1)
	{
    	if ($onchange)
		{
        	if ($onchange=="select")
                	{
                	$onchange_js="true";
                	$onchange_select = "onchange=\"$select2\"";
                	}
        	else
                	{
               	 	$onchange_select = "onchange=\"$onchange\"";
               	 	}
		}       	
	//$today = date("Y-m-d",time());
    	if (strpos($date," ") > 0) $date = substr($date,0,10);
    	$date_time = strtotime($start_date);
 	$select = "<select id='$name' name='$name' $onchange_select>\n";
        $p_msg = "<IFRAME frameBorder=1 id=heads src=\"popup_calendar.php?type=1\" ></IFRAME>";
        $popup_msg = addslashes(htmlspecialchars($p_msg));

    	if ($start_date==$today)
        	{
		$limit--;
        	$date_time -= 60*60*24;
		$select .="\t<option value='$today' selected >今天</option>\n";
        	}
    	else
        	{
  		$select .="\t<option value='$today' >今天</option>\n";
        	}
		//changed on 2004-5-18
   		//$select .="\t<option value='$today'>今天</option>\n";	
    	for ($i = 0; $i < $limit; $i++)
		{
		$day = date("Y-m-d",$date_time-(3600*24*$i));
		//echo $today.$view_date;
		if ($day==$view_date) 
			$selected = "selected";
	    	else 
			$selected="";
		$select .="\t<option value='$day'$selected >$day</option>\n";
    		}
    	$last = Array();
    	$aux = Array();
    	$aux["date"] = date("Y-m-d",$date_time-(60*60*24*6));
    	$aux["name"] = "一个礼拜前";
    	$last[] = $aux;

    	$aux["date"] = date("Y-m-d",$date_time-(60*60*24*30));
    	$aux["name"] = "一个月前";
    	$last[] = $aux;
    
    	$aux["date"] = date("Y-m-d",$date_time-(60*60*24*350));
    	$aux["name"] = "一年前";
    	$last[] = $aux;   
    	foreach ($last as $day) 
		{
		$select .="\t<option value='".$day["date"]."'>".$day["name"]."</option>\n";
		}
$select .= "\n</select><a href=\"javascript:popupCalendar('$popup_msg','$name','$onchange_js')\"><img border=0 src=\"images/calendar.png\"></a>\n";
	}
else
	{
	$p_msg = "<IFRAME frameBorder=1 id=heads src=\"popup_calendar.php?type=2\" ></IFRAME>";
	$popup_msg = addslashes(htmlspecialchars($p_msg));
	$select="从<select id='start_date'   name='start_date' >";
	$select.="<option>开始日期</option>