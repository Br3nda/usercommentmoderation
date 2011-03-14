<?php
//$Id: user_comment_moderation_ajax_call.inc,v 1.11 2010/06/03 00:41:54 shamlin Exp $

/**
 * This function handles comment approval (review)
 * 
 * @param string $cid the comment ID
 * @return Ajax response message:
 * <code>cid=valid|invalid & status=approve_successful|approve_failed</code>
 */
function ucm_approve_comment($cid, $key) {
  $out = '';
  
  watchdog(
    'UCM - approve', 
    '$cid=%cid',
    array('%cid' => $cid),
    WATCHDOG_NOTICE
  );
  
  // Stop Cross-Site Request Forgery (CSRF)
  if(ucm_keygen($cid, 'approve') == $key ){ 
  
    $flag = flag_get_flag(variable_get('UCM_flag', ''));
    if($flag->is_flagged($cid)) {
      $out = 'cid=valid&status=comment_flagged';
    } else {
      if($flag->flag('flag', $cid)) {
  	    $out = 'cid=valid&status=approve_successful';         
      } else {
  	    $out = 'cid=valid&status=approve_failed';               
      }
    }       
  } else {
    $out = 'cid=invalid_key&status=approve_failed';         
  }
  
  print t($out);
}

/**
 * This function handles comment delete (archive)
 * 
 * @param object $cid  the comment ID
 * @param object $reason_id [optional]
 * @return 
 */
function ucm_delete_comment($cid, $reason_id , $key) {
  // load the comment
  if(_comment_load($cid)) {
    
    // Stop Cross-Site Request Forgery (CSRF) using key
    if( ucm_keygen($cid, 'delete') != $key  ) {
      $out = 'cid=invalid_key&status=delete_failed';  
    } elseif (ucm_process_delete($cid, $reason_id , $key ) ) {
      $out = 'cid=valid&status=delete_successful&reason=' . $reason_id;          
    } else {
      $out = 'cid=valid&status=delete_failed';             
    }  
  } else {
    $out = 'cid=invalid&status=delete_failed';   
  }
  
  print t($out);
}

/**
 * This function responses to ajax /comment/action/lock/$cid
 * When moderator clicks on accordion
 * @param object $cid [optional]
 * @return TRUE / FALSE
 */
function ucm_ajax_call($cid='') {
	global $user;
  
	// Guard statement - if is not valid comment, return FALSE
  $comment = _comment_load($cid);
  if(!$comment) {
    return FALSE;
  }
  
	$lockedItem = ucm_comment_is_locked($cid);
  if (!$lockedItem) {
    // Insert lock to table
    ucm_insert_locked_ssesion($cid);
	  return TRUE;
  } else {  
    if ($lockedItem[locked_datetime] + variable_get('UCM_lock_expiry_time', '120') > time()) {
		  if($lockedItem['locked_by'] == $user->uid) {
        // Renew my locked session
			  ucm_renew_locked_session($cid);
			  return TRUE;	
		  } else {
			  // Return false
			  return FALSE;
		  }

    } else {
      //Return true
		  ucm_remove_locked_ssesion($cid);
		  ucm_insert_locked_ssesion($cid);
		  return TRUE;
    }
  }				
}

/**
 * This function uses Ajax to test the user uid status
 * Ajax response format: key=val&key2=val2
 * update=success|fail&locked=true|false&
 * 
 * @param object $uid
 * @return Ajax response string
 */
function ucm_ajax_lock($cid) {
	global $user;
	// Guard statement - to make user user is not being assigned a comment
	if(!ucm_is_unmoderated_comment($cid)) {
	  $out = 'cid=invalid';
	} else {
	  $out = 'cid=valid';
  	$lockedItem = ucm_comment_is_locked($cid);
  	
  	if (!$lockedItem) {
  	  // Insert lock to table
  	  ucm_insert_locked_ssesion($cid);
  	  $out .= '&status=new_session';
  
  	} else {  
      // check t osee if the lock has expired
      if($lockedItem[locked_datetime] + 
        variable_get('URM_lock_expiry_time', '120') > time()) {
          
  	    if($lockedItem['locked_by'] == $user->uid) {
          // Renew my locked session
  				ucm_renew_locked_session($cid);
          $out .= '&status=renew_session';	
  			} else {
  		    $out .= '&status=session_is_locked';	
  			}
  
      } else {
        // Return true
  			ucm_remove_locked_ssesion($cid);
  			ucm_insert_locked_ssesion($cid);
  		  $out .= '&status=overwrite_new_session';
      }
    }
	}

  print t($out);
}

/**
 * This function uses Ajax to test cid status
 * Ajax response format: key=val&key2=val2
 * update=success|fail&locked=true|false&
 * 
 * @param object $cid
 * @return 
 */
function ucm_ajax_unlock($cid) {
  $out='';

  global $user;
  // check if user is locked
  $commentLocked = ucm_comment_is_locked($cid);
  if (!$commentLocked) {
    $out .= 'status=comment_not_locked'; 
    print t($out);
    return;
  }
  
  // make sure you are the locker
  if($commentLocked['locked_by'] == $user->uid) {
    // remove lock
    ucm_remove_locked_ssesion($cid);
    $out .= 'status=lock_removed';  
  } 
  else {
    // fail
    $out .= 'status=locked_by_another_user';  
  }
  print t($out);
}

/**
 * Check to see if the uid is valid
 * 
 * @param object $uid the user ID
 * @return 
 */
function ucm_is_valid_user($uid) {
  // User's UID must exist
  // exclude uid 0, 1
  $sql = "SELECT *
  	FROM {users} AS u 
  	WHERE u.uid = '%d'
  	AND u.uid > 1";
  $result = db_query($sql, $uid);

  if (count($result) == '1') {
    $out = db_result($result);
  } else {
    $out = 0;
  }

  if ($out == 0) {
    return FALSE;
  } else {
    return $out;
  }
}
/**
 * This function check if user has never been moderated
 * Unmoderated user: user with no comment or comment that setup in 
 * the administration settings
 * 
 * @param object $cid the comment ID
 * @return 
 */
function ucm_is_unmoderated_comment($cid) {
  $sql = "SELECT c.cid 
    FROM {comments} AS c
    WHERE c.status = '0'
    AND c.cid = '%d'";
  $result = db_query($sql, $cid);
  
  if (count($result) == '1') {
    $out = db_result($result);
  } else {
   $out = 0;
  }
  
  if ($out == 0) {
    return FALSE;
  } else {
    return $out;
  }
}

/**
 * This function check is user is locked
 * 
 * @param object $cid the comment ID
 * @return 
 */
function ucm_comment_is_locked($cid) {
	// Return moderator UID who locked the user
	$sql = "SELECT *
		FROM {user_comment_moderation} AS ucm
		WHERE ucm.cid = '%d'
		AND ucm.cid > 1";
		
  $result = db_query($sql, $cid);
  $out = array();
	while ($row = db_fetch_object($result)) {
		$out['cid'] = $row->cid;			
		$out['locked_by'] = $row->locked_by;			
		$out['locked_datetime'] = $row->locked_datetime;			
		$out['status'] = $row->status;
		$out['total']= count($row);			
	}

	if(count($row) == 1) {
		return $out;	
	} else {
		return FALSE;
	}
}

/**
 * This function creates a new lock session
 * @param object $cid [optional]
 * @return 
 */
function ucm_insert_locked_ssesion($cid = '') {
	global $user;
	$now = time();	

  $sql = "INSERT INTO {user_comment_moderation}
    (cid, locked_by, locked_datetime, status)
		VALUES ( %d, %d, $now, 1 )";
  if(db_query($sql, $cid, $user->uid)) {
	  return TRUE;
  } else {
    return FALSE;
  }
}

/**
 * This function renews the lock
 * @param object $cid the comment ID
 * @return success
 */
function ucm_renew_locked_session($cid) {
  global $user;
	$now = time();	

  $sql = "UPDATE {user_comment_moderation}
	  SET locked_datetime = '$now'
	  WHERE cid = '%d' AND locked_by = '%d'";
  if(db_query($sql, $cid, $user->uid)) {
  	return TRUE;
  } else {
  	return FALSE;
  }
}

/**
 * This function unlocks locked session
 * 
 * @param object $cid the comment ID
 * @return success or not
 */
function ucm_remove_locked_ssesion($cid) {
	// Unlocked session locked by someone else
  $sql = "DELETE FROM {user_comment_moderation} WHERE cid='%d'";
  if(db_query($sql, $cid )) {
  	return TRUE;
  } else {
  	return FALSE;
  }
}

/**
 * Special delete function that archives the comment, rather than flat out
 * removing it from the database (core delete)
 * 
 * @param object $cid the comment ID
 * @param object $reason_id
 * @return 
 */
function ucm_process_delete($cid, $reason_id, $key) {
	global $user;
  if( empty($cid) || $reason_id == NULL ) {
    return FALSE;
  } 
  
  $now = time();
  $reason = variable_get('UCM_reason', '');
  $reasons = split("\n", $reason);
  $comment = $reasons[$reason_id];
  
  watchdog(
    'UCM - banned', 
    '$cid=%cid, $comment=%comment',
    array('%cid' => $cid, '%comment' => $comment),
    WATCHDOG_NOTICE
  );
	
  // Before deleting, ensure comment has not been recorded
  // in the {user_comment_moderation_comment} table
  $qry = "SELECT cid FROM {user_comment_moderation_comment} WHERE cid='%d'";
  $ucm_tt_comment = db_query($qry, $cid);     

  if($ucm_tt_comment) {
    $comment_object =  _comment_load($cid);
    $comment_nid = $comment_object->nid;
    $serialised_comment = serialize($comment_object);
    
    $sql = "INSERT INTO {user_comment_moderation_comment}
      (cid, moderator_id, datetime, comment, deleted_comment)
      VALUES ( '%d', '%d', $now, '%s', '%s' )";
      
    if(db_query($sql, $cid, $user->uid, $comment, $serialised_comment)) {
      //Blow away user comment...
      $d = "DELETE FROM {comments} WHERE cid = '%d'";
      $success = db_query($d, $cid);
      if($success) {
        _comment_update_node_statistics($comment_nid);
		    return TRUE;
      } else {
        return FALSE;
      }
    } else {
      return FALSE;
    }      
  } else {
    return FALSE;
  }
}

/**
 * Stores user preferences in the session, for which term they want to see the
 * comments for
 * 
 * @param string $termId the term(s) they want to see comments for
 * @return success string
 */
function ucm_ajax_save_tid($termId) {
  global $user;
	if($termId) {
		$tids = split('_tid-', $termId);
    $SID = $user->uid . "-" . MD5('TID' . $user->uid);
    $_SESSION[$SID] = '';
		if(is_array($tids)) {
      $_SESSION[$SID] = serialize($tids);
		} 
	}
	print t('status=successful');
}