function cPageSplit(){
    this.data           =   new Array();
    this.DataSplit      =   new Array();
    this.CurrentPage    =   0;
    this.PageTotal      =   0;
    this.PageLimit      =   10;
    this.PageViewLimit  =   10;
    this.cPageSplit     =   function(data,PageLimit,PageViewLimit){
        if(data){this.data  =   data;}
        if(PageLimit){this.PageLimit    =   PageLimit;}
        if(PageViewLimit){this.PageViewLimit    =   PageViewLimit ;}
    }
    //set Page Limit
    this.SetPageLimit   =   function(PageLimit){
        if(PageLimit){this.PageLimit    =   PageLimit;}
    }
    //set Page View Limit
    this.SetPageViewLimit   =   function(PageViewLimit){
        if(PageViewLimit){this.PageViewLimit    =   PageViewLimit;}
    }
    //data Array
    this.DataAdd        =   function(data){
        this.data.push(data);
        this.PageSplit();
    }
    //index 
    this.DataDrop       =   function(index){
        this.data[index] =   false;      
        this.PageSplit();
    }
    //reset
    this.DataReset      =   function(){
        var data        =   this.data;
        this.data       =   new Array();
        var n = 2;
        var val ;
        for(index in data){
            val             =   data[index]; 
            if(!val)            {continue;}
            if(!this.data[1]){
                this.data[1]    =   val;
            }else{
                this.data.push(val);
            }
        }
    }
    //Page Split
    this.PageSplit      =   function(){
        this.DataReset();
        this.DataSplit  =   new Array();
        var Total       =   0;
        var pageid;
        for(i in this.data){
            pageid      =    Math.ceil( i / this.PageLimit);
            if(typeof(this.DataSplit[pageid]) == 'undefined' ){
                this.DataSplit[pageid]   =   new Array();
                //alert(pageid);
            }
            this.DataSplit[pageid].push(this.data[i]); 
            Total++;
        }
        this.PageTotal  =   Math.ceil(Total / this.PageLimit);
        
    }
    //data CurrentPage
    this.DataCurrent    =   function(CurrentPage){
        this.PageSplit();
        this.CurrentPage    =   CurrentPage;
        return this.DataSplit[CurrentPage];
    }
    //Page Split draw
    this.PageSplitDraw  =   function(CurrentPage,fun){
        if(!CurrentPage || !fun){
            return '';
        }
	this.PageSplit();
        var PageSplitString =   '';
        var PageViewTotal   =   Math.ceil(this.PageTotal / this.PageViewLimit);
        var CurrentView     =   Math.ceil(CurrentPage  / this.PageViewLimit);
        var PageStart,PageEnd;
        //Start Page
        PageStart   =   ( CurrentView - 1 ) * this.PageViewLimit + 1;
        //End Page
        if(CurrentView == PageViewTotal){
            PageEnd =   this.PageTotal;
        }else{
            PageEnd =   CurrentView  * this.PageViewLimit;
        }
        //show first page view
        if(CurrentView != 1){
            PageSplitString +=  '<a href="javascript:'+fun+'(1);" >';
            PageSplitString +=  '<font face=webdings >9</font></a>&nbsp;&nbsp;&nbsp;';
        }
        //pre page
        if(CurrentPage  !=  1){
            var prev =   CurrentPage -   1;
            PageSplitString +=  '<a href="javascript:'+fun+'('+prev+');" >';
            PageSplitString +=  '<font face=webdings >3</font></a>&nbsp;&nbsp;';
        }
        for(i=PageStart;i<=PageEnd;i++){
            if(CurrentPage == i){
                PageSplitString +=  i+'&nbsp;&nbsp;';
            }else{
                PageSplitString +=  '<a href="javascript:'+fun+'('+i+');">'+i+'</a>&nbsp;&nbsp;';
            }
        }
        //next page
        if(CurrentPage  !=  this.PageTotal){
            var next    =   CurrentPage +   1;
             PageSplitString +=  '<a href="javascript:'+fun+'('+next+');" >';
             PageSplitString +=  '<font face=webdings >4</font></a>&nbsp;&nbsp&nbsp;';
        }
        //show last page view
        if(CurrentView != PageViewTotal){
            PageSplitString +=  '<a href="javascript:'+fun+'('+this.PageTotal+');" >';
            PageSplitString +=  '<font face=webdings >:</font></a>';
        }
        return PageSplitString;
    }
    
}
var PageSplit   =   new cPageSplit();
