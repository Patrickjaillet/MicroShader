#include "document.h"

#include <cstddef>

#include "glsl_language.h"
#include "../render/default_shader.h"

std::string document_display_name(const std::string& file_path)
{
    if (file_path.empty())
    {
        return "untitled";
    }
    std::size_t separator = file_path.find_last_of("/\\");
    if (separator == std::string::npos)
    {
        return file_path;
    }
    return file_path.substr(separator + 1);
}

void configure_document_editors(Document& document)
{
    document.source_editor.SetLanguageDefinition(glsl_language_definition());
    document.source_editor.SetPalette(TextEditor::GetDarkPalette());
    document.golfed_editor.SetLanguageDefinition(glsl_language_definition());
    document.golfed_editor.SetPalette(TextEditor::GetDarkPalette());
    document.golfed_editor.SetReadOnly(true);
}

std::unique_ptr<Document> make_document(const std::string& source_text, const std::string& file_path)
{
    auto document = std::make_unique<Document>();
    configure_document_editors(*document);
    document->source_editor.SetText(source_text);
    document->saved_source = source_text;
    document->file_path = file_path;
    document->display_name = document_display_name(file_path);
    return document;
}

std::unique_ptr<Document> make_default_document()
{
    return make_document(kDefaultShaderSource, std::string());
}
