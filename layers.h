struct layer {
	bool (*init)     (void);
	void (*paint)    (void);
	void (*zoom)     (const unsigned int zoom);
	void (*resize)   (const unsigned int width, const unsigned int height);
	void (*destroy)  (void);
};

bool layers_init (void);
void layers_destroy (void);
void layers_paint (void);
void layers_zoom (const unsigned int zoom);
void layers_resize (const unsigned int width, const unsigned int height);
