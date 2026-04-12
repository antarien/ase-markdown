#include <ase/markdown/markdown.hpp>
#include <cstdlib>

namespace ase::markdown {

Document parse(const char* input, uint32_t len, ParseOptions opts) {
    (void)input;
    (void)len;
    (void)opts;

    Document doc{};
    doc.buffer_size = DOCUMENT_ARENA_SIZE;
    doc.buffer = static_cast<char*>(std::malloc(doc.buffer_size));
    doc.arena = static_cast<ase::alloc::Arena*>(std::malloc(sizeof(ase::alloc::Arena)));
    *doc.arena = ase::alloc::Arena(doc.buffer, doc.buffer_size);

    doc.root = alloc_node(doc, NODE_DOCUMENT);
    return doc;
}

void free_document(Document& doc) {
    if (doc.arena) {
        std::free(doc.arena);
        doc.arena = nullptr;
    }
    if (doc.buffer) {
        std::free(doc.buffer);
        doc.buffer = nullptr;
    }
    doc.root = nullptr;
    doc.buffer_size = 0;
}

}  // namespace ase::markdown
