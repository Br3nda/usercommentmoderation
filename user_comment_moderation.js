/**
 * Global DOM ready JS
 */
$(document).ready(function(){
  // build the accordion (only if it exists)
  if ($("#accordion").length > 0) {
    $("#accordion").accordion({
      autoHeight: false,
      collapsible: true,
      active: false,
      event: ""
    });
  }
  
  // 60 second auto refresh (only if there is no open accordion panes),
  // if a pane is open, defer refresh for another 60 seconds
  setTimeout('doRefresh()', 60000);
  
  // change document title to correspond to the number of items on the TODO list
  document.title = '(' + countPanes() + ') Comment moderation';
  
  // Binding show/hide search filter button
  $('#filter-form, #hide-filter').hide();
  
  $('#show-filter').bind('click',function(){
  	$('#filter-form').show('slow');  
  	$('#show-filter').hide('slow'); 
  	$('#hide-filter').show('slow'); 	
  });
  
  $('#hide-filter').bind('click',function(){
		$('#filter-form').hide('slow'); 
		$('#show-filter').show('slow'); 
		$('#hide-filter').hide('slow'); 		
	});  
  
  $('#filter-button').bind('click',function(){
	  var ajax_string="";
	  $('#filter-form input:checkbox:checked').each(function(index) {
		  ajax_string += "_tid-" + $(this).val();
	  });
	  saveTid(ajax_string);
	  return false;
  });

  // Bind function to delete buttons
  $('form.delete_forms input:submit:[value^=Confirm delete]').bind('click', function(){
    var this_id = $(this).attr("id");
    var cid = this_id.split("-").pop();
    return confirmDeleteComment(cid);
  });
  
  // Bind function to cancel buttons
  $('form.ban_forms input:submit:[value^=Cancel]').bind('click', function(){
    $.fn.colorbox.close();
    var this_id = $(this).attr("id");
    var cid = this_id.split("-").pop();      
    enableButtons(cid);
    return false;
  });

  // Unlock the previous user  
  $('#accordion').bind('accordionchange', function(event, ui) {
    if ($(ui.oldHeader).length > 0 && $(ui.newHeader).length > 0) {
      var accordionId = $(ui.oldHeader).attr("id");
      //var accordionIndex = $(ui.oldHeader + "h3").index(this);
      var cid = accordionId.split("-").pop();
      unLockRow(accordionId, cid, -1); 
    }
  });
  
  // Before you can open an accordion pane, you
  // need to check whether the accordion panel is
  // locked by another user
  $("#accordion h3").click(function(){
    var accordionId = $(this).attr("id");
    var accordionIndex = $("#accordion h3").index(this);
    var cid = accordionId.split("-").pop();
 
    // add spinner class to accordion row to show AJAX running
    $("#" + accordionId).addClass("ui-state-ajax");

    // if row is active, then it is expanded and needs to be
    // unlocked and collapsed
    if ($(this).hasClass("ui-state-active")) {
       //alert(accordionId + cid + accordionIndex);
      unLockRow(accordionId, cid, accordionIndex);
    }
    // if row is not active, then it is collapsed and needs to be
    // locked and opened
    else {
      lockRow(accordionId, cid, accordionIndex);
    }
  });
  
  // Approve user, no confirmation is required
  // ID of <a> = approve_user-[cid]
  $("#accordion .approve").click(function(){
    var cid = $(this).attr("id").split("-").pop();
    var key = $(this).attr('href').split("/").pop();

    var accordionId = "comment_cid-" + cid;

    // add spinner class to accordion row to show AJAX running
    $("#" + accordionId).addClass("ui-state-ajax");
    
    // check if this button has already been clicked,
    // if so, do not send another request
    if ($(this).hasClass("ui-state-disabled")) {return false;}
    
    // This send AJAX to approve user
    approveComment(cid, key);
    
    // disable buttons to stop user getting click happy
    disableButtons(cid);

    return false;
  });

  // Bind button to pop up delete confirmation screen
  $("#accordion .delete").click(function(){
    var cid = $(this).attr("id").split("-").pop();
    var accordionId = "comment_cid-" + cid;
    
    // add spinner class to accordion row to show AJAX running
    $("#" + accordionId).addClass("ui-state-ajax");
    
    // check if this button has already been clicked,
    // if so, do not send another request
    if ($(this).hasClass("ui-state-disabled")) {return false;}
    
    // This pop up confirmation form with reason for moderator to select
    deleteComment(cid);
    
    // disable buttons to stop user getting click happy
    disableButtons(cid);
    
    return false;
  });
});

/**
 * 60 second auto refresh (only if there is no open accordion panes),
 * if a pane is open, defer refresh for another 60 seconds
 */
function doRefresh() {
  // there is a pane open, reset the timer
  if (countOpenPanes() > 0) {
    // reset the timer
    setTimeout('doRefresh()', 60000);
  }
  // there is no pane open, reload the page
  else {
    document.location.reload();
  }
}

/**
 * Helper function to count the number of open panes
 */
function countOpenPanes() {
  // check if a accordion pane is open
  var panesOpen = 0;
  $("#accordion .ui-accordion-content").each(function(){
    if ($(this).css("display") == "block") {
      panesOpen++;
    }
  });
  return panesOpen;
}

/**
 * Helper function to count the number of panes
 */
function countPanes() {
  // check if a accordion pane is open
  var panesOpen = 0;
  $("#accordion .ui-accordion-content").each(function(){
    panesOpen++;
  });
  return panesOpen;
}

/**
 * Call the back end to attempt to create the lock
 * 
 * @param {Object} cid the user id you are trying to lock
 */
function lockRow (accordionId, cid, rowId) {
  // fire AJAX
  $.ajax({
    url: "/admin/moderation/comment/action/lock/" + cid,
    type: "GET",
    dataType: "text",
    timeout: 10000,
    success: function (data, textStatus, XMLHttpRequest) {
      // remove spinner class on accordion row to show AJAX running
      $("#" + accordionId).removeClass("ui-state-ajax");
      
      // was user valid?
      var cidValid = data.split("&")[0].split("=").pop();
      if (cidValid != "valid") {
        $("#" + accordionId).addClass("ui-state-disabled");
        $("#" + accordionId + " .error-reason").html("cid was not valid !");
        return false;
      }
      // was locking successful?
      var lockingStatus = data.split("&")[1].split("=").pop();
      if (lockingStatus == "session_is_locked") {
        $("#" + accordionId).addClass("ui-state-disabled");
        $("#" + accordionId + " .error-reason").html("session is locked!");
        return false;
      }
      $("#" + accordionId).removeClass("ui-state-disabled");
      $("#" + accordionId + " .error-reason").html("");
      if (rowId >= 0) {
        $("#accordion").accordion("activate", rowId);
      }
    },
    error: function(XMLHttpRequest, textStatus, errorThrown) {
      // remove spinner class on accordion row to show AJAX running
      $("#" + accordionId).removeClass("ui-state-ajax");
      
      alert ("AJAX error");
      return false;
    }
  });
}

/**
 * Call the back end to attempt to remove the lock
 * 
 * @param {Object} accordionId the CSS ID for the accordion
 * @param {Object} cid the comment ID
 * @param {Object} rowId the row ID for the accordion
 */
function unLockRow (accordionId, cid, rowId) {
  // fire AJAX
  $.ajax({
    url: "/admin/moderation/comment/action/unlock/" + cid,
    type: "GET",
    dataType: "text",
    timeout: 10000,
    success: function (data, textStatus, XMLHttpRequest) {
      // remove spinner class on accordion row to show AJAX running
      $("#" + accordionId).removeClass("ui-state-ajax");
      
      // was unlocking successful?
      var unLockingStatus = data.split("&")[0].split("=").pop();
      if (unLockingStatus == "lock_removed") {
        $("#" + accordionId).removeClass("ui-state-disabled");
        $("#" + accordionId + " .error-reason").html("");
        // collapse row if needed
        if (rowId >= 0) {
          $("#accordion").accordion("activate", rowId);
        }
      }
      // lock was not removed, display error
      else {
        $("#" + accordionId).addClass("ui-state-disabled");
        $("#" + accordionId + " .error-reason").html("error - " + unLockingStatus);
      }
    },
    error: function(XMLHttpRequest, textStatus, errorThrown) {
      // remove spinner class on accordion row to show AJAX running
      $("#" + accordionId).removeClass("ui-state-ajax");
      
      alert ("AJAX error");
      return false;
    }
  });
}

/**
 * Add disabled classes to both buttons for this user
 * 
 * @param {Object} cid
 */
function disableButtons(cid){
  $("#delete_comment-" + cid + ", #approve_comment-" + cid)
    .addClass("ui-state-disabled"); 
}

/**
 * Remove disabled classes to both buttons for this user
 * 
 * @param {Object} cid
 */
function enableButtons(cid){
  $("#disapprove_comment-" + cid + ", #approve_comment-" + cid)
    .removeClass("ui-state-disabled"); 
}

/**
 * Approve the comment
 * 
 * @param {Object} cid the comment ID
 */
function approveComment(cid, key) {
   // Fire Ajax
   // If successful - return true, remove current row
   // If failed - display errro message, enable buttons
	  $.ajax({
    url: "/admin/moderation/comment/action/approve/" + cid + "/" + key,
    type: "GET",
    dataType: "text",
    timeout: 10000,
    success: function (data, textStatus, XMLHttpRequest) {      
      // was callback successful?
      var callbackCid = data.split("&")[0].split("=").pop();
      var callbackStatus = data.split("&")[1].split("=").pop();
      
      //Expected data cid=valid&status=approve_successful
      if (callbackCid == "valid" && callbackStatus=="approve_successful" ) {
        //alert("well done");
        $("#comment_cid-" + cid).hide('slow');
        $("#comment_cid-" + cid).next().hide('slow');
        location.reload();
      } else if(callbackCid == "invalid_key" ){
          // If key is not matched
      	  alert("Ajax error - #Invalid key , page will be refreshed automatically!");
          location.reload();
      }
      else {
          // lock was not removed, display error
      	  alert("Ajax error - #Locked by another moderator, page will be refreshed automatically!");
          location.reload();
      }
    },
    error: function(XMLHttpRequest, textStatus, errorThrown) {      
  	  alert("Ajax error - #Ajax timeout, page will be refreshed automatically!");
      location.reload();
    }
  });
  
  return false;
}

/**
 * This function displays ban_form
 * 
 * @param {Object} cid
 */
function deleteComment(cid) {
  var delete_form = "#delete_form-" + cid;
  $.fn.colorbox({ width: "40%", inline:true, href: delete_form });
  return false;
}

/**
 * Display the colorbox for the confirmation delete
 * 
 * @param {Object} cid the comment ID
 */
function confirmDeleteComment(cid){
  var delete_form = "#delete_form-" + cid;
  var tt =  $(delete_form + " input:radio:checked").length;
  
  // check to see if an option was selected
  if(tt > 0) {
    var reason_id = $(delete_form + " input:radio:checked").val();
    var key = $(delete_form + " .key").val();
  
    var ajax_url = "/admin/moderation/comment/action/delete/" + cid + "/" + reason_id + "/" + key;

    $.ajax({
      url: ajax_url,
      type: "GET",
      dataType: "text",
      timeout: 10000,
      success: function (data, textStatus, XMLHttpRequest) {
        // was callback successful?
        var callbackCid = data.split("&")[0].split("=").pop();
        var callbackStatus = data.split("&")[1].split("=").pop();
        
        //Expected data cid=valid&status=delete_successful
        if (callbackCid == "valid" && callbackStatus=="delete_successful" ) {
          $("#comment_cid-" + cid).css('display: none;');
          $("#comment_cid-" + cid).next().css('display: none;');
          location.reload();
        } else if(callbackCid == "invalid_key" ){
          // If key is not matched
      	  alert("Ajax error - #Invalid key , page will be refreshed automatically!");
          //location.reload();
        }
        // lock was not removed, display error 
        else {
        	alert("Ajax error - #Locked by another moderator, page will be refreshed automatically!");
          //location.reload();
        }
      },
      error: function(XMLHttpRequest, textStatus, errorThrown) {
        alert("Ajax error - #Timeout, page will be refreshed automatically!");
        //location.reload();
      }
    });
  } else {
    alert("Please select a reason.");
    return false;  
  }
  return false;
}

/**
 * Save the term ID's in the session
 * 
 * @param {Object} str
 */
function saveTid(str) {
  // Fire Ajax
  // If successful - return true, remove current row
  // If failed - display errro message, enable buttons
	$.ajax({
    url: "/admin/moderation/comment/action/save-tid/" + str,
    type: "GET",
    dataType: "text",
    timeout: 10000,
    success: function (data, textStatus, XMLHttpRequest) {
      // was callback successful?
      var callbackStatus = data.split("&")[0].split("=").pop();

      //Expected data cid=valid&status=approve_successful
      if (callbackStatus == "successful" ) {
        location.reload();
      }
      // lock was not removed, display error
      else {
      	alert("Ajax error, please refresh the page try again!");
      }
    },
    error: function(XMLHttpRequest, textStatus, errorThrown) {
  	  alert("Ajax error, please refresh the page try again!");
      location.reload();
    }
  });
  
	return false;
}