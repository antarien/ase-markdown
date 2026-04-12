#include <ase/markdown/markdown.hpp>
#include <cstdlib>
#include <cstring>

namespace ase::markdown {

Document parse(const char* input, uint32_t len, ParseOptions opts) {
    (void)input;
    (void)len;
    (void)opts;

    // Stub: returns empty document with root node
    auto* arena = std::malloc(sizeof(Node));
    auto* root = new (arena) Node{};
    root->type = NODE_DOCUMENT;

    Document doc{};
    doc.root = root;
    doc.arena = arena;
    return doc;
}

void free_document(Document& doc) {
    if (doc.arena) {
        std::free(doc.arena);
        doc.arena = nullptr;
    }
    doc.root = nullptr;
}

}  // namespace ase::markdown
