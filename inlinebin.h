enum inlinebin {
	INLINEBIN_NONE,
	SHADER_CURSOR_FRAGMENT,
};

void inlinebin_get (enum inlinebin member, const uint8_t **buf, size_t *len);
