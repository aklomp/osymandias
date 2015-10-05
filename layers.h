struct layer {
	bool (*init)     (void);
	bool (*occludes) (void);
	void (*paint)    (void);
	void (*zoom)     (const unsigned int zoom);
	void (*destroy)  (void);
};

bool layers_init (void);
void layers_destroy (void);
void layers_paint (void);
void layers_zoom (const unsigned int zoom);
