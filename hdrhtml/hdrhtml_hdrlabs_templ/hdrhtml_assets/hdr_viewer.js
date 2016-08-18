/*
-----------------------------------------------
HDR HTML Viewer ver. 1.7
Author: v1.0 Rafal Mantiuk
URL:    http://www.cs.ubc.ca/~mantiuk/hdrhtml.html

Template v1.7 by Christian Bloch, www.hdrlabs.com
Requires: mootools javascript library from www.mootools.net or linked in from google's server:
               http://ajax.googleapis.com/ajax/libs/mootools/1.2.4/mootools-yui-compressed.js
	      Slider and Asset extensions from the mootools (More) section
----------------------------------------------- */

		// For suppressing the text-selection cursor when dragging or interacting with the viewer
		
		var noselect = false;
		
		window.addEvent('selectstart', function(e) {
		  if (noselect) {
		  	e.stop();
		    return false;
		  }
		});
	
	  //  for updating the EV label
	
      function update_exp_text( hdr_image, new_exp ) {
      			ev_label = "EV:<span class='labelnumber'>"+ Math.round( (hdr_image.best_exp-new_exp)*10 )/10 + " f</span>";
      
   				//  delete the next two lines if you prefer plain decimal display for fractional EV values
      			ev_label = ev_label.replace(/\.3/, "<font size=\"-1\"><sup> 1</sup>/<sub>3</sub></font>");
      			ev_label = ev_label.replace(/\.7/, "<font size=\"-1\"><sup> 2</sup>/<sub>3</sub></font>");

                $(hdr_image.base_name + "_exp_text").innerHTML =  ev_label;
      }            
            
      function change_exp( hdr_image, exp_change ) {
            
            hdr_active_image = hdr_image;
            hdr_image.exposure = hdr_image.exposure + exp_change;
            
      //  clamp to exposure limits      
      
            if( hdr_image.exposure > hdr_image.f8_stops*8 ) {
              hdr_image.exposure = hdr_image.f8_stops*8;  
            } 
            else if( hdr_image.exposure < 0 ) {
              hdr_image.exposure = 0;
            }
            
            exp_cur = Math.floor(hdr_image.exposure / 8);
            exp_shar = exp_cur + 1;
                        
            exp_blend = Math.round((hdr_image.exposure - exp_cur*8)*hdr_image.f_step_res);
       
      // blend each image accordingly 
            
            var i;
            for( i = 0; i < hdr_image.basis; i++ ) {
            	var img_obj = $(hdr_image.base_name+"_"+i);
            	img_obj.set('src', hdr_image.image_dir + hdr_image.base_name + "_"+exp_cur+"_" + (i+1) + ".jpg");
            	img_obj.setStyle('opacity', cf[exp_blend][i]);
            }
            
            for( i = 0; i < hdr_image.shared_basis; i++ ) {
            	var img_obj = $(hdr_image.base_name+"_"+(i+hdr_image.basis));  
            	img_obj.set('src',hdr_image.image_dir + hdr_image.base_name + "_"+exp_shar+"_" + (i+1) + ".jpg");
            	img_obj.setStyle( 'opacity', cf[exp_blend][i+hdr_image.basis] );
            }
                              
            update_exp_text( hdr_image, hdr_image.exposure );
      }    

      
      //   keyboartd events    
	    function hdr_onkeydown(e) { 
              var hdr_image = hdr_active_image;

              var keynum;
              if( hdr_image == null )
                  return;
              
              var hdrnum = hdr_names.indexOf(hdr_image.base_name);
                
              if(window.event) { // IE
                keynum = window.event.keyCode;
                if( keynum == 189 )  
                	hdr_slider[hdrnum].set(hdr_image.exposure*10 - 10);
                if( keynum == 187 )  
                	hdr_slider[hdrnum].set(hdr_image.exposure*10 + 10);                                
              } else if(e.which) { // Netscape/Firefox/Opera                
                keynum = e.which;
                if( keynum == 109 )
                	hdr_slider[hdrnum].set(hdr_image.exposure*10 - 10);
                if( keynum == 61 )
                	hdr_slider[hdrnum].set(hdr_image.exposure*10 + 10);                                
              }

       }


      // ----------------------------------------    
      // This must be called from within the HTML    
      // ----------------------------------------    
      //      - injects images into viewport
      //      - binds slider to EV
      //      - binds mousewheel to slider
      // Called once from each "hdr_viewer" div

      
      // inject images into HTML                    
            
      function insert_hdr_image( hdr_image ) {
            
            hdr_viewport = $(hdr_image.base_name+'_viewport');
            hdr_viewer = hdr_viewport.getParent('div.hdr_viewer');

            //  preload images
            
            var preloading = new Array(); 
            var k = 0;
            for( i = 0; i < hdr_image.f8_stops; i++ ) {
            	for( j = 0; j < hdr_image.basis; j++ ) {
        			preloading[k] = ""+hdr_image.image_dir + hdr_image.base_name + "_"+i+"_" + (j+1) + ".jpg";
        			k = k + 1;
        		}
            }
            preloading[k] = ""+hdr_image.image_dir + hdr_image.base_name + "_"+hdr_image.f8_stops+"_1.jpg";
            
            //  for showing the AJAX spinning circle swap the next line with the block below
            new Asset.images(preloading);
            
        //    var loadspinner = new Spinner(hdr_viewer).show('noFX');
        //    var myImages = new Asset.images(preloading, {
    	//		onComplete: function(){
        //			loadspinner.destroy();
    	//		}
	    //	});
           
            //  enumerate all instances to remember the name
            hdrcount = hdrcount + 1;
            hdr_names[hdrcount] = ""+hdr_image.base_name;
            
            var injection = "";
            for( i = 0; i < hdr_image.basis + hdr_image.shared_basis; i++ ) {
            injection += "<img id=\"" + hdr_image.base_name + "_" + i +
                                 "\" style=\"margin: 0px; padding: 0px; opacity: 1; width: " +
                                 hdr_image.width + "px; height: " + hdr_image.height + 
                                 "px; position: absolute; top: 0px; left: 0px\"" + 
                                 " />\n";
            }
            
            // also inject a hidden help viewer
                        
            injection += "<div id=\"" + hdr_image.base_name + "_help_view" + "\" class=\"hdr_instructions\"" +
            			      "style=\"margin: 0px; padding: 0px; opacity: 0; width: " +
                                 hdr_image.width + "px; height: " + hdr_image.height + 
                                 "px; position: absolute; top: 0px; left: 0px\" ><div style=\"padding: 50px;\">" + 
                                 "<h1><a href=\"http://pfstools.sourceforge.net/hdrhtml/\" target=\"_blank\">HDR HTML Viewer</a></h1>" +
                                 "v1.0 (C) 2008 Rafal Mantiuk | <a href=\"http://pfstools.sourceforge.net/\" target=\"_blank\">pfsTools</a><br />" + 
                                 "v1.7 mod 2010 Christian Bloch | <a href=\"www.hdrlabs.com\" target=\"_blank\">HDR Labs</a><br />" +
                                 "<ul><li>Change exposure by sliding the dynamic range window left and right. Or just use the scroll wheel.</li>" +
                                 "<li>Press \"-\" and \"=\" keys to change exposure by 1 f-stop.</li>" +
                                 "<li>The green plot represents the HDR histogram. EV value is given in f-stops (log<sub>2</sub> units) " +
                                 "relative to the inital exposure.</li>" +
                                 "<li>The exposure window / slider encompasses 8 f-stops.</li>" +
                                 "</ul></div></div>";	

            
            hdr_viewport.set('html',injection);
			

			// link help viewer to help buttom

            var help_view = $(hdr_image.base_name+'_help_view').fade('hide');;

			$(hdr_image.base_name+'_help').addEvent('click', function() { 
			    if(help_view.getStyle('opacity') == 0) 	help_view.fade(0.85);
			    else help_view.fade('out');
			});

			help_view.addEvent('click', function() { 
				help_view.fade('out');
			});
    
	                                 
      		// initialize mootools slider
           
            hist_left = Math.round((-hdr_image.hist_start)*hdr_image.pix_per_fstop);
           
            var el = $(hdr_image.base_name+'_dr_ctrl');
           
            el.style.left = Math.round(hist_left) + "px";
            el.style.width = Math.round(hdr_image.hist_width-hist_left) + "px";	
            el.getElement('.knob').style.width = hdr_image.pix_per_fstop*8 + "px"; 
        	
        	var EVrange = (hdr_image.hist_width-hist_left)/hdr_image.pix_per_fstop-7.7;
 
            var slider = new Slider(el, el.getElement('.knob'), {
         		steps: EVrange*hdr_image.f_step_res,
        		range: [0,EVrange*10],
         		onChange: function(value) {
        			change_exp( hdr_image, value/10 - hdr_image.exposure);
        		}
         	}).set(hdr_image.exposure*10);
            
            
            // clone this slider into an array for keyboard access
            hdr_slider[hdrcount] = slider;   
            
            // link active hdr image to rollover on the current image
	        hdr_viewer.addEvent('mouseenter', function() {
            	hdr_active_image = hdr_image;
            	// suppress accidental selection of individual images or text
            	noselect = true;
            });

			// link the mousewheel to the slider
	        hdr_viewer.addEvent('mousewheel', function(e) {
	        	e.stop(); // prevent the mousewheel from scrolling the page.
		        noselect = true;
		        slider.set((hdr_image.exposure + e.wheel/2)*10);                                
	        });
	        
	        // restore outside text to be selectable
	        hdr_viewer.addEvent('mouseleave', function() {
				noselect = false;
			});

     }           
