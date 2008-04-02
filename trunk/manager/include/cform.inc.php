<?php
/*****************************************************************************
    $Id: cform.inc.php,v 1.17 2002/12/17 18:24:05 djresonance Exp $
    Copyright 2002 Brandon Tallent

    This file is part of phpTest.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*****************************************************************************/

// class for outputting XHTML compliant forms

class cForm {
    var $_action;
    var $_method;
    var $_name;
    var $_form_string;
    var $_form_draw_string;
    var $_body_string;
    var $_form_head;
    var $_form_foot;

    function cForm($action='',$method='POST')
    {
        // set some defaults
	$action	= ($action)?$action:$_SERVER['PHP_SELF'];
        $this->set_action($action);
        $this->set_method($method);
    }

    function add_feedback($text, $breaks) {
        $string = "<span class=\"feedback\">" . addslashes($text) . "</span>\n";

        for ($i = 0; $i < $breaks; $i++) {
            $string .= '<br />';
        }

        $this->_form_string[] = $string;
	return $string;
    }

    function add_text($text, $breaks, $class = '')
    {
        $string = $text;


        if (!empty($class)) {
            $string = "<div class=\"$class\">$string</div>";

            for ($i = 0; $i < $breaks; $i++) {
                $string .= '<br />';
            }

        } else {

            for ($i = 0; $i < $breaks; $i++) {
                $string .= '<br />';
            }

        }

        $this->_form_string[] = $string;
	return $string;
    }

    function checkbox($name, $value, $breaks, $checked = '')
    {
        $string = "<input type=\"checkbox\" name=\"$name\"";

        if ($checked) {
            $string .= " checked=\"checked\"";
        }

        $string .= " value=\"$value\" />\n";

        for ($i = 0; $i < $breaks; $i++) {
            $string .= '<br />';
        }

        $this->_form_string[] = $string;
	return $string;
    }

    function draw($enctype = '', $submitvar = 'submit')
    {
        if (!$this->_action) {
            $this->set_action($_SERVER['PHP_SELF']);
        }

        echo "<div class=\"form\">\n";
        echo "<form";

        if (!empty($enctype)) {
            echo " enctype=\"$enctype\"";
        }

        echo " method=\"$this->_method\" action=\"$this->_action\"";

        if ($this->_name) {
            echo " name=\"$this->_name\"";
        }


        echo ">\n";

        foreach ($this->_form_string as $form_string) {
            echo $form_string;
        }

        //echo "<input type=\"submit\" name=\"$submitvar\" value=\"{$strings['BUTTON_SUBMIT']}\" />\n";
        echo "<input type=\"submit\" name=\"$submitvar\" />\n";
        echo "</form>\n";
        echo "</div>\n";
        echo "<br /><br />\n";
    }
    function _draw($enctype = '', $submitvar = 'submit')
    {
        if (!$this->_action) {
            $this->set_action($_SERVER['PHP_SELF']);
        }

        $this->_form_draw_string =  "<div class=\"form\">\n";
        $this->_form_draw_string .= "<form";

        if (!empty($enctype)) {
             $this->_form_draw_string .= " enctype=\"$enctype\"";
        }

         $this->_form_draw_string .=  " method=\"$this->_method\" action=\"$this->_action\"";

        if ($this->_name) {
             $this->_form_draw_string .= " name=\"$this->_name\"";
        }


         $this->_form_draw_string .=  ">\n";

        foreach ($this->_form_string as $form_string) {
            $this->_form_draw_string .= $form_string;
        }

        //echo "<input type=\"submit\" name=\"$submitvar\" value=\"{$strings['BUTTON_SUBMIT']}\" />\n";
         $this->_form_draw_string .=  "<input type=\"submit\" name=\"$submitvar\" />\n";
         $this->_form_draw_string .=  "</form>\n";
         $this->_form_draw_string .=  "</div>\n";
         $this->_form_draw_string .=  "<br /><br />\n";
	 return $this->_form_draw_string;
    }
    function head($enctype = ''){
	 if (!$this->_action) {
            $this->set_action($_SERVER['PHP_SELF']);
        }
        $this->_form_draw_head = "<form";

        if (!empty($enctype)) {
             $this->_form_head .= " enctype=\"$enctype\"";
        }

         $this->_form_head .=  " method=\"$this->_method\" action=\"$this->_action\"";

        if ($this->_name) {
             $this->_form_head .= " name=\"$this->_name\"";
        }
	$this->_form_head .=  ">\n";
	return $this->_form_head;
    }
    function foot(){
	$this->_form_foot = "</form>\n";
	return $this->_form_foot;
    }
    function submit($submitvar = 'submit',$submitval=''){
	$this->_form_submit  =  "<input type=\"submit\" name=\"$submitvar\" ";
	$this->_form_submit .= ($submitval?"value=\"".$submitval."\"":'')."/>\n";
	return $this->_form_submit;
    }
    function body(){
	$this->_body_string     = '';
	foreach ($this->_form_string as $form_string) {
            $this->_body_string .= $form_string;
        }
	return $this->_body_string;
    }

    function file($name, $breaks)
    {
        $string = "<input type=\"file\" name=\"$name\"";
        $string .= " />\n";

        for ($i = 0; $i < $breaks; $i++) {
            $string .= '<br />';
        }

        $string .= "\n";
        $this->_form_string[] = $string;
	return $string;
    }

    function hidden($name, $value)
    {
        $string = "<input type=\"hidden\" name=\"$name\" value=\"$value\" />\n";
        $this->_form_string[] = $string;
	return $string;
    }

    function input($name, $breaks, $value = '', $size = '', $maxlength = '')
    {
        $string = "<input type=\"text\" name=\"$name\"";

        if (isset($value)) {
            $string .= " value=\"$value\"";
        }

        if (!empty($size)) {
            $string .= " size=\"$size\"";
        }

        if (!empty($maxlength)) {
            $string .= " maxlength=\"$maxlength\"";
        }

        $string .= " />\n";

        for ($i = 0; $i < $breaks; $i++) {
            $string .= '<br />';
        }

        $string .= "\n";
        $this->_form_string[] = $string;
	return $string;
    }

    function password($name, $breaks, $value = '', $maxlength = '')
    {
        $string = "<input type=\"password\" name=\"$name\"";
        if ($value) $string .= " value = \"$value\"";
        if ($maxlength) $string .= " maxlength=\"$maxlength\"";
        $string .= " />\n";

        for ($i = 0; $i < $breaks; $i++) {
            $string .= '<br />';
        }

        $string .= "\n";
        $this->_form_string[] = $string;
	return $string;
    }

    /**
    * select
    *
    * Adds an html select box to the form queue to be drawn
    *
    * @param      string    $name     The html name of the select box
    * @param      array     $values   An array of values to put the select boxes in
    * @param      int       $breaks   The number of <br>s to put after the select box
    * @param      string    $selected (optional) The value which should initially be selected
    * @param      string    $onchange (optional) The value of the onchange javascript parameter
    * @return     none
    * @access     public
    */

    function select($name, $values, $breaks, $selected = '', $onchange = '')
    {
        $string = "<select name=\"$name\">\n";

        foreach($values as $value) {
            $string .= " <option ";

            if ($selected == $value) {
                $string .= " selected=\"selected\"";
            }

            $string .= ">$value</option>\n";
        }

        $string .= "</select>\n";

        for ($i = 0; $i < $breaks; $i++) {
            $string .= '<br />';
        }

        $string .= "\n";
        $this->_form_string[] = $string;
	return $string;
    }

    function set_action($action)
    {
        $this->_action = $action;
    }

    function set_method($method)
    {
        $this->_method = $method;
    }

    function set_name($name)
    {
        $this->_name = $name;
    }

    function textarea($name, $breaks, $value = '', $rows = 10, $cols = 60)
    {
        $string = "<textarea name=\"$name\" rows=\"$rows\" cols=\"$cols\">";
        $string .= $value;
        $string .= "</textarea>\n";

        for ($i = 0; $i < $breaks; $i++) {
            $string .= '<br />';
        }

        $string .= "\n";
        $this->_form_string[] = $string;
	return $string;
    }

}
?>
