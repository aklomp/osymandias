#ifdef __GTK_H__
void framerate_init (GtkWidget *widget, void (*paint_callback)(GtkWidget *widget));
#endif

void framerate_request_refresh (void);
void framerate_destroy (void);
