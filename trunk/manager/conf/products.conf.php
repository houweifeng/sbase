<?php
define('USAGEROOT','/var/tmp/webalizer/data');
define('BADRESP', '/var/tmp/webalizer/data/badresp');
$products	= Array(
	'10000'	=> Array(
		'name'	=> 'www.alibaba.com',
		'path'	=> USAGEROOT.'/www'	
	)
);

$view_list	= Array(
	'day'	=> '日报',
	'hist'	=> '累计'
);

define('DEFAULT_PRODUCT', 10000);
define('DEFAULT_VIEW', 'day');

?>
