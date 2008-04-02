<?php
define('SYSTEM_ADMIN','10000');
define('DATA_ADMIN','10001');
define('STRUCTURE_ADMIN','10002');
define('PERMISSION_ADMIN_NAME','权限管理');
define('PERMISSION_ADMIN_DESC','系统用户权限管理');
define('PERMISSION_DATA_ADMIN_NAME','资料管理');
define('PERMISSION_DATA_ADMIN_DESC','资料系统管理权限');
define('PERMISSION_STRUCTURE_ADMIN_NAME','网站管理');
define('PERMISSION_STRUCTURE_ADMIN_DESC','网站结构管理');
$____user_permissions_lists = Array(
	SYSTEM_ADMIN	=>	Array(
		'permission_id'		=> SYSTEM_ADMIN,
		'permission_name'	=> PERMISSION_ADMIN_NAME,
		'permission_desc'	=> PERMISSION_ADMIN_DESC,
	),
	DATA_ADMIN	=>	Array(
		'permission_id'		=> DATA_ADMIN,
		'permission_name'       => PERMISSION_DATA_ADMIN_NAME,
		'permission_desc'       => PERMISSION_DATA_ADMIN_DESC,
	),
	STRUCTURE_ADMIN      =>      Array(
                'permission_id'         => STRUCTURE_ADMIN,
                'permission_name'       => PERMISSION_STRUCTURE_ADMIN_NAME,
                'permission_desc'       => PERMISSION_STRUCTURE_ADMIN_DESC,
        ),
);
?>
