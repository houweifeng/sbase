<?php
/* 
 *  Copyright 2001-2002 by Dudu. Inc., all rights reserved.
 *
 *  These coded instructions, statements, and computer programs are the
 *  property of Dudu. Inc. and are protected by copyright laws.  
 *  Copying, modification, distribution and use without Dudu. 
 *  Inc.'s permission are prohibited. 
 *
 */
?>

    
<table cellspacing="0" cellpadding="5" border="0">
  <form method="post" action="login.php">
      <tr> 
        <td align="right">用户名</td>
        <td> 
          
        <input type="text" name="username" maxlength="<?php echo $max_username_length; ?>" size="10" />
        </td>
        <td>密码</td>
        <td> 
          
        <input type="password" name="password" maxlength="<?php echo $max_password_length; ?>" size="10" />
        </td>
        <td> 
          <input type="hidden" name="op" value="login" />
          <input type="submit" name="submit" value="登录" />
        </td>
      </tr>
</form>
    </table>

