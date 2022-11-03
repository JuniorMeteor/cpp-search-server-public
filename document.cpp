#include  "document.h"

std::ostream& operator<<(std::ostream& os, const Document& doc) {
    os << "{ document_id = " << doc.id
        << ", relevance = " << doc.relevance
        << ", rating = " << doc.rating
        << " }";
    return os;
}

void PrintDocument(const Document& document) {
    std::cout << "{ "
        << "document_id = " << document.id << ", "
        << "relevance = " << document.relevance << ", "
        << "rating = " << document.rating
        << " }" << std::endl;
}