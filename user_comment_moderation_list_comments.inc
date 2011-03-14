<?php
//$Id: user_comment_moderation_list_comments.inc,v 1.13 2010/06/03 00:41:54 shamlin Exp $

/**
 * Create comment moderation list
 * @return 
 */
function ucm_list_comments(){
  global $user;
  // add js + css
  jquery_ui_add(array(
    'ui.core', 
    'effects.core', 
    'ui.accordion'
  ));
  drupal_add_css(drupal_get_path('module', 'jquery_ui') . 
    '/jquery.ui/themes/' . variable_get('UCM_theme', 'base') . '/ui.all.css');
    
  drupal_add_css(drupal_get_path('module', 'user_comment_moderation') . 
    '/user_comment_moderation.css');
  drupal_add_js(drupal_get_path('module', 'user_comment_moderation') . 
    '/user_comment_moderation.js'); 
    
   // Initialise (CSRF) session
   if(!isset($_SESSION['CSRF'])) {  
     $_SESSION['CSRF'] = MD5(time()); 
   }
  
  // Use this to generates comment filter list - $form_fields
  // Vocabs which used in forum
  $ucm_vocabs = variable_get('UCM_vocabs','');
  $vocab_names = taxonomy_get_vocabularies();
  
  $f = variable_get('UCM_flag', '');
  $o = flag_get_flag($f);
  
  if(is_array($ucm_vocabs)) {
    foreach($ucm_vocabs as $key =>$val){
        $ucm_taxonomy = taxonomy_get_tree($val);
        $vocab_name = $vocab_names[$key]->name;
        
        if(is_array($ucm_taxonomy)){
          foreach($ucm_taxonomy as $key2 => $val2){
              if($ucm_taxonomy[$key2]->name){
                 //Term ID - {term_data}.tid 
              	 $tid = $ucm_taxonomy[$key2]->tid;
                 $filter_fields['vocab_name'][$val] = $vocab_name;
                 $filter_fields[$val][$tid] = $ucm_taxonomy[$key2]->name;    
              }
           }          
        }
    }  
  }
  
  $filter_form = '<a id="show-filter" href=#>More options</a>';
  $filter_form .= '<a id="hide-filter" href=#>Hide options</a>';
  $filter_form .= '<div style="clear: both;"></div>';
  $filter_form .= '<div id="filter-form"> ' . drupal_get_form('ucm_list_taxonomy_form' , $filter_fields) . '</div>';
  $filter_form .= '<div style="clear: both;"></div>';

  $sql_filter = '';
  $list= '';
  
  //Query comments which have never been reviewed
  $sql = 'SELECT 
          c.cid,
          c.nid,
          c.uid,
          c.subject,
          c.comment,
          c.timestamp,
          c.status,
          fc.fcid
          FROM {comments} c
          LEFT JOIN {flag_content} fc
          ON c.cid=fc.content_id
          WHERE c.status=0
          AND fc.fcid IS NULL
          AND c.uid NOT IN (0,1)
          ORDER BY c.timestamp ASC
          LIMIT 0, ' . variable_get('UCM_list_max', 20);
          
  //Get filter list session ID
  $SID = $user->uid."-".MD5("TID".$user->uid);
  
  if(isset($_SESSION[$SID])){
    $TID =  $_SESSION[$SID]; 
    if($TID){
     $tids = unserialize($TID);
  	  $sql = "SELECT 
  	          c.cid,
  	          c.nid,
  	          c.uid,
  	          c.subject,
  	          c.comment,
  	          c.timestamp,
  	          c.status,
  	          fc.fcid
  	          FROM {comments} c
  	          LEFT JOIN {term_node} tn
  	          ON c.nid = tn.nid
  	          LEFT JOIN {flag_content} fc
  	          ON c.cid=fc.content_id
  	          WHERE c.status = 0
  	          AND fc.fcid IS NULL
  	          AND c.uid NOT IN (0,1)
  	          AND tn.tid IN ( " . db_placeholders($tids,'int') . " )
  	          ORDER BY c.timestamp ASC
  	          LIMIT 0, " . variable_get('UCM_list_max', 20); 
    }    
  }
  
  $result = db_query($sql);
  
  while ($comment = db_fetch_object($result)) {
    $uid = $comment->uid;
    $nid = $comment->nid;
    $this_user = user_load($uid);
    $uname = check_plain($this_user->name);
    $cid = $comment->cid;
    $uid = $comment->uid;
    $created = $comment->timestamp;

    $flag = flag_get_flag(variable_get('UCM_flag', ''));
    if(!$flag->is_flagged($cid)){
        $list .= 
          '<h3 id="commemt_cid-' .$cid.'">' .
            '<span class="username">' .$uname.'</span>' .
            '<span class="error-reason"></span>' .
            '<span class="ajax-spinner"></span>' .
            '<span class="date">' . ucm_ago($created) . '</span>' .
          '</h3>';
          
	    $list .= '<div>';
  		$c = _comment_load($cid);
  		$n = node_load($c->nid);
      $list .= '<h5 class="comment-title">' . check_plain($n->title) . '</h5>';
      $list .= $comment->comment;
        
      $list .= 
        '<ul class="comment-button">' .
          '<li>' .
          l('Edit comment', 'comment/edit/' . $cid, array(
              attributes => array (
                'id' => 'cid_edit_' . $cid,
                'class' => 'colorbox-form ui-state-default ui-corner-all',
                'title' => 'Edit the comment',
                )
              )
            ) .
          '</li>' .
          '<li>' .
          l('Reply', 'comment/reply/' . $nid . '/' . $cid, array(
              attributes => array (
                'id' => 'cid_reply_' . $cid,
                'class' => 'colorbox-form ui-state-default ui-corner-all',
                'title' => 'Reply to the comment',
                )
              )
            ) .
          '</li>' .
        '</ul>';
                 
      $list .= '</li>';
      
      $list .= 
        '<p>' .
          l('<span class="ui-icon ui-icon-circle-close"></span>' .
            '<span class="text">Delete comment</span>' .
            '<span class="ui-icon ui-icon-right ui-icon-triangle-1-s"></span>', 
            'colorbox/form/user_role_moderation_ban_user_form', 
            array(
              attributes => array (
                'id' => 'delete_comment-' . $cid,
                'class' => 'delete colorbox-form ui-state-default ui-corner-all',
                'title' => 'Delete the comment',
                ),
              html => TRUE,
              )
            ) .
          l('<span class="ui-icon ui-icon-circle-check"></span>' .
            '<span class="text">Approve comment</span>', 
            'admin/moderation/comment/action/approve/' . $cid . '/' . ucm_keygen($cid, 'approve'), 
            array(
              attributes => array (
                'id' => 'approve_comment-' . $cid,
                'class' => 'approve colorbox-form ui-state-default ui-corner-all',
                'title' => 'Approve the comment',
                ),
              html => TRUE,
              )
            ) .
        '</p>';

      $list .= '<div class="clear"></div>';

      $data['cid'] = $cid;
      $list .= 
        '<div style="display: none;">' . 
          drupal_get_form('ucm_delete_comment_form', $data) . 
        '</div>';

      $list .= '</div>';
    }
  }

  if($list) {
    $list = $filter_form . '<div id="accordion">' . $list . '</div>';
  } else {
    $list .= $filter_form . 'There are no comments pending for moderation.';
  }
  return $list;
}

/**
 * Function: Filter list form items
 * @param object $form_state
 * @param object $data
 * @return 
 */
function ucm_list_taxonomy_form($form_state, $data){
  $form['ucm'] = array(
    '#type' => 'fieldset',
    '#title' => t('Filter comments display'),
    '#description' => t('You can display the comments by following topics'),
    '#action' => t('#'),
    '#tree' => TRUE,
    '#style' => '',
    '#attributes' => array( 'class' => 'select_topic'),	
  );

  if(is_array($data)){   
    foreach($data as $key => $val){
      $temp = 'taxonomy_' .$key;
	    $vocab_name = $data['vocab_name'][$key]. $link;
 
      //This generates topics' checkboxes based on vocab name 
      //Checkboxes in $data[$key]
      if($key!='vocab_name'){
        $form['ucm'][$temp] = array(
          '#type' => 'checkboxes',
          '#title' => t($vocab_name),
          '#attributes' => array('class' => 'topic_id'), 
          '#options' =>  $data[$key] ,
          '#description' => t('')
        );   
      }
    }

  $form['ucm']['button'] = array(
    '#type' => 'button',
    '#title' => t('Refine search filter'),
    '#action' => t('#'),
    '#tree' => TRUE,
  	'#id' => 'filter-button',
    '#value' => 'Refine search filter',
    '#attributes' => array( 'class' => 'select_button'),	
  );    
    
    
  }

    return $form; 

}

function ucm_ago($timestamp){
   $difference = time() - $timestamp;
   $periods = array("second", "minute", "hour", "day", "week", "month", "years", "decade");
   $lengths = array("60","60","24","7","4.35","12","10");
   for($j = 0; $difference >= $lengths[$j]; $j++)
     $difference /= $lengths[$j];
     $difference = round($difference);
     if($difference != 1) $periods[$j].= "s";
     $text = "$difference $periods[$j] ago";
   return $text;
  }

/**
 * 
 * 
 * @param object $form_state
 * @param object $data [optional]
 * @return 
 */
function ucm_delete_comment_form($form_state, $data=''){
  $reasons = chop(variable_get('UCM_reason', ''));
  $reason = explode("\n", $reasons);
  
  $form_id = 'delete_form-' . $data['cid'];
  $delete_cid = 'delete_cid_' . $data['cid'];
 
  $form = array(
    '#type' => 'form',
    '#method' => 'post',
    '#attributes' => array('class' => 'delete_forms'),
    '#id' => t($form_id),   
    '#tree' => TRUE,
  );  
  $form[$delete_cid] = array(
    '#type' => 'fieldset',
    '#title' => t('Select the reason why delete this comment'),
    '#attributes' => array( 'class' => '', 'id' => ''), 
    '#tree' => TRUE,
  );  
  $form[$delete_cid]['reason'] = array(
    '#type' => 'radios',
    '#title' => t('Reasons'),
    '#default_value' => '',
    '#attributes' => array( 'class' => 'delete_reasons', 'id' => ''),
    '#options' =>  $reason ,
    '#name' => 'reason[]',
    '#description' => t('Select one of the reason why delete this comment'),
  );

  $form[$delete_uid]['submit'] = array(
    '#type' => 'button', 
    '#attributes' => array( 'class' => 'ui-state-default ui-corner-all'),
    '#name' => 'step',    
    '#id' => 'confirm_delete-' . $data['cid'], 
    '#value' => t('Confirm delete'),
  );
  $form[$delete_uid]['cancel'] = array(
    '#type' => 'button',
    '#attributes' => array( 'class' => 'ui-state-default ui-corner-all'),
    '#name' => 'step',       
    '#id' => 'cancer_delete-' . $data['cid'], 
    '#value' => t('Cancel'),
  ); 
 
  $form[$delete_uid]['hidden'] = array(
    '#type' => 'hidden', 
    '#name' => 'key',    
    '#attributes' => array( 'class' => 'key'), 
    '#value' => t(ucm_keygen($data['cid'],'delete')) ,
  ); 
 
  return $form;
}

function ucm_delete_single_comment_form($cid ){
    $data[cid]=$cid;
	  print drupal_get_form('ucm_delete_comment_form', $data);
}
