function cTable(){
    //***************************************Class Property ***************************
    //bg color
    this._bg_color          =   '#000000';
    //header text
    this._header_text       =   '';      
    //header color
    this._header_color      =   '#C0C0C0';
    //colspan
    this._colspan           =   10;
    //cellspacing
    this._cellspacing       =   1;
    //cellpadding
    this._cellpadding       =   2;
    //border
    this._border            =   0;
    //border color
    this._border_color      =   '#C0C0C0';
    //width
    this._width             =   '100%';
    //height
    this._height            =   '';
    //align
    this._align             =   'center';
    //valign
    this._valign            =   'top';
    //table css
    this._css               =   'font-family:宋体;font-size:12px';
    //td bgcolor
    this._td_bg_color       =   '#FFFFFF';
    //td css
    this._td_css            =   'font-family:宋体;font-size:12px';
    //td light color
    this._table_light       =   '#BDDFFF';
    //td dark color
    this._table_dark        =   '#E5F3FF';
    //page limit
    this._page_limit        =   10;
    //table string
    this._table_string      =   '';
    //table string Array
    this._table_var         =   new Array();
    //table title
    this._table_title	    = 	new Array();

    //*****************************************Class Method *****************************
    //init class
    this.cTable             =   function(bg_color,width,height,header_text,header_color,colspan,cellspacing,cellpadding,
                                border,bordercolor,css,td_css,table_light,table_dark){
        if(bg_color)this.set_bg_color(bg_color);
        if(width)this.set_width(width);
        if(height)this.set_height(height);
        if(header_text)this.set_header_text(header_text);
        if(header_color)this.set_header_color(header_color);
        if(colspan)this.set_colspan(colspan);
        if(cellspacing)this.set_cellspacing(cellspacing);
        if(cellpadding)this.set_cellpadding(cellpadding);
        if(border)this.set_border(border);
        if(bordercolor)this.set_bordercolor(bordercolor);
        if(css)this.set_css(css);
        if(td_css)this.set_css(td_css);
        if(table_light)this.set_table_light(table_light);
        if(table_dark)this.set_table_dark(table_dark);
    }
    //set bg color
    this.set_bg_color       =   function(color){
        this._bg_color      =   color;
    }
    //set header bg color
    this.set_header_color   =   function(color){
        this._header_color  =   color;
    }
    //set header text
    this.set_header_text    =   function(text){
        this._header_text   =   text;
    }
    //set width
    this.set_width          =   function(width){
        this._width         =   width;
    }
    //set height
    this.set_height         =   function(height){
        this._height        =   height;
    }
    //set colspan
    this.set_colspan        =   function(colspan){
        this._colspan       =   colspan;
    }
    //set cellspacing
    this.set_cellspacing    =   function(cellspacing){
        this._cellspacing   =   cellspacing;
    }
    //set cellpadding
    this.set_cellpadding    =   function(cellpadding){
        this._cellpadding   =   cellpadding;
    }
    //set border
    this.set_border         =   function(border){
        this._border        =   border;
    } 
    //set border color
    this.set_bordercolor    =   function(bordercolor){
        this._bordercolor   =   bordercolor;
    }
    //set table css
    this.set_css            =   function(css){
        this._css           =   css;
    }
    //set td css
    this.set_td_css         =   function(td_css){
        this._td_css        =   td_css;
    }
    //tr start
    this.tr_start           =   function(ret){
	if(ret){return '<TR>\n';}
        this._table_var.push('<TR>\n');
    }
    //tr end
    this.tr_end             =   function(ret){
	if(ret){return '</TR>\n';}
        this._table_var.push('</TR>\n');
    }
    //td
    this.td                 =   function(data,width,bgcolor,css,align,valign,ret){
        var td_property     =   '';
            td_property     +=  (width)?(' width = '+width):'';
            td_property     +=  (bgcolor)?' bgcolor = '+bgcolor:' bgcolor = '+this._td_bg_color;
            td_property     +=  (css) ? ' class = "'+css+'" ':' style = "'+this._td_css+'" ';
            td_property     +=  (align) ? ' align = '+align:' align = '+this._align;
            td_property     +=  (valign) ? ' valign = '+valign:' valign = '+this._valign;
	if(ret){return '<TD '+td_property+' > '+strdecode(data)+" </TD>\n";}
        this._table_var.push('<TD '+td_property+' > '+strdecode(data)+" </TD>\n"); 
    }
    //th
    this.th                 =   function(data,width,css,align,valign,ret){
        var td_property     =   '';
            td_property     +=  (width)?(' width = '+width):'';
            td_property     +=  (css) ? ' class = "'+css+'" ':' style = "'+this._td_css+'" ';
            td_property     +=  (align) ? ' align = '+align:' align = '+this._align;
            td_property     +=  (valign) ? ' valign = '+valign:' valign = '+this._valign;
            td_property     +=  ' bgcolor = '+this._header_color; 
	if(ret){return '<TD '+td_property+' > '+strdecode(data)+" </TD>\n";}
        this._table_var.push('<TD '+td_property+' > '+strdecode(data)+" </TD>\n");
    }
    //set td light  bgcolor
    this.set_table_light    =   function(color){
        this._table_light   =   color;
    }
    //set td dark   bgcolor
    this.set_table_dark     =   function(color){
        this._table_dark    =   color;
    }
    //return light or dark color as id
    this.table_highlight    =   function(id){
        var color           =   ( id % 2)?this._table_light:this._table_dark;
        return color;
    }
    //rows input Array
    this.rows               =   function(data){
        var color           =   '';
        var count           =   0 ;
        var val             =   '';
        var pheader         =   /^\^/;
        for(row in data){
            this.tr_start();
            for(col in data[row]){
                val         =   data[row][col];
                if(pheader.test(val)){
                    val     =   val.replace('^','');
                    this.th('<B>'+val+'</B>');
                }else{
                    color   =   this.table_highlight(count++);
                    this.td(val,'',color);
                }
            }
            this.tr_end();
        }
    }
    //table title 
    this.title              =   function(data,ret){
	    var color           =   '';
	    var count           =   0 ;
	    var val             =   '';
	    var pheader         =   /^\^/;
	    var title_string    =   '' ;
	    title_string   += 	this.tr_start(ret);
	    for(col in data){
		    val         =   data[col];
		    if(pheader.test(val)){
			    val     =   val.replace('^','');
			    title_string  += this.th(val,'','','','',ret);
		    }else{
			    color   =   this.table_highlight(count++);
			    title_string  += this.td(val,'',color,'','','',ret);
		    }
	    }
	    title_string  += this.tr_end(ret);
	    return title_string;
    }
    //draw table
    this.draw               =   function(){
        this._table_string  =   '';
        var table_property  =   '';
            table_property  +=  (this._width)?' width = '+this._width:'';
            table_property  +=  (this._height)?' height = '+this._height:'';
            table_property  +=  (this._colspan)?' colspan = '+this._colspan:'';
            table_property  +=  (this._cellspacing)?' cellspacing = '+this._cellspacing:'';
            table_property  +=  (this._cellpadding)?' cellpadding = '+this._cellpadding:'';
            table_property  +=  (this._bg_color)?' bgcolor = '+this._bg_color:'';    
            table_property  +=  (this._css)?' class = "'+this._css+'"':'';
            table_property  +=  (this._align)?' align = "'+this._align:'';
            table_property  +=  (this._valign)?' valign = "'+this._valign:'';
            table_property  +=  (this._border)?' border = "'+this._border:'';
            table_property  +=  (this._border)?' bordercolor = "'+this._bordercolor:'';
            
        this._table_string  +=  '<TABLE '+table_property+' >\n';
        if(this._header_text){
            this._table_string  +=  '<TR>\n';
            this._table_string  +=  '<TD bgcolor='+this._header_color+' colspan='+this._colspan+'>\n';
            this._table_string  +=  '<center><B>'+this._header_text+'</B></center>';
            this._table_string  +=  '</TD>\n';
            this._table_string  +=  '</TR>\n';
        }
	if(this._table_title){
	    this._table_string  +=  this.title(this._table_title,1);
	}
        for(i in this._table_var){
            this._table_string  +=  this._table_var[i];
        }
        this._table_string  +=  '</TABLE>\n';
        return this._table_string;
    }
}
var Table   =   new cTable();
/*****example**************************************************
var table   =   new cTable();
table.cTable('#0e0000','80%','','Header','#C0C0C0',13,1,2);
var data    =   new Array();
for(i=0;i<100;i++){
    data[i] =   new Array();
    for(j=0;j<10;j++){
        data[i][j]  =   i * j;
    }
}
table.rows(data,10);
table.draw();
*************************************************************/
