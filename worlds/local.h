struct world {
	const float *(*matrix)	(void);
	void (*moveto)		(const float lat, const float lon);
	void (*project)		(float *vertex, float *normal, const float lat, const float lon);
	void (*zoom)		(const unsigned int zoom, const unsigned int size);
};
