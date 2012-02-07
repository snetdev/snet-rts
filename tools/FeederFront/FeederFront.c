#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <memfun.h>

#include <bool.h>

#include <gtk/gtk.h>


#define AETHER_DEMO 1


#define MAX_BOXTITLE_LEN  100
#define BUF_SIZE 4096

typedef struct {

  char *text;
  GtkWidget **check_boxes;
  GtkWidget **enter_buffers;
  GtkWidget **enter_values;
  int num;

} container_t;



char buf[4096];
char enter_buf[4096];

int s, c;
unsigned int addr_len;
struct sockaddr_in addr;




static void itoa (int val, char *str, int rdx) {
  #define MAX_INT_LEN 10
  int i;
  int len = 0;
  int offset = 0;
  char result[MAX_INT_LEN + 1];

  result[MAX_INT_LEN] = '\0';
  offset = 48;                  /* 0=(char)48 */

  for( i = 0; i < MAX_INT_LEN; i++) {
      result[MAX_INT_LEN - 1 - i] = (char) ( ( val % rdx) + offset);
      val /= rdx;
      if ( val == 0) {
          len = ( i + 1);
          break;
      }
  }
  strcpy( str, &result[MAX_INT_LEN - len]);
}






static char *GetNextName( char *src, int *i, bool fill) {

#define STR_LEN 12
  int j=0;
  char *str;

  str = SNetMemAlloc( STR_LEN * sizeof( char));

  while( src[*i] != ',') {
    str[j] = src[*i];
    *i += 1; j += 1;
  }
//  if( fill) {
//    while( j<STR_LEN-1) {
//      str[j++] = ' ';
//    }
//    str[STR_LEN-1] = '\0';
//  }
//  else {
    str[j] = '\0';
//  }

  return(  str);
}


void Destroy( GtkWidget *widget, gpointer data) {
    gtk_main_quit ();
  }


  gboolean DeleteEvent( GtkWidget *widget, GdkEvent *event, gpointer data) {
    g_print ("\nWindow closed!");

    return( FALSE);
  }


void ToggleEvent( GtkWidget *widget, gpointer data) {
  // empty
} 



void ClickedRelease( GtkWidget *widget, container_t *cont) {
    
    int i;
    char tmp_str[5];
 
    for( i=0; i<sizeof( buf); i++) {
      buf[i] = '\0';
    }

    for( i=0; i<cont->num; i++) {
      GtkWidget *box, *entry;
      box = cont->check_boxes[i];
      entry = cont->enter_values[i];

#ifndef AETHER_DEMO

      if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( box))) {
        itoa( i, tmp_str, 10);
        strcat( buf, tmp_str);
        strcat( buf, "=");
        strcat( buf, gtk_entry_get_text( GTK_ENTRY( entry)));
        strcat( buf, ",");
      }
#else

      if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( box))) {
        if( i>1) {
          itoa( i+9, tmp_str, 10);
        } else {
          itoa( i, tmp_str, 10);
        }
        strcat( buf, tmp_str);
        strcat( buf, "=");
        strcat( buf, gtk_entry_get_text( GTK_ENTRY( entry)));
        strcat( buf, ",");
      }

#endif

    }
    
    
    send(c, buf, strlen( buf) + 1, 0);  
}

void ClickedTerminate( GtkWidget *widget, container_t *cont) {
    
    int i;
 
    for( i=0; i<sizeof( buf); i++) {
      buf[i] = '\0';
    }

    strcpy( buf, "TERMINATE");
    send(c, buf, strlen( buf) + 1, 0); 

    gtk_main_quit();
}

int main( int argc, char **argv) {

  int server_port;
  int one = 1;
  int i, j;

  char *window_heading;
  char *tmp_str;

  GtkWidget *window;
  GtkWidget *button, *terminate;
  GtkWidget *box;
  GtkWidget *label, *label2;
  GtkWidget *separator, *separator2;

  int num_names;
  GtkWidget **check_boxes;
  GtkWidget **enter_values;

  GtkWidget *vbox1, *vbox2, *terminate_box, *entire_box;
  GtkTextBuffer **enter_buffers;
//  bool *is_checked;
  
  container_t *cont;
  char **title;
  char *boxname;



// --------------------
  if( argc < 2) {
    exit( 1);
  }

  server_port = atoi( argv[1]);

  s = socket(PF_INET, SOCK_STREAM, 0);
  if (s == -1) {
    perror( "socket() failed");
    return 1;
  }

  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(server_port);
  addr.sin_family = AF_INET;

  if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind() failed");
    return 2;
  }

  if (listen(s, 3) == -1) {
    perror("listen() failed");
    return 3;
  }


  addr_len = sizeof(addr);
  c = accept(s, (struct sockaddr*)&addr, &addr_len);
  
  if (c == -1) {
    perror("accept() failed");
  }
// --------------------




  #define TITLE "S-Net Feeder"
  title = SNetMemAlloc( sizeof( char*));
  *title = SNetMemAlloc( (strlen( TITLE) + 1) * sizeof( char));
  strcpy( title[0], TITLE);

  cont = SNetMemAlloc( sizeof( container_t));
  boxname = SNetMemAlloc( 1024 * sizeof( char));

  recv(c, buf, sizeof( buf), 0);
  
  gtk_init( &one, &title);


  window_heading = SNetMemAlloc( ( MAX_BOXTITLE_LEN + 50) * sizeof( char)); 
  strcpy( window_heading, "Enter data for new record:");
  label = gtk_label_new ( window_heading);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);


  j = 0;
  tmp_str = GetNextName( buf, &j, false); 
  num_names = atoi( tmp_str);

  check_boxes = SNetMemAlloc( num_names * sizeof( GtkWidget*));
  enter_values = SNetMemAlloc( num_names * sizeof( GtkWidget*));

//  h_boxes = SNetMemAlloc( num_names * sizeof( GtkWidget*));

  
  enter_buffers = SNetMemAlloc( num_names * sizeof( GtkTextBuffer*));
  //is_checked = SNetMemAlloc( num_names * sizeof( bool));

  cont->check_boxes = check_boxes;
  cont->enter_values = enter_values;
  //cont->enter_buffers = enter_buffers;
//  cont->is_checked = is_checked;

  box = gtk_hbox_new (FALSE, 1);
  vbox1 = gtk_vbox_new( FALSE, 0);
  vbox2 = gtk_vbox_new( FALSE, 0);
  terminate_box = gtk_vbox_new( FALSE, 0);
  entire_box = gtk_vbox_new( FALSE, 0);


#ifndef AETHER_DEMO

  cont->num = num_names;
  for( i=0; i<num_names; i++) {

    //is_checked[i] = false;

    j += 1;  // GetNextName points to ','
    tmp_str = GetNextName( buf, &j, true);
    check_boxes[i] = gtk_check_button_new_with_label ( tmp_str);
    g_signal_connect ( check_boxes[i], "toggled", G_CALLBACK (ToggleEvent), cont);

    enter_values[i] = gtk_entry_new_with_max_length( 5);
    gtk_entry_set_editable( GTK_ENTRY( enter_values[i]), TRUE);

    gtk_box_pack_start( GTK_BOX( vbox1), check_boxes[i], FALSE, FALSE, 3);
    gtk_box_pack_start( GTK_BOX( vbox2), enter_values[i], FALSE, FALSE, 0);
  }
#else
  // stripped down version for aether (watch out, hardcoded boundaries)

  cont->num = (num_names-9);
  for( i=0; i<2; i++) {
    
    char *one, *two;

    one = SNetMemAlloc( 2*sizeof( char));
    two = SNetMemAlloc( 2*sizeof( char));

    strcpy( one, "1");
    strcpy( two, "2");

    j += 1;  // GetNextName points to ','
    tmp_str = GetNextName( buf, &j, true);
    check_boxes[i] = gtk_check_button_new_with_label ( tmp_str);
    g_signal_connect ( check_boxes[i], "toggled", G_CALLBACK (ToggleEvent), cont);

    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( check_boxes[i]), TRUE);
     
    enter_values[i] = gtk_entry_new_with_max_length( 5);
    if( i == 0) {
      gtk_entry_set_text( GTK_ENTRY( enter_values[i]), two);
    }
    else {
      gtk_entry_set_text( GTK_ENTRY( enter_values[i]), one);
      gtk_entry_set_editable( GTK_ENTRY( enter_values[i]), FALSE);
    }
    
    gtk_box_pack_start( GTK_BOX( vbox1), check_boxes[i], FALSE, FALSE, 3);
    gtk_box_pack_start( GTK_BOX( vbox2), enter_values[i], FALSE, FALSE, 0);
  }
  
  for( i=2; i<11; i++) {

    j += 1;  // GetNextName points to ','
    tmp_str = GetNextName( buf, &j, true);
  }

  for( i=11; i<num_names; i++) {

    j += 1;  // GetNextName points to ','
    tmp_str = GetNextName( buf, &j, true);
    check_boxes[i-9] = gtk_check_button_new_with_label ( tmp_str);

    g_signal_connect ( check_boxes[i-9], "toggled", G_CALLBACK (ToggleEvent), cont);
    enter_values[i-9] = gtk_entry_new_with_max_length( 5);
    gtk_entry_set_editable( GTK_ENTRY( enter_values[i-9]), TRUE);

    gtk_box_pack_start( GTK_BOX( vbox1), check_boxes[i-9], FALSE, FALSE, 3);
    gtk_box_pack_start( GTK_BOX( vbox2), enter_values[i-9], FALSE, FALSE, 0);
  }
#endif

  gtk_box_pack_start( GTK_BOX( box), vbox1, FALSE, FALSE, 0);
  gtk_box_pack_start( GTK_BOX( box), vbox2, FALSE, FALSE, 0);


  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  
  g_signal_connect (G_OBJECT (window), "delete_event", 
                    G_CALLBACK (DeleteEvent), NULL);
  
  g_signal_connect (G_OBJECT (window), "destroy",
		                G_CALLBACK (Destroy), NULL);

  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
  

  separator = gtk_hseparator_new();
  separator2 = gtk_hseparator_new();

  button = gtk_button_new_with_label ("Send Record");

  label2 = gtk_label_new( NULL);
  
  gtk_label_set_markup( GTK_LABEL( label2), "<i>Clicking the button below will send\n"
                          "a <b>terminate</b> signal to the S-NET and\n"
                          "this application will exit.</i>");



  terminate = gtk_button_new_with_label ("Send Terminate");

  gtk_box_pack_start( GTK_BOX( box), button, FALSE, FALSE, 0);
  
  
  gtk_box_pack_start( GTK_BOX( terminate_box), separator, FALSE, FALSE, 5);
  gtk_box_pack_start( GTK_BOX( terminate_box), label2, FALSE, FALSE, 0);
  gtk_box_pack_start( GTK_BOX( terminate_box), separator2, FALSE, FALSE, 0);
  gtk_box_pack_start( GTK_BOX( terminate_box), terminate, FALSE, FALSE, 0);

  gtk_box_pack_start( GTK_BOX( entire_box), box, FALSE, FALSE, 0);
  gtk_box_pack_start( GTK_BOX( entire_box), terminate_box, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (button), "clicked",
		                G_CALLBACK (ClickedRelease),  cont);
  g_signal_connect (G_OBJECT (terminate), "clicked",
		                G_CALLBACK (ClickedTerminate),  cont);
  
  gtk_container_add (GTK_CONTAINER (window), entire_box);

  gtk_widget_show_all(window);
   
  gtk_main ();



  close(c);
  close(s);

  return( 0);
}
