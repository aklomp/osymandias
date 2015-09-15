struct layer {
	bool (*init)     (void);
	bool (*occludes) (void);
	void (*paint)    (void);
	void (*zoom)     (void);
	void (*destroy)  (void);
};

bool layers_init (void);
void layers_destroy (void);
void layers_paint (void);
void layers_zoom (void);
