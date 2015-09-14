struct program_cursor {
	float halfwd;
	float halfht;
};

struct program *program_cursor (void);
void program_cursor_use (struct program_cursor *);
