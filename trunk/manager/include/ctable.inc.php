<?php
/*****************************************************************************
    $Id: ctable.inc.php,v 1.11 2002/08/19 20:21:35 djresonance Exp $
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

// class for outputting XHTML compliant tables

class cTable {
	var $_border_color;
	var $_bg_color;
	var $_cellpadding;
	var $_cellspacing;
	var $_center;
	var $_colspan;
	var $_header_color;
	var $_header_text;
	var $_table_string = Array();
	var $_td_bg_color;
	var $_table_draw_string;

	function cTable($BG_COLOR='#000000',$HEADER_COLOR='#eeeeee',$TD_COLOR='#FFFFFF',$colspan=10,$cellspacing=1,
	$cellpadding=5,$width='100%')
	{
		// set some defaults as defined in config.inc.php
		$this->set_bg_color($BG_COLOR);
		$this->set_header_color($HEADER_COLOR);
		$this->set_td_bg_color($TD_COLOR);
		$this->set_colspan($colspan);
		$this->set_cellspacing($cellspacing);
		$this->set_cellpadding($cellpadding);
		$this->set_width($width);
		$this->set_center(0);
	}

	// whether or not to center text in each <td>
	// 0 for not centered, 1 for centered
	function set_center($center)
	{
		$this->_center = $center;
	}

	function draw()
	{
		echo "<table bgcolor=\"$this->_bg_color\" width=\"$this->_width\" cellspacing=\"$this->_cellspacing\" cellpadding=\"$this->_cellpadding\">";

		if ($this->_header_text) {
			echo "<tr>\n";
			echo " <td class=\"header\" bgcolor=\"$this->_header_color\" colspan=\"$this->_colspan\"><center>$this->_header_text</center></td>\n";
			echo "</tr>\n";
		}

		foreach ($this->_table_string as $table_string) {
			echo $table_string;
		}

		echo "</table><br /><br />\n";
	}
	function _draw()
        {
                $this->_table_draw_string = "<table bgcolor=\"$this->_bg_color\" width=\"$this->_width\" cellspacing=\"$this->_cellspacing\" cellpadding=\"$this->_cellpadding\">";

                if ($this->_header_text) {
                        $this->_table_draw_string .= "<tr>\n";
                        $this->_table_draw_string .= " <td class=\"header\" bgcolor=\"$this->_header_color\" colspan=\"$this->_colspan\"><center>$this->_header_text</center></td>\n";
                        $this->_table_draw_string .= "</tr>\n";
                }

                foreach ($this->_table_string as $table_string) {
                        $this->_table_draw_string .= $table_string;
                }

                $this->_table_draw_string .= "</table><br /><br />\n";

		return $this->_table_draw_string;
        }

	function set_border_color($color)
	{
		$this->_border_color = $color;
	}

	function set_bg_color($color)
	{
		$this->_bg_color = $color;
	}

	function set_colspan($colspan)
	{
		$this->_colspan = $colspan;
	}

	function set_header_color($color)
	{
		$this->_header_color = $color;
	}

	function set_cellpadding($cellpadding)
	{
		$this->_cellpadding = $cellpadding;
	}

	function set_cellspacing($cellspacing)
	{
		$this->_cellspacing = $cellspacing;
	}

	function set_width($width)
	{
		$this->_width = $width;
	}

	function set_header_text($header_text)
	{
		$this->_header_text = '<b>'.$header_text.'</b>';
	}

	function set_td_bg_color($color)
	{
		$this->_td_bg_color = $color;
	}

	function tr_start()
	{
		$this->_table_string[] = "<tr>\n";
	}

	function tr_end()
	{
		$this->_table_string[] = "</tr>\n";
	}

	function td($data, $colspan = '', $color = '', $css_class = 'small', $width = '', $valign = 'top')
	{
		$append = '';

		if ($colspan) {
			$append .= " colspan=\"$colspan\"";
		}

		$append .= $color ? " bgcolor=\"$color\"" : " bgcolor=\"$this->_td_bg_color\"";

		if ($width) {
			$append .= " width=\"$width\"";
		}

		if ($valign) {
			$append .= " valign=\"$valign\"";
		}

		$string = "<td$append><span class=\"$css_class\">";

		if ($this->_center) {
			$string .= '<center>';
		}

		$string .= $data;

		if ($this->_center) {
			$string .= '</center>';
		}

		$string .= "</span></td>\n";
		$this->_table_string[] = $string;
	}

	function th($data, $colspan = '', $color = '', $css_class = 'small', $width = '', $valign = 'top')
	{
		// initialize $append so php doesn't throw a notice
		$append = '';
		if ($colspan) $append .= " colspan=\"$colspan\"";
		if ($width) $append .= " width=\"$width\"";
		if ($valign) $append .= " valign=\"$valign\"";
		$string = "<th $append bgcolor=\"$this->_header_color\"><span class=\"$css_class\">";
		if ($this->_center) $string .= '<center>';
		$string .= $data;
		if ($this->_center) $string .= '</center>';
		$string .= "</span></th>\n";
		$this->_table_string[] = $string;
	}
}
?>
