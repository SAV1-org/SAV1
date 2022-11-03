

typedef struct ParseContext {
    std::uint8_t *data;
    std::size_t size;
    std::size_t capacity;
    std::uint64_t timecode;
    webm::FileParser;
};

void
parse_init(struct ParseContext *context, char *file_name);

void
parse_destroy(struct ParseContext *context);

int
parse_get_next_frame(struct ParseContext *context);
