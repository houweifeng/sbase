<?php

/* 
 *
 */
include_once('./lang/zh_CN.lang.php');
include_once('./conf/permissions.conf.php');
include_once('./conf/base.conf.php');
include_once('./include/config.inc.php');
include_once('./include/functions.inc.php');
include_once('./include/cinterface.inc.php');
include_once('./include/cdatadb.inc.php');
include_once('./include/cstoredb.inc.php');
include_once('./include/cdatabase.inc.php');
include_once('./include/ctable.inc.php');
include_once('./include/cform.inc.php');
include_once('./include/cuser.inc.php');
$db	= new cDatabase(DB_HOST, DB_USERNAME, DB_PASSWORD, DB_TYPE, DB_NAME,1);
$dbdata	= new cDatabase(DATA_DB_HOST, DATA_DB_USERNAME, DATA_DB_PASSWORD, 
    DATA_DB_TYPE, DATA_DB_NAME,1);
$dbstore	= new cDatabase(STORE_DB_HOST, STORE_DB_USERNAME, STORE_DB_PASSWORD, 
    STORE_DB_TYPE, STORE_DB_NAME, 1);
session_start();
foreach($_SESSION as $key => $val) {
  $$key=$val;
}
$PHP_SELF = $_SERVER['PHP_SELF'];
?>
