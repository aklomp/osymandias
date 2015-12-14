struct tiledrawer {
	const struct tilepicker *pick;
	struct {
		unsigned int world;
		unsigned int found;
	} zoom;
	GLuint texture_id;
};

void tiledrawer (const struct tiledrawer *);
