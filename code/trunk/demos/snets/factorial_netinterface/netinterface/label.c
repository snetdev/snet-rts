/*******************************************************************************
 *
 * $Id$
 *
 * Author: Jukka Julku, VTT Technical Research Centre of Finland
 * -------
 *
 * Date:   10.1.2008
 * -----
 *
 * Description:
 * ------------
 *
 * Label functions for S-NET network interface.
 *
 *******************************************************************************/

#include <stdlib.h>
#include "label.h"
#include "str.h"
#include <pthread.h>
#include <memfun.h>
#include <stdio.h>

/* TODO: 
 * The reference counting system might not work correctly if the
 * SNet runtime can copy unknown data fields. Check how this work
 * and adjust reference counting accordingly.
 */

label_t *initLabels(const char *const *labels, int len){
  label_t *temp = SNetMemAlloc(sizeof(label_t));
  temp->labels = labels;
  temp->number_of_labels = len;
  pthread_mutex_init(&temp->mutex, NULL);
  temp->temp_labels = NULL;
  
  return temp;
}

void deleteLabels(label_t *labels){
  pthread_mutex_destroy(&labels->mutex);

  //remove are temporary labels
  while(labels->temp_labels != NULL){
    temp_label_t *temp = labels->temp_labels->next;
    SNetMemFree(labels->temp_labels);
    labels->temp_labels = temp;
  }
  SNetMemFree(labels);
}

int searchIndexByLabel(label_t *labels, const char *label){
  int i = 0;
  int index = LABEL_ERROR;
  if(label == NULL || labels == NULL){
    return index;
  }

  //search static labels
  for(i = 0; i < labels->number_of_labels; i++) {
    if(STRcmp(labels->labels[i], label) == 0) {
      return i;
    }
  }

  //search temporary labels
  pthread_mutex_lock(&labels->mutex);

  temp_label_t *templabel = labels->temp_labels;
  while(templabel != NULL){
    if(STRcmp(templabel->label, label) == 0) {
      templabel->ref_count++;
      index = templabel->index;
      
      pthread_mutex_unlock(&labels->mutex);
      return index;
    }
    templabel = templabel->next;
  }

  // Unknown label, create new one
  temp_label_t *new_label = SNetMemAlloc(sizeof(temp_label_t)); 
  new_label->label = STRcpy(label);

  if(labels->temp_labels == NULL) {
    new_label->index = labels->number_of_labels;
  }
  else{
    new_label->index = labels->temp_labels->index + 1;
  }

  new_label->next = labels->temp_labels;
  labels->temp_labels = new_label;

  new_label->ref_count = 1;

  index = new_label->index;
  
  pthread_mutex_unlock(&labels->mutex);
  return index;
}

char *searchLabelByIndex(label_t *labels, int i){
  if(labels == NULL || i < 0){
    return NULL;
  }

  //search static labels
  if(i < labels->number_of_labels){
    return STRcpy(labels->labels[i]);    
  }
  
  //search temporary labels
  pthread_mutex_lock(&labels->mutex);

  temp_label_t *templabel = labels->temp_labels;
  temp_label_t *last = NULL;
  char *t = NULL;
  while(templabel != NULL){
    if(templabel->index == i){
      t = STRcpy(templabel->label);
      //remove label if the reference count goes to zero
      if(--templabel->ref_count == 0){
	if(templabel == labels->temp_labels){
	  labels->temp_labels = templabel->next;
	}
	else{
	  last->next = templabel->next;
	}
	SNetMemFree(templabel->label);
	SNetMemFree(templabel);
	templabel = NULL;
      }
      
      break;
    }
    last = templabel;
    templabel = templabel->next;
  }

  pthread_mutex_unlock(&labels->mutex);
  return t;
}
