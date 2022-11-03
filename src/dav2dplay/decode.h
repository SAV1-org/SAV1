typedef struct DecodeContext {
    Dav1dContext *dc;
    Dav1dPicture *picture;
};

void
decode_init(struct DecodeContext *context);

void
decode_destroy(struct DecodeContext *context);

int
decode_frame(struct DecodeContext *context, std::uint8_t *data, std::size_t size);
