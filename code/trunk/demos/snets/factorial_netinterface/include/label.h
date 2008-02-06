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

/* Struct to store the static and temporary names */
typedef struct label snetin_label_t;

/* Init new labels structure with given statistic names 
 *
 * @param static_labels Static labels to be used.
 * @param len Number of static labels.
 *
 * @return Initiated label structure.
 *
 */

extern snetin_label_t *SNetInLabelInit(char **static_labels, int len);

/* Free memory used in label structure.
 *
 * @param Label structure to be deleted.
 *
 */

extern void SNetInLabelDestroy(snetin_label_t *labels);

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

extern int SNetInSearchIndexByLabel(snetin_label_t *labels, const char *label);

/* Search for label by given index from the structure.
 *
 * @param labels Label structure to be searched.
 * @param index Index to search for.
 *
 * @return Label corresponding to the given index.
 * @return or NULL if no label with the given index was found.
 *
 */


extern char *SNetInSearchLabelByIndex(snetin_label_t *labels, int index);

#endif /* LABEL_H_ */

