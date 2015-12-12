#ifdef __GTK_H__
void framerate_init (GtkWidget *canvas, void (*paint)(GtkWidget *));
#endif

void framerate_repaint (void);
void framerate_destroy (void);
