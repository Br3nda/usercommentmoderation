<?php
//$Id: user_comment_moderation_admin_settings.inc,v 1.8 2010/05/27 02:56:44 pangv Exp $

/**
 * Implementation of hook admin
 * 
 * @return 
 */
function user_comment_moderation_admin() {
  $s= "SELECT * FROM {flags}";
  $r = db_query($s);
  while ($rs = db_fetch_object($r)) {
    $f = unserialize($rs->options);
    $flags[$rs->name]=$rs->title;
  }
  
  $form = array();
  $form['UCM_list_max'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum number of comments per page'),
    '#default_value' => variable_get('UCM_list_max', 20),
    '#size' => 2,
    '#maxlength' => 2,
    '#description' => t('Display number of comments per page.'),
    '#required' => TRUE,
  );
  $form['UCM_lock'] = array(
    '#type' => 'checkbox',
    '#attributes' => array(
      'disabled' => 'disabled',
    ),
    '#title' => t('Enable comment lock'),
    '#default_value' => variable_get('UCM_lock', 1),
    '#description' => t('Enable comment locking when moderator viewing the
      comment. (Currently must be enabled)'),
  );
  $form['UCM_lock_expiry_time'] = array(
    '#type' => 'textfield',
    '#title' => t('Comment lock expiry time'),
    '#default_value' => variable_get('UCM_lock_expiry_time', 120),
    '#size' => 4,
    '#maxlength' => 4,
    '#description' => t('Set the comment lock expiry time in seconds'),
    '#required' => TRUE,
  );
	$options = ucm_get_vocabularies();
  $form['UCM_vocabs'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Select the vocabularies are used by the comments'),
    '#default_value' => variable_get('UCM_vocabs', ''),
    '#options' => $options,
	  '#description' => t('Select the vocabularies are used by the comments,
      Only comments used by the selected vocabularies will appear on the user
      comment moderation list.'),
    '#required' => TRUE,
  );
  $form['UCM_flag'] = array(
    '#type' => 'radios',
    '#title' => t('Mark comment with the following flag when comment is
      approved.'),
    '#default_value' => variable_get('UCM_flag', ''),
    '#options' => $flags,
	  '#description' => t('Mark comment with the following flag when comment is
      approved. (requires Flags module)'),
    '#required' => TRUE,
  );
  $default_reasons = 'SPAM\nAbusive\nNot on topic';
  $form['UCM_reason'] = array(
    '#type' => 'textarea',
    '#title' => t('Enter the potential reasons for banning a comment'),
    '#size' => 120,
    '#default_value' => variable_get('UCM_reason', $default_reasons),
    '#description' => t('Seperate each reason by new line. If you are using
      WYSIWYG editor for this settings page, please disable it.'),
    '#required' => TRUE,
  );
  $theme = ucm_jquery_ui_themes();
  $form['UCM_theme'] = array(
    '#type' => 'select',
    '#title' => t('Set JQuery UI theme'),
    '#default_value' => variable_get('UCM_theme','base'),
    '#options' =>  $theme,
    '#description' => t('Select the JQuery UI theme to be used by this module.'),
    '#required' => TRUE,
  );

  return system_settings_form($form);
}

/**
 * Find all the vocabularies
 * 
 * @return array of role vocabularies
 */
function ucm_get_vocabularies() {
	$vocab = taxonomy_get_vocabularies();
  if(is_array($vocab)) {
  	foreach($vocab as $key => $val) {
  	  if($vocab[$key]->vid) {
        $vid = $vocab[$key]->vid;
        $output[$vid]= $vocab[$key]->name;
	    }
    }
    return $output;
  } else {
    return FALSE;
  }
}

/**
 * Find and return all jQuery UI themes
 * 
 * @return array of jQuery UI themes
 */
function ucm_jquery_ui_themes(){
  $search_dir = drupal_get_path('module','jquery_ui').'/jquery.ui/themes';
  $exclude = array('.', '..', 'CVS');
  $dir =  file_scan_directory($search_dir, '.*', $exclude, 0, FALSE);
  
  if(is_array($dir)){
    foreach($dir as $key => $val){
      $output[$val->basename] = $val->basename; 
    }
    return $output;
  } else {
    return FALSE;    
  }
}

