<?php
/* 
 * config file
 */
define('DB_HOST', '127.0.0.1:3306');
define('DB_USERNAME', 'dbadmin');
define('DB_PASSWORD', 'dbadmin');
define('DB_TYPE', 'mysql');
define('DB_PREFIX', '');
define('DB_NAME', 'manager_db');
define('DEFAULT_DB', 'manager_db');
define('SITE_NAME', '数据统计系统');
define('ABSOLUTE_PATH', '/view');
define('DEFAULT_PAGE', 'query.php');
define('WEBMASTER_EMAIL', 'admin@abc.com');
define('GRAPHCACHE_PATH', '/var/www/html/view/graphcache');

define('DEFAULT_USER_GROUP',1);
define('USER_NOT_EXISTS',11);
define('ERROR_PASSWORD',12);
define('ERROR_DATABASE',13);
define('USER_OFFLINE',14);
define('ERROR_UPDATE',15);
define('USER_DEFAULT_PASSWORD','123456');
define('REQUIRE_VALID_EMAIL', 0);
define('RECORD_PER_PAGE',10);
define('MULTICOL',5);

// min and max password and username lengths
define('MIN_PASSWORD_LENGTH', 4);
define('MAX_PASSWORD_LENGTH', 13);
define('MIN_USERNAME_LENGTH', 2);
define('MAX_USERNAME_LENGTH', 13);

// main colors
define('BG', '#ffffff');
define('TEXT', '#000000');
define('LINK', '#0000ff');
define('ALINK', '#FF9933');
define('VLINK', '#0000ff');
//the programe require urls
// help bubble colors
define('HELP_BACKGROUND', '#FEFFC0');
define('HELP_BORDER_COLOR', '#659ACC');
define('HELP_TEXT', '#000000');
define('HELP_BORDER_STYLE', 'dashed');

// form colors (for adding questions, users, etc...);
define('FORM_BACKGROUND', '#D6EBFF');
define('FORM_BORDER_COLOR', '#659ACC');
define('FORM_BORDER_STYLE', 'dotted');

// left-hand menu colors
define('MENU_BACKGROUND', '#F9F9F9');
define('MENU_BORDER_COLOR', '#659ACC');

// table colors
define('TABLE_BORDER', '#000000');
define('TABLE_BG', '#ffffff');
define('TABLE_LIGHT', '#BDDFFF');
define('TABLE_DARK', '#E5F3FF');
define('TABLE_HEADER', '#eeeeee');

$strings = array (
		'ERROR_PASSWORD_INCORRECT' => '密码不正确',
		'ERROR_USERNAME_TAKEN' => '用户名已经存在',
		'ERROR_PASSWORD_TOO_SHORT' => '密码太短',
		'ERROR_PASSWORDS' => '密码不合法',
		'ERROR_USERNAME_TOO_SHORT' => '用户名太短',
		'ERROR_USERNAME_TOO_LONG' => '用户名太长',
		'ERROR_NO_ACTION' => '没有动作',
		'ERROR_ADMIN_PRIV' => '您没有系统管理权限',
		'ERROR_USER_PRIV' => '您没有用户权限',
		'ERROR_FORM_INPUT' => '输入不正确',
		'ERROR_IP_NOTVAILD' => '非法IP地址!',
		'ERROR_PORT_NOTVAILD' => '非法端口!',
		'ERROR_DATE_NOTVAILD' => '输入日期不存在!',
		'ERROR_DATE_WRONG' => '输入日期错误!',
		'ERROR_BIG_CUSTOMER_TIP' => '大用户必须输入限制条件!',

		'LOG_LOGIN_FAIL' => '登录系统未成功',
		'LOG_LOGIN_SUCCESS' => '成功登录系统',
		'LOG_SWITCH_FAIL' => '系统管理员切换用户失败',
		'LOG_SWITCH_SUCCESS' => '系统管理员成功切换到用户',
		'LOG_LOGOUT'	=> '退出系统',
		'LOG_TITLE'	=> '系统登录记录',
		'LOG_IP'	=> 'IP地址',
		'LOG_USERNAME'	=> '用户',
		'LOG_EVENT'	=> '事件',
		'LOG_DATE'	=> '时间',
		'LOG_NO_RESULTS'	=>	'无事件',

		'HELP_CHANGE_PASSWORD' => '修改您的密码,密码长度必需大于5位',

		'MENU_ADMIN' => '您有系统管理员权限',
		'MENU_SUPPORT_ADMIN' => '您有产品支持权限',
		'MENU_SERVICE_ADMIN' => '您有客户服务权限',
		'MENU_USER_ADMIN' => '您有用户管理的权限',
		'MENU_VENDOR_ADMIN' => '您有数据查看权限',
		'MENU_ACCOUNT_ADMIN' => '您有帐户管理权限',
		'MENU_HELLO' => '您好!',
		'MENU_CHANGE_PASSWORD' => '修改密码',
		'MENU_LOGOUT' => '退出系统',
		'MENU_ADD_USER' => '增加用户',
		'MENU_EDIT_USER' => '查看/修改用户',
		'MENU_VIEW_SECURITY_LOG' => '查看用户登录记录',
		'MENU_VIEW_NATLOG' => '查询日志记录',

		'USER_PASSWORD_CHANGED' => '修改用户密码成功',
		'USER_OLD_PASSWORD' => '旧密码',
		'USER_NEW_PASSWORD' => '新密码',
		'USER_CONFIRM_NEW_PASSWORD' => '确认新密码',
		'USER_INVALID_EMAIL' => '您的EMAIL格式不正确',
		'USER_SUCCESS' => '用户添加成功',
		'USER_DESIRED_USERNAME' => '用户名',
		'USER_ISBIG_CUSTOMER' => '大用户',
		'USER_PASSWORD' => '输入用户密码',
		'USER_PASSWORD_CONFIRM' => '确认用户密码',
		'USER_EMAIL' => 'Email地址',
		'USER_DESC' => '描述' ,
		'USER_PHONE' => '电话' ,
		'USER_ACCESSLEVEL' => '用户权限设置',
		'USER_IPLIMIT' => '可登录节点限制' ,
		'USER_DIRACCESS' => '可访问目录' ,
		'USER_DELETED' => '成功删除用户',
		'USER_USERS' => '用户列表',
		'USER_ID' => '用户ID',
		'USER_DATE_ADDED' => '用户注册日期',
		'USER_ACCESS_LEVEL' => '用户权限',
		'USER_EDIT' => '编辑',
		'USER_DELETE' => '删除',
		'USER_CONFIRM_DELETE' => '您确定要删除这个用户吗',
		'USER_IPMAP' => '管理IP地址',
		'USER_NO_RESULTS' => '无符合条件用户',
		'USER_USERNAME' => '用户名',
		'USER_BIG_CUSTOMER_TIP' => '大用户限制条件,如voip',
		'USER_UPDATED' => '用户资料修改成功',
		'USER_NOTEXIST' => '用户不存在',

		'ACL_ANONYMOUS' =>	'未知用户',
		'ACL_AD_CUSTOMER'	=>	'广告用户',
		'ACL_BIG_CUSTOMER' =>	'大用户',
		'ACL_VIP_USER'	=> '公司用户',
		'ACL_ADMIN'	=>  '系统管理员',

		'FILE_TOO_LARGE' => '文件过大',
		'SEARCH_TITLE' => '搜寻日志记录',
		'DEBUG_USER' => '请选择要调试的用户名',

		'SUBMIT_EXCHANGE_LIST' => '请输入要提交的交换机/IP对应表,提交后自动覆盖原有对应表',
		'EXCHANGE_SERVER_LIST' => '交换机/IP对应表',
		'EXCHANGE_SERVER_NO_RESULTS' => '无记录,请提交相应对应表',
		'EXCHANGE_SERVER_NAME' => '交换机名称',
		'EXCHANGE_SERVER_IP' => '交换机IP',
		'EXCHANGE_SERVER_PROVINCE' => '省份',
		'EXCHANGE_SERVER_CONFIRM_SUBMIT' => '您确定要提交新的交换机/IP对应表覆盖原来的对应表?',
		'EXCHANGE_SERVER_ADDED' => '更新交换机/IP对应表成功',

		'VIEW_PROV_DIR_LIST' => '查看目录列表',
		'PROV_DIR_LIST' => '省份目录列表',
		'PROV_DIR_NORESULT' => '无省份目录!',
		'PROV_DIR_USED' => '该目录已经分配,不能重复使用!',

		'CT_TITLE' => '任务列表',
		'CT_NICKNAME' => '任务名称',
		'CT_NAME' => '交换机',
		'CT_IP' => 'IP',
		'CT_TYPE' => '类型',
		'CT_SLOT' => '槽道',
		'CT_INTERVAL' => '间隔',
		'CT_OWNER' => '隶属用户',
		'CT_DIR' => '用户目录',
		'CT_TIME_START' => '起始时间',
		'CT_TIME_END' => '结束时间',
		'CT_NORESULT' => '无运行中的任务',
		'CT_STOP'	=> '停止',
		'CT_CONFIRM_DELETE' => '确认删除本任务?',
		'CT_CONFIRM_STOP' => '确认停止本任务?',
		'CT_STATUS'	=> '状态',
		'CT_STATUS_RUNNING'	=> '正在运行',
		'CT_STATUS_OVER'	=> '运行完毕',
		'CT_STATUS_WAIT'	=> '等待运行',
		'CT_DATE_WRONG'		=> '您提交的任务同现有的任务时间上有重合,请重新输入!',

		'SEARCH_TRUNK_TITLE' => '查询TRUNK图表',
		'SEARCH_PVC_TITLE' => '查询PVC图表',
		'SEARCH_IP_TITLE' => '查询IP图表',
		'SEARCH_CPU_TITLE' => '查询CPU图表',
		'SEARCH_MEM_TITLE' => '查询MEM图表',
		'SEARCH_EACHPROV_TITLE' => '查询各省图表',
		'SEARCH_FILENAME' => 'MRTG图表文件',
		'SEARCH_COMMENT' => '附注',
		'SEARCH_NORESULT' => '无任何符合条件的图表!',
		'SEARCH_AVG_DAY'	=> '日平均流量',
		'SEARCH_AVG_WEEK'	=> '周平均流量',
		'SEARCH_AVG_MONTH'	=> '年平均流量',
		'SEARCH_AVG_IN'	=> 'In',
		'SEARCH_AVG_OUT'	=> 'Out',
		'SEARCH_VIEW_RESULT' => '搜寻结果',

		'CUMVIEW_RETURN' => '返回',
		'CUMVIEW_PRINT' => '打印',
		'BUTTON_SUBMIT' => '提交',

		' ' => ' ',
		'END' => ' '
		);
		?>
