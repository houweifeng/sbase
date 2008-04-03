var ContentOutID	=	'content';
var PageSplitOutID	=	'PageSplit';
var PageLimit		=	10;
var PageViewLimit	=	10;
var ColsLimit		=	10;
function DataShow(PageNum,HTMLID,PAGEID,_PageLimit,_PageViewLimit,_ColsLimit){
    var string  	=   	'';
    	PageLimit 	=	(_PageLimit)?_PageLimit:PageLimit;
    	PageViewLimit	=	(_PageViewLimit)?_PageViewLimit:PageViewLimit;
    	ColsLimit	=	(_ColsLimit)?_ColsLimit:ColsLimit;
    	ContentOutID	= 	(HTMLID)?HTMLID:ContentOutID;
    	PageSplitOutID	= 	(PAGEID)?PAGEID:PageSplitOutID;
    	PageSplit.cPageSplit('',PageLimit,PageViewLimit);
    var PageSplitString =   	PageSplit.PageSplitDraw(PageNum,'DataShow');
    var data    	=   	PageSplit.DataCurrent(PageNum);
    Table._table_var 	=   	new Array();
    Table.rows(data,ColsLimit);
    string      +=   Table.draw();
    alert(string);
    if(document.getElementById(ContentOutID)){
    	document.getElementById(ContentOutID).innerHTML	=   string;
    }
    if(document.getElementById(PageSplitOutID)){
    	document.getElementById(PageSplitOutID).innerHTML	=   PageSplitString;
    }
}
function DataInit(DataArray,HTMLID,PAGEID,PageLimit,PageViewLimit,ColsLimit){
    if(DataArray['title']){
	Table._table_title	= DataArray['title'].split(',');
    }
    var RowArray	= new Array();
    for( x in DataArray){
	if(x != 'title'){
	   RowArray = DataArray[x].split(",");
	   PageSplit.DataAdd(RowArray);
	}
    }
    DataShow(1,HTMLID,PAGEID,PageLimit,PageViewLimit,ColsLimit);
}
