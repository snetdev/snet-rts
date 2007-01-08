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


#define MAX_BOXTITLE_LEN  100
#define BUF_SIZ 4096

typedef struct {

  char *text;
  GtkWidget *view;
  GtkButton *button;

} container_t;



char buf[4096];

int s, c;
unsigned int addr_len;
struct sockaddr_in addr;
bool terminate = false;


 void Destroy( GtkWidget *widget, gpointer data) {
    gtk_main_quit ();
  }


gboolean DeleteEvent( GtkWidget *widget, GdkEvent *event, gpointer data) {
    g_print ("\nWindow closed!");

    return( FALSE);
}




  

void ClickedRelease( GtkWidget *widget, container_t *cont) {
    
    GtkWidget *view;
    GtkTextBuffer *buffer;

    view = cont->view;

    if( !( terminate)) {
      send(c, "RELEASE\n", 9, 0);
      if( recv(c, buf, sizeof(buf), 0) <= 0) {
        return;
      }
    }
              
    if( strncmp( buf, "Buffer empty", 
          strlen( "Buffer empty")) == 0) {
      gtk_button_set_label( cont->button, "Fetch Record");      
    }
    else {
      if( strcmp( buf, "The current record contains:\n\n - \n - control message: terminate\n - \n") == 0) {
        terminate = true;
        gtk_button_set_label( cont->button, "Release & Exit");
        
        g_signal_connect (G_OBJECT (cont->button), "clicked",
		                      G_CALLBACK (Destroy),  NULL);
      }
      else {
        gtk_button_set_label( cont->button, "Release Record");      
      }
    }

    buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW( view));
    gtk_text_buffer_set_text( GTK_TEXT_BUFFER( buffer), buf, -1);
    gtk_text_view_set_buffer( GTK_TEXT_VIEW( view), GTK_TEXT_BUFFER( buffer));
}



int main( int argc, char **argv) {

  int server_port;
  int one = 1;

  char *window_heading;

  GtkWidget *window;
  GtkWidget *button;
  GtkWidget *box;
  GtkWidget *label;
  GtkWidget *view_record;
  GtkWidget *separator;
  GtkTextBuffer *txt_buffer;

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




  #define TITLE "S-Net Box Viewer"
  title = SNetMemAlloc( sizeof( char*));
  *title = SNetMemAlloc( (strlen( TITLE) + 1) * sizeof( char));
  strcpy( title[0], TITLE);

//  text = SNetMemAlloc( 4096 * sizeof( char));
  cont = SNetMemAlloc( sizeof( container_t));
  boxname = SNetMemAlloc( 1024 * sizeof( char));

  recv(c, boxname, 1024, 0);
//  recv(c, buf, sizeof( buf), 0);

  strcpy(buf, "\n\nBuffer Empty.\n\n");

  gtk_init( &one, &title);


  cont->text = buf;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  
  g_signal_connect (G_OBJECT (window), "delete_event", 
                    G_CALLBACK (DeleteEvent), NULL);
  
  g_signal_connect (G_OBJECT (window), "destroy",
		                G_CALLBACK (Destroy), NULL);

  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
  
  button = gtk_button_new_with_label ("Fetch Record");
  cont->button = GTK_BUTTON( button);

  box = gtk_vbox_new (FALSE, 0);

  window_heading = SNetMemAlloc( ( MAX_BOXTITLE_LEN + 50) * sizeof( char)); 
  strcpy( window_heading, "Observing Box   <");
  strcat( window_heading, boxname);
  strcat( window_heading, ">  ");
  label = gtk_label_new ( window_heading);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  
  view_record = gtk_text_view_new();
  txt_buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW( view_record));
  gtk_text_buffer_set_text( txt_buffer, buf, -1);
  gtk_text_view_set_editable( GTK_TEXT_VIEW( view_record), FALSE);
  gtk_text_view_set_cursor_visible( GTK_TEXT_VIEW( view_record), FALSE);

  cont->view = view_record;

  g_signal_connect (G_OBJECT (button), "clicked",
		                G_CALLBACK (ClickedRelease),  cont);
  
  separator = gtk_hseparator_new ();
  
  gtk_box_pack_end( GTK_BOX( box), button, FALSE, FALSE, 0);
  gtk_box_pack_end( GTK_BOX( box), separator, FALSE, TRUE, 15);
  gtk_box_pack_start( GTK_BOX( box), label, FALSE, FALSE, 0);
  gtk_box_pack_end( GTK_BOX( box), view_record, FALSE, FALSE, 0);
  
  gtk_widget_show( button);
  gtk_widget_show( label);
  gtk_widget_show( view_record);
  gtk_widget_show( separator);

  gtk_container_add (GTK_CONTAINER (window), box);

  gtk_widget_show (box);

  gtk_widget_show  (window);


   
  gtk_main ();



  close(c);
  close(s);

  return( 0);
}
