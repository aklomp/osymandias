// Hold init data for GTK signals:
struct signal {
	const gchar	*signal;
	GCallback	 handler;
	GdkEventMask	 mask;
};

void signal_connect (GtkWidget *widget, struct signal *signals, size_t members);
