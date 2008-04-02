<?php
/* 
 *
 */
    echo "<div class=\"menu\">";
    echo '<b>&nbsp;&nbsp;&nbsp;&nbsp;'.$sess_user->user_name .' '.MENU_HELLO. '</b>&nbsp;&nbsp;&nbsp;&nbsp;';
    echo make_href("login.php?op=change&amp;username=".$sess_user->user_name, MENU_CHANGE_PASSWORD);
    echo "&nbsp;&nbsp;&nbsp;";
    echo make_href("login.php?op=logout", MENU_LOGOUT) ;
?>
