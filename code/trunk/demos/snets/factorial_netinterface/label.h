#ifndef LABEL_H_
#define LABEL_H_

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

#define LABEL_ERROR -1

/* Struct to store temporary labels and their number mappings*/
typedef struct temp_label{
  char *label;
  int index;
  int ref_count;
  struct temp_label *next;
}temp_label_t;

/* Struct to store the static and current temporary names */
typedef struct label_data{
  const char *const *labels; /* static labels*/
  int number_of_labels;      /* number of static labels*/
  pthread_mutex_t mutex;     /* mutex for access control to temporary labels */
  temp_label_t *temp_labels; /* temporary labels */
}label_t;

/* Init new labels structure with given statistic names 
 *
 * @param static_labels Static labels to be used.
 * @param len Number of static labels.
 *
 * @return Initiated label structure.
 *
 */

extern label_t *initLabels(const char *const *static_labels, int len);

/* Free memory used in label structure.
 *
 * @param Label structure to be deleted.
 *
 */

extern void deleteLabels(label_t *labels);

/* Search for index by given label from the structure.
 * If the given label is not found, a new label with 
 * the next possible index is added to the structure.
 *
 * @param labels Label structure to be searched.
 * @param label The name to search for.
 *
 * @return Index of the given label.
 * @return LABEL_ERROR, if the label parameter given was NULL.
 *
 */

extern int searchIndexByLabel(label_t *labels, const char *label);

/* Search for label by given index from the structure.
 *
 * @param labels Label structure to be searched.
 * @param index Index to search for.
 *
 * @return Label corresponding to the given index.
 * @return or NULL if no label with the given index was found.
 *
 */


extern char *searchLabelByIndex(label_t *labels, int index);

#endif /* LABEL_H_ */

